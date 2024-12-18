#pragma once

#include "../inf/events/event_tree.h"
#include "../util/base_cli.h"
#include "../util/loggable.h"

#include <memory>

namespace user {

/*! \ingroup user
 * \brief This class defines the command-line parameters that are expected
 * \details This is used by the user::AppManager to parse the command-line arguments and is based on the class util::BaseCLI.
 * See the \ref CLIparams "command-line parameters section" for more details.
 * */
class InfCLI : public util::Loggable {
  public:
    typedef std::shared_ptr<user::InfCLI> Ptr;
    typedef std::shared_ptr<const user::InfCLI> ConstPtr;

    /*! \brief This constructor does not do anything, call user::InfCLI::init() to initialize */
    InfCLI(int argc, char **argv);

    /*! \brief Need to call this method first before being able to use the getter methods
     * \details This method throws an error if the command-line parameters that were passed cannot be appropriately parsed. */
    void init();

    void log() const override;

    /*! \brief The returned string describes the expected command-line parameters */
    std::string const &get_usage() const;

    std::string const &get_target_application_name() const;
    unsigned int get_log_level() const;
    unsigned int get_n_threads() const;
    inf::EventTree::IO get_symtree_io() const;
    unsigned int get_visibility_param() const;

  private:
    bool m_initialized;

    std::string m_usage;

    util::BaseCLI m_base_cli;

    std::string m_target_application_name;
    unsigned int m_log_level;
    unsigned int m_n_threads;

    std::string m_symtree_io;

    unsigned int m_visibility_param;
};

} // namespace user
