#include "app_manager.h"
#include "../inf/inf_problem/feas_options.h"
#include "../util/debug.h"
#include "../util/logger.h"
#include "application_list.h"

// For the current time
#include <chrono>
#include <iomanip>
#include <sstream>

user::AppManager::AppManager(int argc, char **argv)
    : m_application_list(user::get_application_list()),
      m_compile_mode(""),
      m_inf_cli(std::make_shared<user::InfCLI>(argc, argv)) {
    // Init debug vs release mode marker

#ifdef NDEBUG
    m_compile_mode = util::logger.get_color_code(util::Color::alt_yellow) + "[RELEASE]" + util::logger.get_color_code(util::Color::fg);
#else
    m_compile_mode = "[DEBUG]";
#endif

    parse_argv();
}

void user::AppManager::parse_argv() {
    bool display_apps = false;

    try {
        m_inf_cli->init();

        util::logger << "Command-line arguments:" << util::cr
                     << *m_inf_cli;

        for (user::Application::Ptr const &application : m_application_list) {
            // Initialize the feas_options for each application independently so that
            // when running all tests, the modifications of the feas options of one application
            // do not carry over to the next application
            inf::FeasOptions::Ptr feas_options = std::make_shared<inf::FeasOptions>();
            feas_options->set_n_threads(m_inf_cli->get_n_threads());
            feas_options->set(m_inf_cli->get_symtree_io());

            application->set_inf_cli(m_inf_cli);
            application->set_feas_options(feas_options);
        }

        std::string const &target_app_name = m_inf_cli->get_target_application_name();

        if (target_app_name == std::string("")) {
            display_apps = true;
        } else if (target_app_name == std::string("test")) {
            run_all_tests();
        } else {
            user::Application::Ptr target_application(nullptr);
            for (user::Application::Ptr const &application : m_application_list) {
                if (application->name_equals(target_app_name)) {
                    target_application = application;
                    break;
                }
            }

            if (target_application == nullptr) {
                display_apps = true;
                THROW_ERROR("Could not find the target application " + target_app_name);
            }

            run_application(target_application);
        }
    } catch (std::exception const &e) {
        util::logger.echo_error(e.what());
    }

    if (display_apps) {
        display_available_applications();
    }
}

void user::AppManager::run_all_tests() const {
    util::logger.set_log_level(1);
    util::logger.disable_dots_for_hidden_sections();

    util::logger.echo_title("Running all tests " + m_compile_mode);

    int max_width = get_max_application_name_width(true);

    for (user::Application::Ptr const &application : m_application_list) {
        if (application->is_test()) {
            util::logger << "Running "
                         << util::left << util::setw(max_width + 3 + 1) << application->get_name() + std::string("...")
                         << util::flush;

            util::logger << util::begin_section;
            bool success = application->time_and_run();

            if (success) {
                // If not success, the time_and_run() function will already have
                // reset the indentation.
                // util::logger << util::end_section;
                util::logger << util::reset_indent;
            } else {
                util::logger << util::setw(8 + max_width + 3 + 1) << " ";
            }

            log_status_str(success);
            util::logger << ", ";
            application->log_execution_time();
            util::logger << util::endl;
        }
    }

    util::logger << util::end_section;
}

void user::AppManager::run_application(user::Application::Ptr const &target_application) const {
    util::logger.set_log_level(m_inf_cli->get_log_level());

    std::string the_time;
    {
        std::time_t now = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());

        std::ostringstream oss;
        // Format: "12h23, Friday  8 October 2023"
        std::string format = "%Hh%M, %A %e %B %Y";
        oss << std::put_time(std::localtime(&now), format.c_str());
        the_time = oss.str();
    }

    util::logger.echo_title(the_time + " - Running " + target_application->get_name() + " " + m_compile_mode);
    util::logger << util::cr;

    bool success = target_application->time_and_run();

    log_status_str(success);
    util::logger << util::Color::alt_green << ", ";
    target_application->log_execution_time();
    util::logger << util::Color::fg << util::cr << util::end_section;
}

void user::AppManager::display_available_applications() const {
    util::logger.echo_title("Welcome!");

    util::logger
        << "Run as:" << util::cr
        << "$ " << util::Color::alt_yellow << "./{debug|release}_inf " << m_inf_cli->get_usage() << util::Color::fg << util::cr
        << "e.g.," << util::cr
        << "$ " << util::Color::alt_yellow << "./debug_inf ejm --verb 2" << util::Color::fg << util::cr
        << "The " << util::Color::alt_yellow << "app" << util::Color::fg << " can be any of the following:" << util::cr;

    int max_width = get_max_application_name_width(false);

    std::string test_qualifier = "(test)";

    util::logger << util::left;
    util::logger << util::setw(max_width) << "test"
                 << util::setw(8) << " " << util::phantom(test_qualifier)
                 << util::begin_comment << "Run all tests"
                 << util::end_comment << util::cr;

    for (user::Application::Ptr const &t : m_application_list) {
        util::logger << util::setw(max_width) << t->get_name();
        util::logger << util::setw(4) << " ";
        if (t->is_test()) {
            util::logger << util::begin_comment << test_qualifier << util::end_comment;
        } else {
            util::logger << util::phantom(test_qualifier);
        }
        std::string const &description = t->get_description();
        if (description.size() > 0) {
            util::logger << util::setw(4) << " " << util::begin_comment
                         << description << util::end_comment;
        }
        util::logger << util::cr;
    }

    util::logger << util::end_section;
}

void user::AppManager::log_status_str(bool success) {
    if (success)
        util::logger << util::Color::alt_green << "OK";
    else
        util::logger << util::Color::alt_red << "FAIL";

    util::logger << util::Color::fg;
}

int user::AppManager::get_max_application_name_width(bool only_look_at_tests) const {
    // Because may want to run "test", too
    size_t max_width = 4;
    for (auto const &t : m_application_list) {
        if (only_look_at_tests and not t->is_test())
            continue;

        size_t current_width = t->get_name().size();
        if (current_width > max_width)
            max_width = current_width;
    }

    return static_cast<int>(max_width);
}
