#pragma once

#include "../../util/loggable.h"
#include "../constraints/dual_vector.h"
#include "../frank_wolfe/frank_wolfe.h"
#include "../optimization/optimizer.h"

namespace inf {

/*! \ingroup infpb
 * \brief To conviently manipulate the parameters of an inf::FeasProblem
 * \details
 * The fact that we use a specific type for each option allows us the following unambiguous syntax when using this class:
 * \code{.cpp}
 * inf::FeasOptions options;
 * options.set(inf::Inflation::Size({2,2,2}))
 *     .set(inf::Optimizer::SearchMode::tree_search)
 *     .set(inf::ConstraintSet::Description({"..."}));
 * \endcode
 * This class is not documented, please refer to the documenation of each type (e.g., inf::ConstraintSet::Description)
 * for a description of what the parameter controls and the possible values that it can take.
 *
 * Note that each user::Application gets access to default inf::FeasOptions with user::Application::get_feas_options().
 * These have their number of threads and inf::EventTree::IO initialized with the command-line arguments, see
 * the \ref CLIparams "command-line parameter section".
 * */
class FeasOptions : public util::Loggable {
  public:
    typedef std::shared_ptr<const inf::FeasOptions> ConstPtr;
    typedef std::shared_ptr<inf::FeasOptions> Ptr;

    /*! \brief This constructor initializes the parameters to the most sensible values */
    FeasOptions();
    //! \cond
    FeasOptions(FeasOptions const &other) = delete;
    FeasOptions(FeasOptions &&other) = delete;
    FeasOptions &operator=(FeasOptions const &other) = delete;
    FeasOptions &operator=(FeasOptions &&other) = delete;
    //! \endcond

    void log() const override;

    // setters

    FeasOptions &set(inf::Inflation::Size const &inflation_size);
    FeasOptions &set(inf::ConstraintSet::Description const &constraint_set_description);
    FeasOptions &set(inf::Optimizer::SearchMode search_mode);
    FeasOptions &set(inf::Inflation::UseDistrSymmetries use_distr_symmetries);
    FeasOptions &set(inf::Optimizer::StopMode stop_mode);
    FeasOptions &set(inf::FrankWolfe::Algo fw_algo);
    FeasOptions &set(inf::DualVector::StoreBounds store_bounds);
    FeasOptions &set_n_threads(Index n_threads);
    FeasOptions &set(inf::EventTree::IO symtree_io);

    // getters

    inf::Inflation::Size get_inflation_size() const;
    inf::ConstraintSet::Description const &get_constraint_set_description() const;
    inf::Optimizer::SearchMode get_search_mode() const;
    inf::Inflation::UseDistrSymmetries get_use_distr_symmetries() const;
    inf::Optimizer::StopMode get_stop_mode() const;
    inf::FrankWolfe::Algo get_fw_algo() const;
    inf::DualVector::StoreBounds get_store_bounds() const;
    Index get_n_threads() const;
    Index get_vis_param() const;
    inf::EventTree::IO get_symtree_io() const;

  private:
    inf::Inflation::Size m_inflation_size;
    inf::ConstraintSet::Description m_constraint_set_description;

    // FeasOptions

    inf::Optimizer::SearchMode m_search_mode;
    inf::Inflation::UseDistrSymmetries m_use_distr_symmetries;
    inf::Optimizer::StopMode m_stop_mode;
    inf::FrankWolfe::Algo m_fw_algo;
    inf::DualVector::StoreBounds m_store_bounds;
    Index m_n_threads;
    inf::EventTree::IO m_symtree_io;
};

} // namespace inf
