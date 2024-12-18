#include "inf_cli.h"
#include "../util/debug.h"
#include "../util/logger.h"

#include <thread>

user::InfCLI::InfCLI(int argc, char **argv)
    : m_initialized(false),
      m_usage(""),
      m_base_cli(argc, argv),
      // Don't place the default values here
      m_target_application_name(""),
      m_log_level(),
      m_n_threads(),
      m_symtree_io(),
      m_visibility_param() {}

void user::InfCLI::init() {
    HARD_ASSERT_TRUE(not m_initialized)

    // Init the unnamed args
    m_target_application_name = m_base_cli.extract_str_arg("");
    m_usage += "[app] ";

    // Init the named options
    m_log_level = m_base_cli.extract_uint_option("verb", 4);
    m_usage += "[--verb {1,2,...}] ";

    m_n_threads = m_base_cli.extract_uint_option("threads", 1);
    m_usage += "[--threads {1,2,...}] ";

    m_symtree_io = m_base_cli.extract_str_option("symtree-io", "read");
    m_usage += "[--symtree-io {none,read,write}] ";

    m_visibility_param = m_base_cli.extract_uint_option("vis", 0);
    m_usage += "[--vis N]";

    m_base_cli.hard_assert_empty();

    m_initialized = true;
}

void user::InfCLI::log() const {
    HARD_ASSERT_TRUE(m_initialized)

    util::logger
        << util::begin_comment << "app = " << util::end_comment
        << "\"" << m_target_application_name << "\"" << util::cr
        << util::begin_comment << "--verb = " << util::end_comment
        << m_log_level << util::cr
        << util::begin_comment << "--threads = " << util::end_comment
        << m_n_threads << util::cr
        << util::begin_comment << "--symtree-io = " << util::end_comment
        << m_symtree_io << util::cr
        << util::begin_comment << "--vis = " << util::end_comment
        << m_visibility_param << util::cr;
}

std::string const &user::InfCLI::get_usage() const {
    return m_usage;
}

std::string const &user::InfCLI::get_target_application_name() const {
    HARD_ASSERT_TRUE(m_initialized)
    return m_target_application_name;
}

unsigned int user::InfCLI::get_log_level() const {
    HARD_ASSERT_TRUE(m_initialized)
    return m_log_level;
}

unsigned int user::InfCLI::get_n_threads() const {
    HARD_ASSERT_TRUE(m_initialized)
    HARD_ASSERT_LT(0, m_n_threads)
    HARD_ASSERT_LT(m_n_threads, std::thread::hardware_concurrency() + 1)
    return m_n_threads;
}

inf::EventTree::IO user::InfCLI::get_symtree_io() const {
    HARD_ASSERT_TRUE(m_initialized)

    if (m_symtree_io == "read") {
        return inf::EventTree::IO::read;
    } else if (m_symtree_io == "write") {
        return inf::EventTree::IO::write;
    } else if (m_symtree_io == "none") {
        return inf::EventTree::IO::none;
    } else {
        THROW_ERROR("Could not parse \"" + m_symtree_io + "\" to {none,read,write}")
    }
}

unsigned int user::InfCLI::get_visibility_param() const {
    return m_visibility_param;
}
