#pragma once

#include "../constraints/marginal.h"
#include "../frank_wolfe/frank_wolfe.h"
#include "../inf_problem/inflation.h"
#include "../inf_problem/target_distr.h"
#include "../optimization/optimizer.h"
#include "feas_options.h"

/*! \file */

namespace inf {

/*! \ingroup infpb
 * \brief This class defines inflation problems, i.e., problems of the form: given a target distribution \f$\targetp\in\targetps\f$, is there an inflation distribution \f$\infq\in\infqs\f$ satisfying appropriate symmetry and marginal constraints?
 * \details Defining an inf::FeasProblem requires passing an inf::TargetDistr as target distribution \f$\targetp\in\targetps\f$ and inf::FeasOptions specifying the inflation size \f$\infsize = (\infsize_0,\infsize_1,\infsize_2)\f$ (see inf::Inflation::Size) and the inflation constraints \f$\constraintlist \in \infconstraints\f$ (see inf::ConstraintSet).
 * Additionally, the inf::FeasOptions specify the methods to use to solve the inflation problem.
 * The inf::FeasProblem is solved using a Frank-Wolfe algorithm, see inf::FrankWolfe.
 *
 * This class allows to write and read nonlocality certificates from text files, see inf::FeasProblem::write_dual_vector_to_file() and inf::FeasProblem::read_dual_vector_from_file()
 * for more details.
 * */
class FeasProblem {
  public:
    typedef std::unique_ptr<inf::FeasProblem> UniquePtr;

    /*! \brief The return status of a feasibility problem */
    enum class Status {
        nonlocal,    ///< The target distribution \f$\targetp\f$ is provably nonlocal using exact arithmetic.
                     /// This conclusion comes from the inf::Optimizer having checked that a nonlocality certificate is valid.
        inconclusive ///< Inconclusive (may or may not be local): up to numerical precision, the target distribution \f$\targetp\f$ appears to be compatible with the inflation problem.
                     /// Going to a larger inflation with more constraints may allow to prove the nonlocality of the distribution \f$\targetp\f$.
                     /// This conclusion comes from the inf::FrankWolfe algorithm being unable to provide a next dual vector that would
                     /// be a candidate nonlocality certificate.

    };

    /*! \brief One option of the inf::FeasProblem is whether or not to retain previously encountered event when updating the target distribution
     * \details This has proven to be quite useful for performance when scanning the feasibility of a family of inf::TargetDistr.
     * The idea is that when the inf::TargetDistr is changes a little bit, the inflation events generating the active set used in the inf::FrankWolfe algorithm are still relevant ones.
     * It is thus recommended to use inf::FeasProblem::RetainEvents::yes when running sequential inf::FeasProblem with inf::TargetDistr that are not too different from each other.
     * It may however be detrimental to use this option when running inf::FeasProblem with "completely different" inf::TargetDistr. */
    enum class RetainEvents {
        yes, ///< This is used to indicate that the inf::FrankWolfe algorithm should restart keeping the previously encountered events.
             /// This helps to quickly rule out some inf::DualVector as being potential separating hyperplanes.
             /// See inf::FrankWolfe::memorize_event_and_quovec().
        no   ///< Do not keep the previously encountered events when restarting the inf::FrankWolfe algorithm.
    };

    static void log(RetainEvents retain_events);

    /*! \brief Constructs an inflation feasibility problem
        \param distribution The target distribution \f$\targetp\in\targetps\f$
        \param options This specifies in particular the inflation size \f$\infsize = (\infsize_0,\infsize_1,\infsize_2)\f$ and the inflation constraints */
    FeasProblem(inf::TargetDistr::ConstPtr const &distribution,
                inf::FeasOptions::ConstPtr const &options);
    //! \cond
    FeasProblem(inf::FeasProblem const &other) = delete;
    FeasProblem(inf::FeasProblem &&other) = delete;
    inf::FeasProblem &operator=(inf::FeasProblem const &other) = delete;
    inf::FeasProblem &operator=(inf::FeasProblem &&other) = delete;
    //! \endcond

    /*! \brief This logs information about the time spent on the inflation problem and the number of events stored
     * \details This is used by inf::VisProblem.
     * It logs in particular the inf::Optimizer::total_optimization_chrono, the inf::FrankWolfe::total_fw_chrono and the total time util::Chrono::process_clock. */
    void log_status_bar() const;

    /*! \brief This is the heavy function that checks feasibility of a target distribution
     * \details This checks the compatibility of the inf::TargetDistr \f$\targetp\in\targetps\f$ that was passed to the constructor or last passed to inf::FeasProblem::update_target_distribution(), if applicable.
     * The way the algorithm works is by successively invoking inf::FrankWolfe::time_and_solve() to find a next inf::DualVector \f$\quovec = \{\quovec_\constraintname\}_{\constraintname\in\constraintlist} \in \totquovecspace\f$ that may be a potential separating hyperplane, and then to minimize the inner product \f$\inner{\quovec}{\totconstraintmap(\detdistr\infevent)}_{\constraintlist}\f$ over all \f$\infevent\in\infevents\f$ (actually, it is sufficient to only look at the symmetric inflation events obtained with inf::Inflation::get_symtree()) using inf::Optimizer::optimize().
     * If the algorithm finds a dual vector \f$\quovec\f$ such that \f$\inner{\quovec}{\totconstraintmap(\detdistr\infevent)}_{\constraintlist} > 0\f$ for all \f$\infevent\f$, then this proves the nonlocality of the distribution \f$\targetp\f$, as shown in the paper.
     * \return The feasibility status, see inf::FeasProblem::Status for more information */
    inf::FeasProblem::Status get_feasibility();

    /*! \brief This allows to change the target distribution \f$\targetp\in\targetps\f$ without recreating an inf::FeasProblem from nothing
     * \details This methods throws an error if \p d has different symmetries compared to the initial distribution, see inf::Inflation::has_symmetries_compatible_with().
     * In this case, one should re-create a new inf::FeasProblem entirely.
     * In the future, this may be automatized from within the method if it is common to have families of distributions with different symmetries.
     * \param d The new distribution \f$\targetp\in\targetps\f$ to use in the next call of inf::FeasProblem::get_feasibility().
     * \param retain_events This controls how the inf::FrankWolfe algorithm held in `m_frank_wolfe` is reset, see inf::FeasProblem::RetainEvents.
     * \warning This method resets the internal dual vectors to all zeros. */
    void update_target_distribution(inf::TargetDistr::ConstPtr const &d,
                                    inf::FeasProblem::RetainEvents retain_events);

    /*! \brief This saves the current dual vector held by inf::ConstraintSet to a text file.
     * \param filename The filename (without extension) to save the dual vector to
     * \param metadata An arbitrary string to give a little context in the file, e.g., a text description of which nonlocality problem was considered when obtaining this dual vector.
     * \details The idea is that one can run inf::FeasProblem::get_feasibility(), and then, if the resulting distribution is nonlocal, one can call inf::FeasProblem::write_dual_vector_to_file()
     * to store the corresponding nonlocality certificate for later re-use.
     *
     * This method can't be const, because of the util::Serializable and util::FileStream interface that does not allow (by design!) to
     * output const objects, see util::OutputFileStream.
     * */
    void write_dual_vector_to_file(std::string const &filename, std::string const &metadata);

    /*! \brief This reads the current dual vector held by inf::ConstraintSet from a text file
     * \param filename The filename (without extension) to read the dual vector from
     * \param metadata This string needs to match the one passed to inf::FeasProblem::save_dual_vector_to_file()
     *
     * \details This method can be called on an inf::FeasProblem to bypass the need to call inf::FeasProblem::get_feasibility().
     * Instead, the user can just call inf::FeasProblem::read_dual_vector_from_file(), then inf::FeasProblem::minimize_dual_vector(), and if the resulting
     * inf::Optimizer::Solution has `inf::Optimizer::Solution::get_inflation_event_score() > 0`, then this proves the nonlocality of the target distribution. */
    void read_dual_vector_from_file(std::string const &filename, std::string const &metadata);

    /*! \brief This minimizes the inner product between the current dual vector stored in `m_constraint_set`, see inf::Optimizer
     * \details If the returned inf::Optimizer::Solution has `inf::Optimizer::Solution::get_inflation_event_score() > 0`, then this proves the nonlocality of the target distribution. */
    inf::Optimizer::Solution minimize_dual_vector() const;

    /*! \brief This call inf::FeasProblem::read_dual_vector_from_file(), then inf::FeasProblem::minimize_dual_vector(),
     * logs the resulting inf::Optimizer::Solution and concludes about whether or not the inf::TargetDistr is nonlocal */
    inf::FeasProblem::Status read_and_check_dual_vector(std::string const &filename, std::string const &metadata);

  private:
    /*! \brief The target distribution \f$\targetp\in\targetps\f$ */
    inf::TargetDistr::ConstPtr m_distribution;
    /*! \brief The structure holding all the inflation parameters, see inf::FeasOptions */
    inf::FeasOptions::ConstPtr m_options;
    /*! \brief The set \f$\constraintlist\subset\infconstraints\f$ of inflation constraints */
    inf::ConstraintSet::Ptr m_constraint_set;
    /*! \brief The inf::FrankWolfe algorithm that is in charge of finding an inf::DualVector that certifies nonlocality of the target distribution `m_distribution`.
     * \details This is a polymorphic pointer that may point to various Frank-Wolfe algorithms, see inf::FrankWolfe::Algo. */
    inf::FrankWolfe::UniquePtr m_frank_wolfe;
    /*! \brief The inf::Optimizer in charge of minimizing the inner product of an inf::DualVector proposed by `m_frank_wolfe` with all extremal inflation columns/quovecs \f$\{\totconstraintmapelem(\detdistr\infevent)\}\f$ where \f$\infevent\in\infevents\f$
     * \details This is a polymorphic pointer that may point to various optimization algorithms, see inf::Optimizer::SearchMode. */
    inf::Optimizer::Ptr m_optimizer;
    /*! \brief This indicates the number of optimizations (calls to inf::Optimizer::optimize(), or equivalently, calls to inf::FrankWolfe::solve()) that have been ran since the last call to inf::FeasProblem::get_feasibility() */
    Index m_n_iterations;

    /*! \brief This logs info about the options, pretty prints the constraints, and initializes the inf::Optimizer
     * \details The reason for initializing the inf::Optimizer in this method is that it may be quite slow to initialize the inf::Optimizer if a call to inf::Inflation::get_symtree() is required.
     * It is thus nice to first print the problem that is being solved, so that the user has a chance to realize that the problem being solved is not the one they wanted to solve *before* having to wait for the inf::Optimizer initialization. */
    void log_info_and_init_optimizer();

    /*! \brief To log information about the feasibility status and the inflation problem that it corresponds to
     * \details This method is not static because it also prints information about the problem, i.e., the target distribution & inflation. */
    void log_feasibility(inf::FeasProblem::Status feas_status) const;

    /*! \brief This forwards both the inflation event \p event (some \f$\infevent\in\infevents\f$) as well as the associated
     * quovec \f$\totconstraintmap(\detdistr\infevent)\f$ to the inf::FrankWolfe algorithm held by `m_frank_wolfe`
     * \sa inf::FrankWolfe::memorize_event_and_quovec() */
    void memorize_event(inf::Event const &event);

    /*! \brief This calls inf::FeasProblem::memorize_event() with the all-zero inflation event */
    void init_frank_wolfe();

    /*! \brief This takes as input a floating-point representation of a quovec \f$\quovec = \{\quovec_\constraintname\}_{\constraintname\in\constraintlist} \in \totquovecspace\f$ and returns a scaled-up integer quovec
     * \details In more details, this method scales up the input quovec as much as possible by multiplying it with a positive constant,
     * then floors each entry to obtain an integer quovec, and finally divide each entry by the GCD of all entries.
     * This allows to the compute inner products using integer arithmetic.
     * Implicitly, this uses the fact that multiplying or dividing a quovec by a positive constant does not change the result of a minimization of \f$\inner{\quovec}{\totconstraintmap(\detdistr\infevent)}_{\constraintlist}\f$ over \f$\infevent\f$, and nor does it change whether or not a dual vector \f$\quovec\f$ is or isn't a nonlocality certificate (since this relies on \f$\inner{\quovec}{\totconstraintmap(\detdistr\infevent)}_{\constraintlist}\f$ being positive for all \f$\infevent\f$).
     * \param dual_vector_double A floating-point representation of a dual vector/quovec \f$\quovec = \{\quovec_\constraintname\}_{\constraintlist}\f$
     * \return A scale-and-rounded representation of \p dual_vector_double */
    inf::Quovec round_dual_vector(std::vector<double> const &dual_vector_double) const;
};

} // namespace inf
