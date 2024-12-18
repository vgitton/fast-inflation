#pragma once

#include "../../inf/constraints/dual_vector.h"
#include "../../inf/constraints/marginal.h"
#include "../../inf/inf_problem/inflation.h"
#include "../../inf/inf_problem/network.h"
#include "../../inf/inf_problem/target_distr.h"
#include "../application.h"

/*! \file */

/*! \defgroup SRB Shared Random Bit
    \ingroup applications
    \brief Studying the nonlocality of the noisy shared random bit distribution
    \details The noisy shared random bit (SRB) distribution is defined as
    \f[
    p = \frac{\mathtt{visibility}}{\mathtt{denom}}\frac{[000] + [111]}{2}
    + \left(1 - \frac{\mathtt{visibility}}{\mathtt{denom}}\right)\frac{[000] + \cdots + [111]}{8},
    \f]
    where \f$\mathtt{visibility} \in \{0,\dots,\mathtt{denom}\}\f$.
    This is a natural distribution to study in the triangle network, since the case of \f$\mathtt{visibility}/\mathtt{denom} = 100\%\f$ corresponds to a noiseless SRB
    which is easily shown to be causally incompatible with the triangle network, in all classical, quantum and even post-quantum theories.
    We can then study the nonlocal visibility of the SRB, i.e., the smallest value of \f$\mathtt{visibility}/\mathtt{denom}\f$ such that the resulting noisy SRB distribution \f$p\f$ is nonlocal in the triangle network.
    As shown in the paper, the noisy SRB family of distributions satisfies the assumptions described in inf::VisProblem, so that we can use a dichotomic search to find this minimal nonlocality.

    This is a useful case study, because it allows us to compare and validate our results to those of the article "Post-quantum nonlocality in the minimal triangle scenario" by Alejandro Pozas-Kerstjens, Antoine Girardin, Tamás Kriváchy, Armin Tavakoli, Nicolas Gisin, published in New J. Phys. 25, 113037 (2023).
    See [https://www.arxiv.org/abs/2305.03745](https://www.arxiv.org/abs/2305.03745) and the accompanying code at [https://github.com/apozas/minimal-triangle-nonlocality](https://github.com/apozas/minimal-triangle-nonlocality).
*/

/*! \brief The namespace for Shared Random Bit visibility related tasks */
namespace user {

/*! \addtogroup SRB
 * @{
 */

/*! \brief Triangle network with 2 outcomes */
inf::Network::ConstPtr get_srb_network();

/*! \brief Noisy shared random bit distribution
    \param visibility Integer between 0 and `visibility_denom` included.
    0 means pure noise, while the maximum value means no noise.
    \param visibility_denom Ought to be a positive integer */
inf::TargetDistr::ConstPtr get_noisy_srb(Num visibility, Num visibility_denom);

/*! \brief user::Application that shows info about the problem and its symmetries for various inflation sizes */
class srb_sym : public user::Application {
  public:
    srb_sym() : user::Application("srb_sym", "Symmetries of the Shared Random Bit", true) {}
    void run() override;
};

/*! \brief See user::test_dual_vector_eval() */
class srb_eval : public user::Application {
  public:
    srb_eval() : user::Application("srb_eval", "Dual vector evaluation for the Shared Random Bit", true) {}
    void run() override;
};

/*! \brief Visibility of SRB under \f$2\times2\times2\f$ inflation, using only the order-2 diagonal constraint */
class srb_vis_222_weak : public user::Application {
  public:
    srb_vis_222_weak() : user::Application("srb_vis_222_weak", "Nonlocal visibility of the Shared Random Bit for the 2x2x2 inflation under {\"A00,B00,C00\",\"A11,B11,C11\",\"\"}", true) {}
    void run() override;
};

/*! \brief Visibility of SRB under \f$2\times2\times2\f$ inflation, with all constraints */
class srb_vis_222_strong : public user::Application {
  public:
    srb_vis_222_strong() : user::Application("srb_vis_222_strong", "Nonlocal visibility of the Shared Random Bit for the 2x2x2 inflation using all constraints", true) {}
    void run() override;
};

/*! \brief Writes a nonlocality certificate for the SRB and then reads it back to test its validity */
class srb_dual_vector_io : public user::Application {
  public:
    srb_dual_vector_io() : user::Application("srb_dual_vector_io", "Testing disk I/O of nonlocality certificates for the Shared Random Bit with the 2x2x2 inflation", true) {}
    void run() override;
};

/*! \brief Visibility of SRB under a variety of inflation sizes and solver options */
class srb_vis_detailed_test : public user::Application {
  public:
    srb_vis_detailed_test() : user::Application("srb_vis_detailed_test", "(slow) Nonlocal visibility of the Shared Random Bit for various inflation sizes and solver options", false) {}
    void run() override;
};

/*! \brief Visibility of SRB under \f$2\times2\times3\f$ inflation, using only the order-2 diagonal constraint */
class srb_vis_223_weak : public user::Application {
  public:
    srb_vis_223_weak() : user::Application("srb_vis_223_weak", "Nonlocal visibility of the Shared Random Bit for the 2x2x3 inflation under {\"A00,B00,C00\",\"A11,B11,C11\",\"\"}", true) {}
    void run() override;
};

/*! \brief Visibility of SRB under \f$2\times2\times3\f$ inflation, with all constraints */
class srb_vis_223_strong : public user::Application {
  public:
    srb_vis_223_strong() : user::Application("srb_vis_223_strong", "Nonlocal visibility of the Shared Random Bit for the 2x2x3 inflation using all constraints", false) {}
    void run() override;
};

/*! \brief Visibility of SRB under \f$2\times3\times3\f$ inflation, using only the order-2 diagonal constraint */
class srb_vis_233_weak : public user::Application {
  public:
    srb_vis_233_weak() : user::Application("srb_vis_233_weak", "Nonlocal visibility of the Shared Random Bit for the 2x3x3 inflation under {\"A00,B00,C00\",\"A11,B11,C11\",\"\"}", false) {}
    void run() override;
};

/*! \brief Visibility of SRB under \f$2\times3\times3\f$ inflation, using only the full diagonal constraint */
class srb_vis_233_inter : public user::Application {
  public:
    srb_vis_233_inter() : user::Application("srb_vis_233_inter", "Nonlocal visibility of the Shared Random Bit for the 2x3x3 inflation under {\"A00,B00,C00\",\"A11,B11,C11\",\"A22\",\"\"}", false) {}
    void run() override;
};

/*! \brief Visibility of SRB under \f$2\times3\times3\f$ inflation, with all constraints */
class srb_vis_233_strong : public user::Application {
  public:
    srb_vis_233_strong() : user::Application("srb_vis_233_strong", "Nonlocal visibility of the Shared Random Bit for the 2x3x3 inflation using all constraints", false) {}
    void run() override;
};

/*! \brief Visibility of SRB under \f$3\times3\times3\f$ inflation, using only main constraint
 * \details This one takes about 5min to run */
class srb_vis_333 : public user::Application {
  public:
    srb_vis_333() : user::Application("srb_vis_333", "Nonlocal visibility of the Shared Random Bit for the 3x3x3 inflation", false) {}
    void run() override;
};

/*! \brief Visibility of SRB under \f$3\times3\times4\f$ inflation, using only main constraint of \f$3\times3\times3\f$ inflation
 * \details This one takes about 45min to run */
class srb_vis_334 : public user::Application {
  public:
    srb_vis_334() : user::Application("srb_vis_334", "Nonlocal visibility of the Shared Random Bit for the 3x3x4 inflation", false) {}
    void run() override;
};

/*! \brief Placeholder for Shared Random Bit feasibility problems */
class srb : public user::Application {
  public:
    srb() : user::Application("srb", "Placeholder for Shared Random Bit feasibility problems", false) {}
    void run() override;
};

//! @}

} // namespace user
