#include "application.h"
#include "../inf/optimization/optimizer.h"
#include "../util/logger.h"

user::Application::Application(std::string const &name,
                               std::string const &description,
                               bool is_test)
    : m_name(name),
      m_description(description),
      m_is_test(is_test),
      m_chrono(util::Chrono::State::paused),
      m_inf_cli(nullptr),
      m_feas_options(nullptr) {}

void user::Application::set_inf_cli(user::InfCLI::ConstPtr const &inf_cli) {
    m_inf_cli = inf_cli;
}

void user::Application::set_feas_options(inf::FeasOptions::Ptr const &feas_options) {
    m_feas_options = feas_options;
}

bool user::Application::name_equals(std::string const &name) const {
    return m_name == name;
}

std::string const &user::Application::get_name() const {
    return m_name;
}

std::string const &user::Application::get_description() const {
    return m_description;
}

bool user::Application::is_test() const {
    return m_is_test;
}

void user::Application::log_execution_time() const {
    util::logger
        << "opt: " << inf::Optimizer::total_optimization_chrono
        << ", fw: " << inf::FrankWolfe::total_fw_chrono
        << ", tot: " << m_chrono;
}

bool user::Application::time_and_run() {
    bool success = false;

    m_chrono.reset();
    m_chrono.start();

    try {
        run();
        success = true;
    } catch (std::exception const &e) {
        success = false;
        util::logger.echo_error(e.what());
    } catch (...) {
        success = false;
        util::logger.echo_error("An exception of unknown type occured.");
    }

    m_chrono.pause();

    return success;
}

user::InfCLI const &user::Application::get_inf_cli() const {
    HARD_ASSERT_TRUE(m_inf_cli != nullptr)
    return *m_inf_cli;
}

inf::FeasOptions::Ptr const &user::Application::get_feas_options() const {
    HARD_ASSERT_TRUE(m_feas_options != nullptr)
    return m_feas_options;
}
