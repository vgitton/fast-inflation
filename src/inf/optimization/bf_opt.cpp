#include "bf_opt.h"
#include "../../util/logger.h"

inf::BruteForceOpt::BruteForceOpt(inf::ConstraintSet::Ptr const &constraints)
    : inf::Optimizer(constraints),
      m_evaluators(m_constraints->get_marg_evaluators()) {}

inf::Optimizer::PreSolution inf::BruteForceOpt::get_pre_solution() {
    LOG_BEGIN_SECTION("inf::BruteForceOpt optimization")

    Num min_numerator = 0;
    bool min_num_uninit = true;
    inf::Event best_event;

    util::logger << "Optimizing... " << util::flush;

    for (inf::Event const &e : m_constraints->get_inflation()->get_event_range()) {
        for (Index i : util::Range(e.size()))
            m_evaluators.set_outcome(i, e[i]);

        Num const numerator = m_evaluators.evaluate_dual_vector();

        if (min_num_uninit or numerator < min_numerator) {
            min_num_uninit = false;
            min_numerator = numerator;
            best_event = e;

            if (m_stop_mode == inf::Optimizer::StopMode::sat and min_numerator <= 0)
                break;
        }
    }

    LOG_END_SECTION

    return inf::Optimizer::PreSolution(min_numerator,
                                       best_event);
}
