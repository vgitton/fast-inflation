#pragma once

#include "../inf/inf_problem/feas_options.h"
#include "../util/chrono.h"
#include "inf_cli.h"
#include <memory>
#include <string>

/*! \file */

namespace user {

/*! \ingroup user
 * \brief Parent class of user applications
 * \details Notice that each child class gets access to some inf::FeasOptions, appropriately initialized with the command-line parameters,
 * see user::Application::get_feas_options().
 *
 * For performance reasons, the constructor of any derived class should be trivial, and any initialization code should be called only
 * *after* the user::Application::run() method is called. The reason is that internally, *all* available classes will be constructed
 * every time the executable is ran, but only the target application whose name matches the command-line arguments will see its user::Application::run() method called.
 * \sa user::get_application_list() for available %applications */
class Application {
  public:
    typedef std::shared_ptr<user::Application> Ptr;

    /*! \brief Constructor with command line target name
        \param name The name that will then be callable from the command line using `./{debug|release}_inf name`
        \param description A short description, displayed when listing the existing applications
        \param is_test Set to `true` to register the application as a test that will be ran with `./{debug|release}_inf test` */
    Application(std::string const &name,
                std::string const &description = "",
                bool is_test = false);
    //! \cond
    Application(Application const &other) = delete;
    Application(Application &&other) = delete;
    Application &operator=(Application const &other) = delete;
    Application &operator=(Application &&other) = delete;
    //! \endcond

    /*! \brief To inherit the user::InfCLI storing the command-line parameters */
    void set_inf_cli(user::InfCLI::ConstPtr const &inf_cli);
    /*! \brief To inherit the inf::FeasOptions storing the inflation problem specifications */
    void set_feas_options(inf::FeasOptions::Ptr const &feas_options);

    /*! \brief Returns `true` is the name passed to user::Application::Application() matches the argument `name` */
    bool name_equals(std::string const &name) const;
    /*! \brief Application name passed to user::Application::Application() */
    std::string const &get_name() const;
    /*! \brief Get the description of what the application does */
    std::string const &get_description() const;
    /*! \brief Whether or not the application is to be included in the list of tests */
    bool is_test() const;
    /*! \brief Logs the execution time of the application */
    void log_execution_time() const;
    /*! \brief Times a call to user::Application::run() and catches exceptions */
    bool time_and_run();

    /*! \brief This method runs the application */
    virtual void run() = 0;

  protected:
    /*! \brief The command-line argument, this is mostly provided so that child applications can access the `--vis` parameter */
    user::InfCLI const &get_inf_cli() const;
    /*! \brief This allows child classes to be able to receive and modify the inf::FeasOptions initialized from the command-line arguments
     * \details The idea is that the inf::FeasOptions of each user::Application are initialized in inf::AppManager::parse_argv()
     * based on the command-line arguments, but each child user::Application can then modify it in their user::Application::run() override
     * using e.g.
     * \code{.cpp}
     * (*get_feas_options()).set(inf::Inflation::Size({2,2,2}));
     * \endcode
     * */
    inf::FeasOptions::Ptr const &get_feas_options() const;

  private:
    /*! \brief The identifier of the user::Application to make it callable from the command line */
    const std::string m_name;
    /*! \brief The description displayed when listing all existing applications */
    const std::string m_description;
    /*! \brief Whether or not the application is to be run with the other tests */
    bool m_is_test;
    /*! \brief The execution time of the application */
    util::Chrono m_chrono;

    /*! \brief The command-line parameters */
    user::InfCLI::ConstPtr m_inf_cli;
    /*! \brief This is not a pointer-to-const because we want each child user::Application to be able to modify the inf::FeasOptions,
     * starting from the ones initalized based on the command-line parameters */
    inf::FeasOptions::Ptr m_feas_options;
};

} // namespace user
