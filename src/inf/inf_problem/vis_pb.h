#pragma once

#include "feas_pb.h"

/*! \file */

namespace inf {

/*! \ingroup infpb
    \brief A simple dichotomic search for finding the minimum infeasible visibility of a distribution
    \details
    The class inf::VisProblem may be used for any family of distribution \f$\{p_v\}_{v = v_\text{min}}^{v_\text{max}}\f$, indexed by an integer parameter \f$v \in \mathcal V\f$.
    However, the dichotomic search that it performs only really make sense if the inflation relaxation will partition the set \f$\mathcal{V}\f$ into \f$\{v_\text{min},\cdots,v_0,v_1,\cdots,v_\text{max}\}\f$ such that
    - For all \f$v \leq v_0\f$, \f$p_v\f$ is compatible with the target inflation (for instance, \f$p_v\f$ is local),
    - For all \f$v \geq v_1\f$, \f$p_v\f$ is incompatible with the target inflation (so that \f$p_v\f$ is nonlocal).

    In the paper, we describe a general class of noise models that generate families of distributions that satisfy these assumptions.
     */
class VisProblem {
  public:
    /*! \brief This returns a string of the form `visibility / visbility_denom = (floating point value)` */
    static std::string visibility_to_str(Num visibility, Num visibility_denom);

    /*! \param get_distribution A function pointer providing a way to compute
        the distribution for each visibility value (which is input to the function pointer)
        \param min_visibility The minimum value \f$v_\text{min}\f$ to be tested
        \param max_visibility The maximum value \f$v_\text{max}\f$ to be tested
        \param visibility_denom The tested values are typically considered to be numerators, so \p visibility_denom provides the denominator (passed to the function pointers \p get_distribution and \p visibility_to_str )
        \param feas_problem_options Defines the type of feasibility problem that will be tested
        \param retain_events If `yes`, the inflation events encountered by one inf::FeasProblem
        are forwarded to the next one. This gives a useful speedup. May want to disable this mechanism:
        it induces a larger linear program, and might hence generate some instability. See inf::FeasProblem::RetainEvents. */
    VisProblem(inf::TargetDistr::ConstPtr (*get_distribution)(Num, Num),
               Num min_visibility,
               Num max_visibility,
               Num visibility_denom,
               inf::FeasOptions::ConstPtr const &feas_problem_options,
               inf::FeasProblem::RetainEvents retain_events);
    //! \cond ignore deleted
    VisProblem(inf::VisProblem const &other) = delete;
    VisProblem(inf::VisProblem &&other) = delete;
    VisProblem &operator=(inf::VisProblem const &other) = delete;
    VisProblem &operator=(inf::VisProblem &&other) = delete;
    //! \endcond

    /*! \brief Get the minimum visibility \f$v_1\f$ as described in inf::VisProblem
     * \details Recall that \f$v_1\f$ is the smallest value such that the associated inf::TargetDistr is provably nonlocal
     * based on the specified inflation problem.
     *
     * If even `max_visibility` is not ruled out, `max_vibility + 1` is returned, which might be completely unphysical,
     * but signals what happened. */
    Num get_minimum_nonlocal_visibility();

  private:
    /*! \brief To sample the inf::TargetDistr \f$ p_v \f$
     * \details The first argument is the visibility, over which the dichotomic search is ran,
     * and the second is the "denominator" (it can be used arbitrarily, but is fixed accross the whole inf::VisProblem) */
    inf::TargetDistr::ConstPtr (*const m_get_distribution)(Num, Num);
    /*! \brief \f$v_\text{min}\f$ (included in the search) */
    Num const m_min_visibility;
    /*! \brief \f$v_\text{max}\f$ (included in the search) */
    Num const m_max_visibility;
    /*! \brief For convenience: typically, visibilities are rationals, and this is their common denominator */
    Num const m_visibility_denom;
    /*! \brief A pointer to an inf::FeasProblem to allow delayed initialization */
    inf::FeasProblem::UniquePtr m_feas_problem;
    /*! \brief The inf::FeasProblem parameters (what the inflation problem is, and how it should run) */
    inf::FeasOptions::ConstPtr const m_feas_problem_options;
    /*! \brief Whether or not to keep the previously encountered events in inf::FeasProblem */
    inf::FeasProblem::RetainEvents const m_retain_events;

    /*! \brief Constructs the target distribution based on \p visibility and tests for inflation compatibiltiy
        \param visibility The visibility used to construct the target distribution \f$p_v\f$
        \return `true` if \f$p_v\f$ is compatible with the target inflation */
    bool visibility_is_feasible(Num visibility);
};

} // namespace inf
