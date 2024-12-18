#include "optimizer.h"
#include "../symmetry/symmetry.h"
// Add other optimizers here
#include "bf_opt.h"
#include "tree_opt.h"

#include "../../util/logger.h"

util::Chrono inf::Optimizer::total_optimization_chrono(util::Chrono::State::paused);

void inf::Optimizer::log(inf::Optimizer::StopMode stop_mode) {
    util::logger << util::begin_comment << "inf::Optimizer::StopMode::"
                 << util::end_comment;
    switch (stop_mode) {
    case inf::Optimizer::StopMode::sat:
        util::logger << "sat";
        break;
    case inf::Optimizer::StopMode::opt:
        util::logger << "opt";
        break;
    default:
        THROW_ERROR("switch")
    }
}

inf::Optimizer::PreSolution::PreSolution(Num inflation_event_score,
                                         inf::Event const &inflation_event)
    : inflation_event_score(inflation_event_score),
      inflation_event(inflation_event) {}

inf::Optimizer::Solution::Solution(inf::Optimizer::PreSolution const &pre_sol,
                                   inf::Inflation::ConstPtr const &inflation,
                                   inf::Optimizer::StopMode stop_mode)
    : m_pre_sol(pre_sol),
      m_inflation(inflation),
      m_stop_mode(stop_mode) {}

Num inf::Optimizer::Solution::get_inflation_event_score() const {
    return m_pre_sol.inflation_event_score;
}

inf::Event const &inf::Optimizer::Solution::get_inflation_event() const {
    return m_pre_sol.inflation_event;
}

void inf::Optimizer::Solution::log() const {
    LOG_BEGIN_SECTION("inf::Optimizer::Solution")
    util::logger << "The inflation event " << util::cr;
    m_inflation->log_event(get_inflation_event());
    util::logger << "scored " << get_inflation_event_score()
                 << util::begin_comment << " (" << util::end_comment;
    inf::Optimizer::log(m_stop_mode);
    util::logger << util::begin_comment << ")" << util::end_comment
                 << "                         " << util::cr;
    LOG_END_SECTION
}

void inf::Optimizer::log(inf::Optimizer::SearchMode search_mode) {
    util::logger << util::begin_comment << "inf::Optimizer::SearchMode::"
                 << util::end_comment;
    switch (search_mode) {
    case inf::Optimizer::SearchMode::brute_force:
        util::logger << "brute_force";
        break;
    case inf::Optimizer::SearchMode::tree_search:
        util::logger << "tree_search";
        break;
    default:
        THROW_ERROR("switch")
    }
}

inf::Optimizer::Ptr inf::Optimizer::get_optimizer(inf::Optimizer::SearchMode search_mode,
                                                  inf::ConstraintSet::Ptr const &constraints,
                                                  inf::EventTree::IO symtree_io,
                                                  Index n_threads) {
    switch (search_mode) {
    case inf::Optimizer::SearchMode::brute_force:
        return std::make_shared<inf::BruteForceOpt>(constraints);
    case inf::Optimizer::SearchMode::tree_search:
        return std::make_shared<inf::TreeOpt>(constraints, symtree_io, n_threads);
    default:
        THROW_ERROR("Unsupported inf::Optimizer::SearchMode")
    }
}

inf::Optimizer::Optimizer(inf::ConstraintSet::Ptr const &constraints)
    : m_constraints(constraints),
      m_stop_mode(inf::Optimizer::StopMode::sat) // will be modified on call to optimize()
{
    inf::Optimizer::total_optimization_chrono.reset();
}

inf::Optimizer::Solution inf::Optimizer::optimize(inf::Optimizer::StopMode stop_mode) {
    m_stop_mode = stop_mode;

    inf::Optimizer::total_optimization_chrono.start();
    inf::Optimizer::PreSolution pre_sol = this->get_pre_solution();
    inf::Optimizer::total_optimization_chrono.pause();

    return inf::Optimizer::Solution(pre_sol,
                                    m_constraints->get_inflation(),
                                    m_stop_mode);
}
