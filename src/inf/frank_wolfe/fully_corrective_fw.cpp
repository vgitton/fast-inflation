#include "fully_corrective_fw.h"
#include "../../util/debug.h"
#include "../../util/logger.h"
#include "../../util/math.h"
#include "../../util/range.h"
#include "fusion.h"

#include <sstream>

// Public interface

inf::FullyCorrectiveFW::FullyCorrectiveFW(Index dimension, Index n_threads)
    : inf::FrankWolfe(dimension),
      m_n_threads(n_threads),
      m_events{},
      m_matrix{},
      m_model(new mosek::fusion::Model("Fully-corrective Frank-Wolfe")),
      m_model_disposer([&]() { m_model->dispose(); }),
      m_w(nullptr),
      m_s(nullptr) {
    util::logger << "Constructing inf::FullyCorrectiveFW. "
                 << "Using Mosek version " << m_model->getVersion()
                 << " with " << m_n_threads << " threads." << util::cr;
    this->init_model();
}

void inf::FullyCorrectiveFW::memorize_event_and_quovec_double(inf::Event const &event,
                                                              std::vector<double> const &row_double) {
    // Memorize event
    m_events.insert(event);

    std::shared_ptr<monty::ndarray<double, 1>> const mosek_row = monty::new_array_ptr(row_double);

    // The constraint s <= <w,f_i>
    m_model->constraint(mosek::fusion::Expr::sub(mosek::fusion::Expr::dot(mosek_row, var_to_expr(m_w)), m_s),
                        mosek::fusion::Domain::greaterThan(0.0));

    m_matrix.push_back(row_double);
}

inf::FrankWolfe::Solution inf::FullyCorrectiveFW::solve() {
    m_model->solve();

    // Hard assertion because this is also relevant in release mode
    HARD_ASSERT_EQUAL(to_str(m_model->getProblemStatus()),
                      to_str(mosek::fusion::ProblemStatus::PrimalAndDualFeasible))
    HARD_ASSERT_EQUAL(to_str(m_model->getPrimalSolutionStatus()),
                      to_str(mosek::fusion::SolutionStatus::Optimal))

    std::shared_ptr<monty::ndarray<double, 1>> const w_monty_ptr = m_w->level();
    std::vector<double> solution_vec(w_monty_ptr->begin(),
                                     w_monty_ptr->end());

    double const s = m_model->primalObjValue();
    double const min_inner_product = this->get_min_inner_product(solution_vec);
    bool const is_valid_solution = (s > 0.0) and (min_inner_product > 0.0);

    inf::FrankWolfe::Solution sol(s, solution_vec, is_valid_solution);

    return sol;
}

std::set<inf::Event> const &inf::FullyCorrectiveFW::get_stored_events() const {
    return m_events;
}

Index inf::FullyCorrectiveFW::get_n_stored_events() const {
    return m_events.size();
}

void inf::FullyCorrectiveFW::reset() {
    m_events.clear();
    m_matrix.clear();

    m_model->dispose();
    m_model = new mosek::fusion::Model("Max-Min-Inner-Product");

    this->init_model();
}

// Private methods

void inf::FullyCorrectiveFW::init_model() {
    m_model->setLogHandler(
        [](std::string const &msg) {
            util::logger << msg;
        });
    m_model->setSolverParam("log", 0);
    m_model->setSolverParam("logPresolve", 0);
    m_model->setSolverParam("presolveLindepUse", "off");
    m_model->setSolverParam("presolveUse", "on");
    m_model->setSolverParam("numThreads", static_cast<int>(m_n_threads));

    // Vector variable
    init_w();
    // Slack variable for norm squared
    init_s();

    // Objective
    m_model->objective(mosek::fusion::ObjectiveSense::Maximize, m_s);
}

void inf::FullyCorrectiveFW::init_w() {
    m_w = m_model->variable(static_cast<int>(m_dimension),
                            mosek::fusion::Domain::unbounded(static_cast<int>(m_dimension)));

    // The constraint of the norm of w, ||w|| <= 1
    m_model->constraint(mosek::fusion::Expr::vstack(1.0, var_to_expr(m_w)),
                        mosek::fusion::Domain::inQCone(static_cast<int>(m_dimension + 1)));
}

void inf::FullyCorrectiveFW::init_s() {
    mosek::fusion::Variable::t var_s = m_model->variable(1, mosek::fusion::Domain::unbounded());

    m_s = var_to_expr(var_s);
}

// Utilities

mosek::fusion::Expression::t inf::FullyCorrectiveFW::var_to_expr(mosek::fusion::Variable::t &var) {
    // Explicit conversion to avoid compiler warnings
    return var.operator mosek::fusion::Expression::t();
}

std::string inf::FullyCorrectiveFW::to_str(mosek::fusion::ProblemStatus status) {
    std::ostringstream string_stream;
    string_stream << status;
    return string_stream.str();
}

std::string inf::FullyCorrectiveFW::to_str(mosek::fusion::SolutionStatus status) {
    std::ostringstream string_stream;
    string_stream << status;
    return string_stream.str();
}

double inf::FullyCorrectiveFW::get_min_inner_product(std::vector<double> const &vec) const {
    double min = 0.0;
    bool min_uninit = true;

    for (std::vector<double> const &row : m_matrix) {
        if (min_uninit) {
            min = util::inner_product(vec, row);
            min_uninit = false;
        } else {
            double const new_val = util::inner_product(vec, row);
            if (new_val < min)
                min = new_val;
        }
    }

    return min;
}
