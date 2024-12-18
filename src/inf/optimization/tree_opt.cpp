#include "tree_opt.h"
#include "../../util/logger.h"
#include "../../util/misc.h"
#include "../events/tree_splitter.h"
#include <future>
#include <thread>

inf::TreeOpt::GlobalMinimum::GlobalMinimum()
    : m_global_minimum(std::numeric_limits<Num>::max()) {}

void inf::TreeOpt::GlobalMinimum::set_if_smaller(Num score) {
    // Here before attempting to write, we could check if we need to, but ok

    // A std::unique_lock since only one thread can write to the global minimum at the same time
    std::unique_lock<std::shared_mutex> lock(m_shared_mutex);
    if (score < m_global_minimum)
        m_global_minimum = score;
}

Num inf::TreeOpt::GlobalMinimum::get() const {
    // A std::shared_lock since multiple threads can read the global minimum at the same time
    std::shared_lock<std::shared_mutex> lock(m_shared_mutex);
    return m_global_minimum;
}

void inf::TreeOpt::GlobalMinimum::reset() {
    std::unique_lock<std::shared_mutex> lock(m_shared_mutex);
    m_global_minimum = std::numeric_limits<Num>::max();
}

inf::TreeOpt::ThreadWorker::ThreadWorker(inf::Marginal::EvaluatorSet const &marg_evaluators)
    : marg_evaluators(marg_evaluators),
      current_minimum(std::numeric_limits<Num>::max()),
      current_best_event{},
      last_depth_processed(0),
      queue{},
      n_leaves_effective(0) {}

inf::TreeOpt::ThreadReturn::ThreadReturn(inf::Optimizer::PreSolution &&pre_solution,
                                         Index const n_leaves_effective)
    : pre_solution(pre_solution),
      n_leaves_effective(n_leaves_effective) {}

inf::TreeOpt::TreeOpt(inf::ConstraintSet::Ptr const &constraints,
                      inf::EventTree::IO symtree_io,
                      Index n_threads)
    : inf::Optimizer(constraints),

      // Initialized below
      m_event_tree(nullptr),

      m_symtree_io(symtree_io),
      m_marg_evaluators(m_constraints->get_marg_evaluators()),
      m_n_threads(n_threads),

      // Initialized below
      m_path_partition(nullptr),

      m_store_bounds(m_constraints->get_store_bounds() == inf::DualVector::StoreBounds::yes),
      m_inflation_n_parties(m_constraints->get_inflation()->get_n_parties()),
      m_outcome_unknown(m_constraints->get_inflation()->get_network()->get_outcome_unknown()),

      m_n_leaves_effective(0),

      m_global_minimum() {

    util::logger << "Constructing inf::TreeOpt." << util::cr;

    m_event_tree = &m_constraints->get_inflation()->get_symtree(m_symtree_io);
    // NB: the path partition is trivial for one thread. This is implemented in inf::TreeSplitter.
    m_path_partition = inf::TreeSplitter::get_path_partition(*m_event_tree, m_n_threads);
}

inf::Optimizer::PreSolution inf::TreeOpt::get_pre_solution() {
    m_n_leaves_effective = 0;
    m_global_minimum.reset();

    if (m_n_threads == 1) {
        inf::TreeOpt::ThreadReturn thread_ret = thread_opt((*m_path_partition)[0]);

        m_n_leaves_effective = thread_ret.n_leaves_effective;

        return thread_ret.pre_solution;
    }

    std::vector<std::future<inf::TreeOpt::ThreadReturn>> futures;
    futures.reserve(m_n_threads);

    for (Index const thread_index : util::Range(m_n_threads)) {
        futures.push_back(std::async(std::launch::async,
                                     &inf::TreeOpt::thread_opt,
                                     this,
                                     (*m_path_partition)[thread_index]));
    }

    std::unique_ptr<inf::Optimizer::PreSolution> global_sol = nullptr;

    for (Index const thread_index : util::Range(m_n_threads)) {

        inf::TreeOpt::ThreadReturn const thread_ret = futures[thread_index].get();

        m_n_leaves_effective += thread_ret.n_leaves_effective;

        Num const thread_score = thread_ret.pre_solution.inflation_event_score;

        if (global_sol == nullptr or thread_score < global_sol->inflation_event_score)
            global_sol = std::make_unique<inf::Optimizer::PreSolution>(thread_ret.pre_solution);
    }

    HARD_ASSERT_EQUAL(global_sol->inflation_event_score, m_global_minimum.get())

    return *global_sol;
}

void inf::TreeOpt::log_info() const {
    util::logger << "The symmetrized events are stored as:" << util::cr;
    m_event_tree->log_info();
}

void inf::TreeOpt::log_status() const {
    if (m_n_leaves_effective > 0)
        util::logger << "n_leaves = " << m_n_leaves_effective << ", ";
}

inf::TreeOpt::ThreadReturn inf::TreeOpt::thread_opt(std::vector<inf::TreeSplitter::Path> const &paths) {
    // Initialization of the worker. Don't forget to reset it if we reuse them in the future!
    inf::TreeOpt::ThreadWorker thread_worker(m_marg_evaluators);

    for (inf::TreeSplitter::Path const &path : paths) {
        Index const end_of_path = path.size() - 1;

        for (Index const depth : util::Range(m_inflation_n_parties)) {

            if (depth <= end_of_path) {
                inf::EventTree::NodePos const node_pos(depth, path[depth]);

                if (depth < end_of_path) {
                    // Set the first outcomes according to the path
                    inf::EventTree::Node const &the_node = m_event_tree->get_node(node_pos);
                    thread_worker.marg_evaluators.set_outcome(depth, the_node.outcome);
                } else if (depth == end_of_path) {
                    // Init queue with a single node to be processed below
                    thread_worker.queue = inf::EventTree::NodePos::Queue({node_pos});
                }
            } else { // depth > end_of_path
                // If we don't store the bounds, we don't care about the end of the event being initialized to "?"
                if (not m_store_bounds)
                    break;
                else
                    thread_worker.marg_evaluators.set_outcome(depth, m_outcome_unknown);
            }
        }

        while (not thread_worker.queue.empty()) {
            inf::EventTree::NodePos const node_pos = util::pop_back(thread_worker.queue);
            go_down_from(thread_worker, node_pos);
        }
    }

    return inf::TreeOpt::ThreadReturn(inf::Optimizer::PreSolution(thread_worker.current_minimum,
                                                                  thread_worker.current_best_event),
                                      thread_worker.n_leaves_effective);
}

void inf::TreeOpt::go_down_from(inf::TreeOpt::ThreadWorker &thread_worker,
                                inf::EventTree::NodePos const &node_pos) {

    // In sat mode, as soon as the global minimum is <= 0, we stop minimizing.
    // In particular, the <= 0 score may be found in a differen thread.
    if (m_stop_mode == inf::Optimizer::StopMode::sat and m_global_minimum.get() <= 0) {
        thread_worker.queue.clear();
        return;
    }

    if (m_store_bounds) {
        // Here thread_worker.last_depth_processed may be m_inflation_n_parties-1 (i.e., unitialized from a
        // previous path), but it doesn't hurt to set the last outcomes to "?"
        for (Index depth(node_pos.depth + 1); depth <= thread_worker.last_depth_processed; ++depth)
            thread_worker.marg_evaluators.set_outcome(depth, m_outcome_unknown);

        thread_worker.last_depth_processed = node_pos.depth;
    }

    inf::EventTree::Node const &the_node = m_event_tree->get_node(node_pos);
    thread_worker.marg_evaluators.set_outcome(node_pos.depth, the_node.outcome);

    // if we're not at the last depth
    if (node_pos.depth < m_inflation_n_parties - 1) {
        bool keep_branch = true;

        if (m_store_bounds) {
            Num const score_lower_bound = thread_worker.marg_evaluators.evaluate_dual_vector();

            // We keep the branch only if the lower bound suggests that there is a chance to do strictly better than the current minimum
            keep_branch = (score_lower_bound < m_global_minimum.get());
        }

        if (keep_branch)
            m_event_tree->add_children_to_queue(thread_worker.queue, node_pos);
        else
            thread_worker.n_leaves_effective += 1;

    } else // if at the last depth
    {
        thread_worker.n_leaves_effective += 1;
        Num const score = thread_worker.marg_evaluators.evaluate_dual_vector();

        if (score < m_global_minimum.get()) {
            thread_worker.current_minimum = score;
            thread_worker.current_best_event = thread_worker.marg_evaluators.get_inflation_event();

            m_global_minimum.set_if_smaller(score);
        }
    }
}
