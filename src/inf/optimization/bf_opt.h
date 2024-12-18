#pragma once

#include "../../util/chrono.h"
#include "optimizer.h"

/*! \file */

namespace inf {

/*! \ingroup opt
 * \brief This inf::Optimizer simply enumerate all inflation events to perform the minimization
 * \details This class minimizes an inf::DualVector, representing some \f$\quovec = \{\quovec_\constraintname\}_{\constraintname\in\constraintlist}\in\totquovecspace\f$, on the events of some inf::Inflation, i.e., solving the minimization problem
 * \f[
 *     \min_{\infevent\in\infevents} \inner{\quovec}{\totconstraintmap(\detdistr\infevent)}_{\constraintlist},
 * \f]
 * where \f$\constraintlist \subset \infconstraints\f$ describes the constraints of an inflation problem (see inf::ConstraintSet).
 *
 * The minimization is simply performed by enumerating all \f$\infevent\in\infevents\f$, in particular, ignoring the reduced set of events \f$\redinfevents\f$ that uses symmetry. */
class BruteForceOpt : public inf::Optimizer {
  public:
    /*! \brief Initializes an inf::BruteForceOpt with the relevant \p constraints
        \param constraints An optimizer gets access to an inf::ConstraintSet, which is typically used outside of the optimizer.
        This parameter allows to forward the inf::Inflation at hand, but also the current value of the inf::DualVector to optimize
        through inf::ConstraintSet::get_marg_evaluators(). */
    BruteForceOpt(inf::ConstraintSet::Ptr const &constraints);

    inf::Optimizer::PreSolution get_pre_solution() override;

  private:
    /*! \brief These inf::Marginal::Evaluator are obtained with inf::ConstraintSet::get_marg_evaluators() */
    inf::Marginal::EvaluatorSet m_evaluators;
};

} // namespace inf
