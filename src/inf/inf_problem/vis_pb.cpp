#include "vis_pb.h"
#include "../../util/logger.h"
#include "feas_pb.h"

std::string inf::VisProblem::visibility_to_str(Num visibility, Num denom) {
    std::string const ret = util::str(visibility) + "/" + util::str(denom) + " = ";
    double const vis = static_cast<double>(visibility) / static_cast<double>(denom);

    std::string const before_dot = util::str(static_cast<Num>(100 * vis));
    std::string after_dot = util::str(static_cast<Num>(100000 * vis) % 1000);
    while (after_dot.size() < 3)
        after_dot = "0" + after_dot;
    return ret + before_dot + "." + after_dot + "%";
}

inf::VisProblem::VisProblem(inf::TargetDistr::ConstPtr (*get_distribution)(Num, Num),
                            Num min_visibility,
                            Num max_visibility,
                            Num visibility_denom,
                            inf::FeasOptions::ConstPtr const &feas_problem_options,
                            inf::FeasProblem::RetainEvents retain_events)
    : m_get_distribution(get_distribution),
      m_min_visibility(min_visibility),
      m_max_visibility(max_visibility),
      m_visibility_denom(visibility_denom),
      m_feas_problem(nullptr),
      m_feas_problem_options(feas_problem_options),
      m_retain_events(retain_events) {}

Num inf::VisProblem::get_minimum_nonlocal_visibility() {
    util::logger << "Constructing inf::VisProblem, will run a dichotomic search between "
                 << inf::VisProblem::visibility_to_str(m_min_visibility, m_visibility_denom)
                 << " and "
                 << inf::VisProblem::visibility_to_str(m_max_visibility, m_visibility_denom)
                 << "." << util::cr << util::cr;

    util::logger << util::flush;

    if (not visibility_is_feasible(m_min_visibility)) {
        util::logger << "Even the min visibility "
                     << visibility_to_str(m_min_visibility, m_visibility_denom)
                     << " is nonlocal !" << util::cr;
        return m_min_visibility;
    }

    if (visibility_is_feasible(m_max_visibility)) {
        util::logger << "Even the max visibility "
                     << visibility_to_str(m_max_visibility, m_visibility_denom)
                     << " is compatible with the target inflation..." << util::cr;

        return m_max_visibility + 1;
    }

    Num max_feasible_vis = m_min_visibility;
    Num min_infeasible_vis = m_max_visibility;
    // We now know that max_feasible_vis is feasible
    // and min_infeasible_vis is infeasible. Can move on to the core of the loop.

    while (true) {
        ASSERT_LT(max_feasible_vis, min_infeasible_vis)

        if ((max_feasible_vis + 1) == min_infeasible_vis) {
            util::logger
                << "The minimum infeasible visibility at inflation level "
                << m_feas_problem_options->get_inflation_size()
                << " is "
                << visibility_to_str(min_infeasible_vis, m_visibility_denom) << util::cr;

            // This section is started in visibility_is_feasible()
            LOG_END_SECTION
            return min_infeasible_vis;
        }

        Num const middle_vis = (max_feasible_vis + min_infeasible_vis) / 2;

        ASSERT_LT(max_feasible_vis, middle_vis)
        ASSERT_LT(middle_vis, min_infeasible_vis)

        bool const middle_vis_is_feasible = visibility_is_feasible(middle_vis);

        if (middle_vis_is_feasible)
            max_feasible_vis = middle_vis;
        else
            min_infeasible_vis = middle_vis;
    }

    THROW_ERROR("Went out of the while loop somehow...")
}

bool inf::VisProblem::visibility_is_feasible(Num visibility) {
    inf::TargetDistr::ConstPtr distribution = m_get_distribution(visibility, m_visibility_denom);

    if (m_feas_problem == nullptr) {
        m_feas_problem = std::make_unique<inf::FeasProblem>(distribution, m_feas_problem_options);

        util::logger << util::cr;

        LOG_BEGIN_SECTION("Dichotomic search")
    } else {
        m_feas_problem->update_target_distribution(distribution, m_retain_events);
    }

    LOG_BEGIN_SECTION("Feasibility at " + visibility_to_str(visibility, m_visibility_denom) + " visibility")

    util::logger << util::flush;

    inf::FeasProblem::Status feas_status = m_feas_problem->get_feasibility();

    LOG_END_SECTION

    m_feas_problem->log_status_bar();
    util::logger << util::cr;

    return (feas_status != inf::FeasProblem::Status::nonlocal);
}
