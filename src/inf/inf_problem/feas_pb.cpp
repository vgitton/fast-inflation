#include "feas_pb.h"
#include "../../util/logger.h"

void inf::FeasProblem::log(inf::FeasProblem::RetainEvents retain_events) {
    util::logger << util::begin_comment << "inf::FeasProblem::RetainEvents::"
                 << util::end_comment;
    switch (retain_events) {
    case inf::FeasProblem::RetainEvents::yes:
        util::logger << "yes";
        break;
    case inf::FeasProblem::RetainEvents::no:
        util::logger << "no";
        break;
    default:
        THROW_ERROR("switch")
    }
}

// FEAS PROBLEM

inf::FeasProblem::FeasProblem(inf::TargetDistr::ConstPtr const &distribution,
                              inf::FeasOptions::ConstPtr const &options)
    : m_distribution(distribution),
      m_options(options),
      m_constraint_set(
          std::make_shared<inf::ConstraintSet>(
              std::make_shared<const inf::Inflation>(
                  distribution,
                  options->get_inflation_size(),
                  options->get_use_distr_symmetries()),
              options->get_constraint_set_description(),
              options->get_store_bounds())),
      m_frank_wolfe(
          inf::FrankWolfe::get_frank_wolfe(
              options->get_fw_algo(),
              m_constraint_set->get_quovec_size(),
              options->get_n_threads())),
      // Will be initialized within log_info()
      m_optimizer(nullptr),
      m_n_iterations(0) {
    m_constraint_set->set_target_distribution(*m_distribution);

    init_frank_wolfe();

    log_info_and_init_optimizer();
}

void inf::FeasProblem::log_info_and_init_optimizer() {
    util::logger << util::Color::alt_blue << "inf::FeasProblem::log_info() -----------------"
                 << util::cr << util::cr;

    util::logger << *m_options << util::cr;

    util::logger << *m_constraint_set << util::cr;

    m_optimizer = inf::Optimizer::get_optimizer(
        m_options->get_search_mode(),
        m_constraint_set,
        m_options->get_symtree_io(),
        m_options->get_n_threads());

    m_optimizer->log_info();

    util::logger << util::Color::alt_blue << "----------------------------------------------" << util::cr;
}

void inf::FeasProblem::log_status_bar() const {
    util::logger << util::begin_comment
                 << m_frank_wolfe->get_n_stored_events() << " events"
                 << ", " << inf::Optimizer::total_optimization_chrono << " opt"
                 << ", " << inf::FrankWolfe::total_fw_chrono << " fw"
                 << ", " << util::Chrono::process_clock << " tot"
                 << util::end_comment
                 << "                                     " << util::cr;
}

inf::FeasProblem::Status inf::FeasProblem::get_feasibility() {
    std::cout << std::flush;

    LOG_BEGIN_SECTION_FUNC

    m_n_iterations = 0;

    inf::FeasProblem::Status status = inf::FeasProblem::Status::inconclusive;

    bool const new_display_style = true;
    Index last_printed_iteration = 0;

    while (true) {
        ++m_n_iterations;

        if (not new_display_style) {
            if (m_n_iterations > 1)
                util::logger.go_back_to_mark();
            util::logger.set_mark();

            util::logger << util::cr << "New iteration [" << m_n_iterations << "]" << util::cr;
            log_status_bar();
            util::logger << util::cr;
            util::logger << "Looking for the next dual vector...";
        }

        // m_constraint_set->log_dual_vectors();

        inf::FrankWolfe::Solution fw_sol = m_frank_wolfe->time_and_solve();
        if (not new_display_style) {
            util::logger << util::cr << fw_sol;
        } else {
            // Using \sqrt{2} as a ratio, since this gives more or less double the time for each print statement
            if (static_cast<double>(m_n_iterations) >= 1.4142 * static_cast<double>(last_printed_iteration)) {
                util::logger << "Iteration " << m_n_iterations << ", "
                             << "s = " << fw_sol.s << ", ";
                m_optimizer->log_status();
                log_status_bar();

                // To plot this in Mathematica
                // util::logger << "{" << m_n_iterations << ", ";
                // int the_log = static_cast<int>(std::log10(fw_sol.s));
                // util::logger << static_cast<int>(fw_sol.s / std::pow(10.0, the_log-3)) << "*10^" << the_log - 3 << "}, ";

                util::logger << util::flush;

                last_printed_iteration = m_n_iterations;
            }
        }

        if (not fw_sol.valid) {
            util::logger << "Iteration " << m_n_iterations << ", "
                         << "s = " << fw_sol.s << ", ";
            m_optimizer->log_status();
            log_status_bar();

            status = inf::FeasProblem::Status::inconclusive;

            if (not new_display_style)
                util::logger << " -> BREAK";

            break;
        }

        // m_optimizer holds a reference to the constraint set, and will minimize
        // the potential separating hyperplane given by the dual vector
        m_constraint_set->set_dual_vector_from_quovec(round_dual_vector(fw_sol.vec));

        if (not new_display_style)
            util::logger << util::cr << "Optimizing...";

        inf::Optimizer::Solution const sol = minimize_dual_vector();

        if (not new_display_style)
            util::logger << util::cr << sol;

        if (sol.get_inflation_event_score() > 0) {
            if (m_n_iterations > 1) {
                util::logger << "Iteration " << m_n_iterations << ", "
                             << "s = " << fw_sol.s << ", ";
                m_optimizer->log_status();
                log_status_bar();
            }

            util::logger << sol << util::cr;

            status = inf::FeasProblem::Status::nonlocal;

            if (not new_display_style)
                util::logger << " -> BREAK";

            break;
        }

        if (not new_display_style)
            util::logger << util::cr;

        // The distribution might be feasible...
        memorize_event(sol.get_inflation_event());
    }

    LOG_END_SECTION
    util::logger.cancel_go_back_to_mark();
    this->log_feasibility(status);
    util::logger << util::flush;
    return status;
}

void inf::FeasProblem::update_target_distribution(inf::TargetDistr::ConstPtr const &d,
                                                  inf::FeasProblem::RetainEvents retain_events) {
    m_distribution = d;
    m_n_iterations = 0;

    try {
        m_constraint_set->set_target_distribution(*d);
    } catch (inf::Symmetry::SymmetriesHaveChanged const &e) {
        THROW_ERROR(util::str("the target distribution's symmetries have changed, ") +
                    "but re-creating the inf::FeasProblem members is currently unimplemented," +
                    "please re-create an inf::FeasProblem entirely.")
    }

    std::set<inf::Event> prime_events;
    if (retain_events == inf::FeasProblem::RetainEvents::yes) {
        // Need to get the prime events before resetting!
        prime_events = m_frank_wolfe->get_stored_events();
    }

    m_frank_wolfe->reset();

    if (retain_events == inf::FeasProblem::RetainEvents::yes) {
        for (inf::Event const &event : prime_events)
            memorize_event(event);
    } else if (retain_events == inf::FeasProblem::RetainEvents::no) {
        init_frank_wolfe();
    } else
        THROW_ERROR("switch")
}

void inf::FeasProblem::write_dual_vector_to_file(std::string const &filename, std::string const &metadata) {
    m_constraint_set->write_dual_vector_to_file(filename, metadata);
}

void inf::FeasProblem::read_dual_vector_from_file(std::string const &filename, std::string const &metadata) {
    m_constraint_set->read_dual_vector_from_file(filename, metadata);
}

inf::Optimizer::Solution inf::FeasProblem::minimize_dual_vector() const {
    return m_optimizer->optimize(m_options->get_stop_mode());
}

inf::FeasProblem::Status inf::FeasProblem::read_and_check_dual_vector(std::string const &filename,
                                                                      std::string const &metadata) {
    LOG_BEGIN_SECTION_FUNC
    read_dual_vector_from_file(filename, metadata);
    inf::Optimizer::Solution sol = minimize_dual_vector();
    util::logger << sol << util::cr;

    inf::FeasProblem::Status status;
    if (sol.get_inflation_event_score() > 0) {
        status = inf::FeasProblem::Status::nonlocal;
    } else {
        status = inf::FeasProblem::Status::inconclusive;
    }
    log_feasibility(status);
    LOG_END_SECTION

    return status;
}

void inf::FeasProblem::log_feasibility(inf::FeasProblem::Status feas_status) const {
    util::logger << "Got "
                 << util::begin_comment << "inf::FeasProblem::Status::" << util::end_comment;
    if (feas_status == inf::FeasProblem::Status::nonlocal) {
        util::logger << util::Color::alt_magenta << "nonlocal" << util::Color::fg;
    } else if (feas_status == inf::FeasProblem::Status::inconclusive) {
        util::logger << util::Color::alt_cyan << "inconclusive" << util::Color::fg;
    } else
        THROW_ERROR("switch")

    util::logger //<< " with " << m_options
        << " for the ";
    m_distribution->log_short();
}

void inf::FeasProblem::memorize_event(inf::Event const &event) {
    std::vector<Num> const the_quovec = m_constraint_set->get_inflation_event_quovec(event);
    m_frank_wolfe->memorize_event_and_quovec(event, the_quovec, 0.001 * m_constraint_set->get_quovec_denom());
}

void inf::FeasProblem::init_frank_wolfe() {
    memorize_event(m_constraint_set->get_inflation()->get_all_zero_event());
}

inf::Quovec inf::FeasProblem::round_dual_vector(std::vector<double> const &dual_vector_double) const {
    ASSERT_EQUAL(dual_vector_double.size(), m_constraint_set->get_quovec_size())

    inf::Quovec dual_vector_rounded(m_constraint_set->get_quovec_size());

    double minimum_fw_sol_val = std::numeric_limits<double>::max();
    double maximum_fw_sol_val = std::numeric_limits<double>::min();

    for (double component : dual_vector_double) {
        if (component < minimum_fw_sol_val)
            minimum_fw_sol_val = component;
        if (component > maximum_fw_sol_val)
            maximum_fw_sol_val = component;
    }

    minimum_fw_sol_val = std::abs(minimum_fw_sol_val);
    maximum_fw_sol_val = std::abs(maximum_fw_sol_val);

    double const largest_value = std::max(minimum_fw_sol_val, maximum_fw_sol_val);

    double const scale_factor = 0.95 * static_cast<double>(m_constraint_set->get_max_dual_vector_component()) / largest_value;

    for (Index i : util::Range(m_constraint_set->get_quovec_size())) {
        dual_vector_rounded[i] = static_cast<Num>(scale_factor * dual_vector_double[i]);
    }

    util::simplify_by_gcd(dual_vector_rounded);

    return dual_vector_rounded;
}
