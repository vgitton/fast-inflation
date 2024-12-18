#pragma once

#include "../../inf/inf_problem/inflation.h"
#include "../../inf/inf_problem/network.h"
#include "../../inf/inf_problem/target_distr.h"
#include "../application.h"

/*! \file */

/*! \defgroup applications Applications
 * \brief This module documents the user code, i.e., the code that uses the fast inflation library
 * and defines concrete causal compatibility problems.
 *
 * \details For more information on how to register new applications, please see the module \ref user.
 * */

/*! \defgroup ejm Elegant Joint Measurement
    \ingroup applications
    \brief Proving the nonlocality of the Elegant Joint Measurement

    ### The EJM

    The EJM distribution \f$p(a,b,c)\f$ with \f$a,b,c\in\{0,1,2,3\}\f$ is defined as
    \f[
        \forall a\neq b \neq c \neq a :\
        p(a,a,a) = \frac{25}{256}, \quad
        p(a,a,b) = \frac{1}{256}, \quad
        p(a,b,c) = \frac{5}{256}.
    \f]
    The EJM is causally compatible with the quantum triangle network (i.e., using quantum states and measurements in the triangle network),
    but whether or not it is causally compatible with the classical triangle network was an open question for a long time (see the original paper of Nicolas Gisin: [https://arxiv.org/abs/1708.05556](https://arxiv.org/abs/1708.05556)).
    To check that the EJM is nonlocal, the user can directly run the user::ejm_check_nl_certificate application,
    which reads the certificate `data/ejm_224_nl_certificate.txt` and checks that is provides a valid nonlocality certificate for the EJM.
    The certificate was produced using the application user::ejm_find_nl_certificate, which takes about a week to run.

    ### Noise model

    We use the following noise model.
    Let \f$p_1\f$ be the distribution characterized by
    \f[
        \forall a\neq b \neq c \neq a \in \{0,1,2,3\} :\
        p_1(a,a,a) = \frac{1}{8}, \quad
        p_1(a,a,b) = 0, \quad
        p_1(a,b,c) = \frac{1}{48}.
    \f]
    We then consider the family of distributions
    \f[
        p_v = v\cdot p_1 + (1-v)\cdot u,
    \f]
    where \f$ v \in [0,1]\f$ and \f$u\f$ is the maximally mixed distribution.
    We can check that \f$p_{v = 75\%}\f$ is the EJM distribution: indeed, we have
    \f[
        p_{v = 75\%}(0,0,0) = \frac{3}{4}\cdot\frac{1}{8} + \frac{1}{4}\cdot\frac{1}{64}
        = \frac{3}{32} + \frac{1}{256} = \frac{25}{256},
    \f]
    \f[
        p_{v = 75\%}(0,0,1) = \frac{3}{4}\cdot0 + \frac{1}{4}\cdot\frac{1}{64} = \frac{1}{256},
    \f]
    \f[
        p_{v = 75\%}(0,1,2) = \frac{3}{4}\cdot\frac{1}{48} + \frac{1}{4}\cdot\frac{1}{64}
        = \frac{1}{64} + \frac{1}{256} = \frac{5}{256}.
    \f]
    As shown in the paper, this family of distributions satisfies the assumptions described in inf::VisProblem, so we can use a dichotomic search to obtain the critical visibility of inflation compatibility for this family of distributions.
*/

namespace user {

/*! \addtogroup ejm
 * @{
 * */

/*! \brief Triangle network with four outcomes per party */
inf::Network::ConstPtr get_ejm_network();

/*! \brief Get a distribution symmetric under party exchange and global outcome relablings characterized by \f$p(0,0,0)\f$, \f$p(0,0,1)\f$ and \f$p(0,1,2)\f$
    \details This method returns
    \f[
        p = \frac{\mathtt{s111}}{\mathtt{s\_denom}}\frac{[0,0,0] + [1,1,1] + \cdots}{4}
        + \frac{\mathtt{s112}}{\mathtt{s\_denom}}\frac{[0,0,1] + [0,0,2] + \cdots}{36}
        + \frac{\mathtt{s123}}{\mathtt{s\_denom}}\frac{[0,1,2] + [0,1,3] + \cdots}{24}.
    \f]
    More precisely, the common denominator is chosen to be the smallest multiple of 4, 36 and 24, which is 72, so that
    this method really returns
    \f[
        p = \frac{18*\mathtt{s111}}{72*\mathtt{s\_denom}}([0,0,0] + [1,1,1] + \cdots)
        + \frac{2*\mathtt{s112}}{72*\mathtt{s\_denom}}([0,0,1] + [0,0,2] + \cdots)
        + \frac{3*\mathtt{s123}}{72*\mathtt{s\_denom}}([0,1,2] + [0,1,3] + \cdots)
    \f]
    */
inf::TargetDistr::ConstPtr get_symmetric_distribution(Index s111,
                                                      Index s112,
                                                      Index s123,
                                                      Index s_denom);

/*! \brief This returns the distribution \f$p_v\f$ described in \ref ejm "the EJM module"
 * \details The visibility \f$v\in[0,1]\f$ is given by \f$v = \mathtt{vis}/\mathtt{vis\_denom}\f$.
 * The case of \f$v= 75\%\f$ corresponds to the EJM distribution. */
inf::TargetDistr::ConstPtr get_noisy_pureejm(Num vis, Num vis_denom);

/*! \brief The Elegant Joint Measurement distribution
    \details Equivalent to `user::get_symmetric_distribution(25, 9, 30, 64)` */
inf::TargetDistr::ConstPtr get_ejm_distribution();

/*! \brief This shows info about the symmetries of the EJM for various inflation sizes */
class ejm_sym : public user::Application {
  public:
    ejm_sym() : user::Application("ejm_sym", "Symmetries of the EJM", true) {}
    void run() override;
};

/*! \brief See user::test_dual_vector_eval() */
class ejm_eval : public user::Application {
  public:
    ejm_eval() : user::Application("ejm_eval",
                                   "Dual vector evaluation for the EJM",
                                   true) {}
    void run() override;
};

/*! \brief Displays information about the EJM symtree, see inf::EventTree and inf::TreeFiller */
class ejm_symtree : public user::Application {
  public:
    ejm_symtree() : user::Application("ejm_symtree",
                                      "Info about EJM symtree for 2x2x3 inflation",
                                      false) {}
    void run() override;
};

/*! \brief Placeholder for EJM inflation compatibility problems
 * \details This application looks at `--vis` command line argument to determine the target visibility,
 * which is then \f$v = \mathtt{vis}/512\f$, and the target distribution \f$p_v\f$ is described in the \ref ejm "EJM module". */
class ejm : public user::Application {
  public:
    ejm() : user::Application("ejm",
                              "Uses the --vis command line argument to determine the target visibility",
                              false) {}
    void run() override;
};

// Visibility studies

// 2x2x2 ==========================

/*! \brief Visibility of EJM under \f$2\times2\times2\f$ inflation, using only the order-2 diagonal constraint */
class ejm_vis_222_weak : public user::Application {
  public:
    ejm_vis_222_weak()
        : user::Application("ejm_vis_222_weak",
                            "Nonlocal visibility for the EJM with 2x2x2 inflation under {\"A00,B00,C00\",\"A11,B11,C11\",\"\"}",
                            false) {}
    void run() override;
};

/*! \brief Visibility of EJM under \f$2\times2\times2\f$ inflation, using all constraints */
class ejm_vis_222_strong : public user::Application {
  public:
    ejm_vis_222_strong()
        : user::Application("ejm_vis_222_strong",
                            "Nonlocal visibility for the EJM with 2x2x2 inflation under all constraints",
                            false) {}
    void run() override;
};

// 2x2x3 ==========================

/*! \brief Visibility of EJM under \f$2\times2\times3\f$ inflation, using only the main constraint */
class ejm_vis_223_weak : public user::Application {
  public:
    ejm_vis_223_weak()
        : user::Application("ejm_vis_223_weak",
                            "Nonlocal visibility for the EJM with 2x2x3 inflation under main constraint only",
                            false) {}
    void run() override;
};

/*! \brief Visibility of EJM under \f$2\times2\times3\f$ inflation, using all constraints */
class ejm_vis_223_strong : public user::Application {
  public:
    ejm_vis_223_strong()
        : user::Application("ejm_vis_223_strong",
                            "Nonlocal visibility for the EJM with 2x2x3 inflation under all constraints",
                            false) {}
    void run() override;
};

// 2x2x4 ============================

/*! \brief Visibility of EJM under \f$2\times2\times4\f$ inflation, using all constraints of \f$2\times2\times3\f$ inflation */
class ejm_vis_224_weak : public user::Application {
  public:
    ejm_vis_224_weak()
        : user::Application("ejm_vis_224_weak",
                            "Nonlocal visibility for the EJM with 2x2x4 inflation under all 2x2x3 constraints",
                            false) {}
    void run() override;
};

/*! \brief Visibility of EJM under \f$2\times2\times4\f$ inflation, using the good \f$A\f$ type constraint
 * \details This ends up being worse than user::ejm_vis_224_weak. */
class ejm_vis_224_weak_bis : public user::Application {
  public:
    ejm_vis_224_weak_bis()
        : user::Application("ejm_vis_224_weak_bis",
                            "Nonlocal visibility for the EJM with 2x2x4 inflation under 2x2x3 constraints but with another A constraint",
                            false) {}
    void run() override;
};

/*! \brief Visibility of EJM under \f$2\times2\times4\f$ inflation, using only main constraint */
class ejm_vis_224_inter_v1 : public user::Application {
  public:
    ejm_vis_224_inter_v1()
        : user::Application("ejm_vis_224_inter_v1",
                            "Nonlocal visibility for the EJM with 2x2x4 inflation under main constraint",
                            false) {}
    void run() override;
};

/*! \brief Visibility of EJM under \f$2\times2\times4\f$ inflation using main constraint and an A-type constraint
 * \details This is the first inflation problem that we found that proves the EJM to be nonlocal!
 * It gives a tiny bit of resistance to noise. */
class ejm_vis_224_inter_v2_diag_A : public user::Application {
  public:
    ejm_vis_224_inter_v2_diag_A()
        : user::Application("ejm_vis_224_inter_v2_diag_A",
                            "Nonlocal visibility for the EJM with 2x2x4 inflation under main constraint and an A constraint",
                            false) {}
    void run() override;
};

/*! \brief Visibility of EJM under \f$2\times2\times4\f$ inflation using main constraint and a C-type constraint
 * \details This inflation problem gets close to proving the nonlocality of the EJM but not quite there yet. */
class ejm_vis_224_inter_v2_diag_C : public user::Application {
  public:
    ejm_vis_224_inter_v2_diag_C()
        : user::Application("ejm_vis_224_inter_v2_diag_C",
                            "Nonlocal visibility for the EJM with 2x2x4 inflation under main constraint and a C constraint",
                            false) {}
    void run() override;
};

// Nonlocality certificate for EJM

/*! \brief Finds a nonlocality certificate for the EJM and writes it to disk
 * \details This nonlocality certificate is based on the \f$2\times2\times4\f$ inflation problem of
 * user::ejm_vis_224_inter_v2_diag_A. The nonlocality certificate is then read by user::ejm_check_nl_certificate.
 * This takes about a week to run. */
class ejm_find_nl_certificate : public user::Application {
  public:
    ejm_find_nl_certificate()
        : user::Application("ejm_find_nl_certificate",
                            "Finds a nonlocality certificate for the EJM with a 2x2x4 inflation problem",
                            false) {}
    void run() override;
};

/*! \brief Reads the nonlocality certificate for the EJM from disk and checks that it indeed rules out a local model for the EJM
 * \details This nonlocality certificate is based on the \f$2\times2\times4\f$ inflation problem of
 * user::ejm_vis_224_inter_v2_diag_A. The nonlocality certificate is expected to be written to disk by user::ejm_find_nl_certificate. */
class ejm_check_nl_certificate : public user::Application {
  public:
    ejm_check_nl_certificate()
        : user::Application("ejm_check_nl_certificate",
                            "Checks a nonlocality certificate for the EJM with a 2x2x4 inflation problem",
                            false) {}
    void run() override;
};

// SCANS --------------------------------

/*! \brief Tests the compatibility of symmetric distributions with the inflation problem described in \p feas_options
    \param s_denom Determines the fine-graining of the scan, see `user::get_symmetric_distribution()`.
    The number of distributions that will be scanned is given by `(s_denom+1)(s_denom+2)/2`.
    \param run_index e.g., 0 and 1 to split in two the scan
    \param n_runs e.g, 2 to split in two the scan */
void scan_symmetric_distributions(inf::FeasOptions::Ptr const &feas_options,
                                  Index const s_denom,
                                  Index const run_index,
                                  Index const n_runs);

/*! \brief Tests the compatibility of the symmetric distributions with the \f$2\times2\times2\f$ inflation */
class ejm_scan_222 : public user::Application {
  public:
    ejm_scan_222() : user::Application("ejm_scan_222", "", false) {}
    void run() override;
};

//! \cond
class ejm_scan_223_i0 : public user::Application {
  public:
    ejm_scan_223_i0() : user::Application("ejm_scan_223_i0", "", false) {}
    void run() override;
};

class ejm_scan_223_i1 : public user::Application {
  public:
    ejm_scan_223_i1() : user::Application("ejm_scan_223_i1", "", false) {}
    void run() override;
};

class ejm_scan_223_i2 : public user::Application {
  public:
    ejm_scan_223_i2() : user::Application("ejm_scan_223_i2", "", false) {}
    void run() override;
};

class ejm_scan_223_i3 : public user::Application {
  public:
    ejm_scan_223_i3() : user::Application("ejm_scan_223_i3", "", false) {}
    void run() override;
};

class ejm_scan_223_i4 : public user::Application {
  public:
    ejm_scan_223_i4() : user::Application("ejm_scan_223_i4", "", false) {}
    void run() override;
};

class ejm_scan_223_i5 : public user::Application {
  public:
    ejm_scan_223_i5() : user::Application("ejm_scan_223_i5", "", false) {}
    void run() override;
};

class ejm_scan_223_i6 : public user::Application {
  public:
    ejm_scan_223_i6() : user::Application("ejm_scan_223_i6", "", false) {}
    void run() override;
};

class ejm_scan_223_i7 : public user::Application {
  public:
    ejm_scan_223_i7() : user::Application("ejm_scan_223_i7", "", false) {}
    void run() override;
};

class ejm_scan_223_i8 : public user::Application {
  public:
    ejm_scan_223_i8() : user::Application("ejm_scan_223_i8", "", false) {}
    void run() override;
};

class ejm_scan_223_i9 : public user::Application {
  public:
    ejm_scan_223_i9() : user::Application("ejm_scan_223_i9", "", false) {}
    void run() override;
};
//! \endcond

//! @}

} // namespace user
