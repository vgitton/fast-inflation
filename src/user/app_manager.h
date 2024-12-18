#pragma once

#include <string>
#include <vector>

#include "application.h"
#include "inf_cli.h"

/*! \file */

/*! \defgroup user Application interface
    \ingroup applications
    \brief Describes how to conveniently add command-line support for new applications of the code

    This module provides a basic command-line interface along with mecanisms to register user-defined applications.
    To register a new application:
    - create a class that inherits from user::Application,
    - select a `name` to be passed to the parent class constructor user::Application::Application(): this name
    will be callable from the command line using `./{debug|release}_inf name`,
    - specify whether or not the application is to be included in the list of tests (ran wiht `./{debug|release}_inf test`) by passing `is_test = true` or `is_test = false`
    to user::Application::Application(),
    - override user::Application::run() in the child class: this function returns void, and issues should be handled using #THROW_ERROR or hard assertions such as #HARD_ASSERT_EQUAL, #HARD_ASSERT_TRUE etc. --- see the \ref debug module for the reference of hard and soft assertions, and see user::AppManager::run_all_tests() for more details about tests,
    - include your class in the implementation of user::get_application_list() in src/user/application_list.cpp.

    For more details, see the user::Application class documentation.
*/

/*! \brief The namespace for application code */
namespace user {

/*! \ingroup user
 * \brief This class parses the command-line arguments to try to run an application listed in user::get_application_list()
 * \details The main point of this class is to parse the command-line arguments, display adequate error messages and usage guides, and run applications when applicable.
 * This enables the syntax `./{debug|release}_inf example_to_run [optional parameters]`, see the \ref CLIparams "command-line parameters section".
 * \sa user::get_application_list() to register additional examples
 * \sa The ::main() entrypoint of the program */
class AppManager {
  public:
    /*! \brief The constructor receives the command-line arguments */
    AppManager(int argc, char **argv);
    //! \cond
    AppManager(AppManager const &other) = delete;
    AppManager(AppManager &&other) = delete;
    AppManager &operator=(AppManager const &other) = delete;
    AppManager &operator=(AppManager &&other) = delete;
    //! \endcond

  private:
    /*! \brief To parse the command-line arguments */
    void parse_argv();

    /*! \brief This runs all test, hiding their output but showing their execution time & success/failure status */
    void run_all_tests() const;

    /*! \brief Runs a specific application */
    void run_application(user::Application::Ptr const &ptr) const;

    /*! \brief Display a welcome message with usage guide and available applications */
    void display_available_applications() const;

    /*! \brief For internal use, pretty-prints `bool`s
        \param success `true` logs as `OK`, `false` as `FAIL` */
    static void log_status_str(bool success);
    /*! \brief For internal use
        \param only_look_at_tests
        \return Max application name width */
    int get_max_application_name_width(bool only_look_at_tests) const;

    /*! \brief The application list fetched from user::get_application_list() */
    std::vector<user::Application::Ptr> m_application_list;
    /*! \brief Debug or release */
    std::string m_compile_mode;

    /*! \brief This stores the parsed command-line parameters */
    user::InfCLI::Ptr const m_inf_cli;
};

} // namespace user
