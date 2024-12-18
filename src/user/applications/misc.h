#pragma once

#include "../../inf/inf_problem/target_distr.h"
#include "../application.h"
/*! \file */

/*! \defgroup misc_tests Miscellaneous tests
    \ingroup applications

    \brief These applications test the basic functionalities of the codebase */

namespace user {

/*! \addtogroup misc_tests
 * @{
 */

/*! \brief This function tests the evaluation of inner products, see inf::DualVector and inf::Marginal
\details It evaluates the inner product between a random inf::DualVector and an inflation marginal,
where the inflation size is also random.
The evaluation is compared using different methods for consistency checks.
\param d This distribution is used to define an inflation problem for which inner products will be evaluated. */
void test_dual_vector_eval(inf::TargetDistr::ConstPtr const &d);

/*! \brief We use this to conveniently print events in our latex source code format */
void print_event_latex_format(inf::Inflation const &inflation, inf::Event const &event);

/*! \brief Tests util::Frac */
class frac : public user::Application {
  public:
    frac() : user::Application("frac", "Tests util::Frac", true) {}
    void run() override;
};

/*! \brief Used to test and display the effect of symmetry transformations on inflation events */
class event_sym : public user::Application {
  public:
    event_sym() : user::Application("event_sym", "Tests symmetry transformations on inflation events", true) {}
    void run() override;
};

/*! \brief Tests inf::FullyCorrectiveFW */
class fully_corrective_fw : public user::Application {
  public:
    fully_corrective_fw() : user::Application("fully_corrective_fw", "Tests inf::FullyCorrectiveFW", true) {}
    void run() override;
};

/*! \brief A detailed test to investigate the behavior of inf::PairwiseFW */
class pairwise_fw : public user::Application {
  public:
    pairwise_fw() : user::Application("pairwise_fw", "Detailed test of inf::PairwiseFW", false) {}
    void run() override;
};

/*! \brief Tests inf::EventTensor */
class event_tensor : public user::Application {
  public:
    event_tensor() : user::Application("event_tensor", "Tests inf::EventTensor", true) {}
    void run() override;
};

/*! \brief Tests inf::EventTree, including IO mecanism */
class event_tree : public user::Application {
  public:
    event_tree() : user::Application("event_tree", "Tests inf::EventTree", true) {}
    void run() override;
};

/*! \brief Tests inf::TreeFiller */
class inf_tree_filler : public user::Application {
  public:
    inf_tree_filler() : user::Application("inf_tree_filler", "Tests inf::TreeFiller", true) {}
    void run() override;
};

/*! \brief Tests marginals of a triangle distribution
 *  \details This is a test because of the tests within inf::TargetDistr (in particular, the marginals are checked to be normalized) */
class distribution_marginal : public user::Application {
  public:
    distribution_marginal() : user::Application("distribution_marginal", "Tests marginals of a triangle distribution", true) {}
    void run() override;
};

/*! \brief Tests inf::Marginal */
class inflation_marginal : public user::Application {
  public:
    inflation_marginal() : user::Application("inflation_marginal", "Tests inf::Marginal", true) {}
    void run() override;
};

/*! \brief Tests the mechanism that stores the bounds of an inf::DualVector */
class dual_vector_bounds : public user::Application {
  public:
    dual_vector_bounds() : user::Application("dual_vector_bounds", "Tests the bounds of an inf::DualVector", true) {}
    void run() override;
};

/*! \brief Tests the mechanism that reads/writes an inf::DualVector to disk */
class dual_vector_io : public user::Application {
  public:
    dual_vector_io() : user::Application("dual_vector_io", "Tests the I/O of an inf::DualVector", true) {}
    void run() override;
};

/*! \brief Tests the mechanism that uses the bounds of an inf::DualVector to speed up the optimizations of inner products. */
class opt_bounds : public user::Application {
  public:
    opt_bounds() : user::Application("opt_bounds", "Tests the use of bounds of an inf::DualVector by an inf::Optimizer", true) {}
    void run() override;
};

/*! \brief To test the time that an inf::Optimizer takes for various options. This one takes more time to execute. */
class opt_time : public user::Application {
  public:
    opt_time() : user::Application("opt_time", "Times an inf::Optimizer", false) {}
    void run() override;
};

/*! \brief Tests the util::FileStream serialization mechanism in both binary and text form for a dummy class */
class file_stream : public user::Application {
  public:
    file_stream() : user::Application("file_stream", "Tests the text and binary serialization mechanism", true) {}
    void run() override;
};

/*! \brief Tests the tree splitting mechanism on a simple case as described in inf::TreeSplitter */
class tree_splitter : public user::Application {
  public:
    tree_splitter() : user::Application("tree_splitter", "Testing inf::TreeSplitter in a simple example", true) {}
    void run() override;
};

/*! \brief Tests the tree splitting of inflation event trees */
class inf_tree_splitting : public user::Application {
  public:
    inf_tree_splitting() : user::Application("inf_tree_splitting", "Looking at how to evenly split the inflation event tree", false) {}
    void run() override;
};

/*! @} */

} // namespace user
