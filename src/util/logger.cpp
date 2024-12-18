#include "logger.h"
#include "debug.h"
#include "range.h"

#include <iomanip>

// Change this if do not wish to have color in the terminal
util::Logger util::logger(util::Logger::ColorMode::ansi);

const util::Color util::begin_comment = util::Color::alt_black;
const util::Color util::end_comment = util::Color::fg;

util::Logger::Logger(
    util::Logger::ColorMode color_mode)
    : m_depth_enabled(true),
      m_color_mode(color_mode),
      m_log_level(1000),

      m_section_index(1),
      m_indent_pending(true),
      m_disable_dots_for_hidden_sections(false),
      m_n_cr_since_mark(0),
      m_max_n_cr_since_mark(0) {}

void util::Logger::set_log_level(unsigned int log_level) {
    m_log_level = log_level;

    if (m_section_index > m_log_level)
        m_depth_enabled = false;
    else
        m_depth_enabled = true;
}

void util::Logger::disable_dots_for_hidden_sections() {
    m_disable_dots_for_hidden_sections = true;
}

void util::Logger::echo(std::string const &str) {
    if (is_disabled())
        return;

    process_pending_indent();

    std::cout << str;
}

void util::Logger::process_pending_indent() {
    if (is_disabled())
        return;

    if (m_indent_pending) {
        m_indent_pending = false;

        *this << util::begin_comment;
        for (Index i(0); i < m_section_index; ++i) {
            (void)i;
            std::cout << "  | ";
        }
        *this << util::end_comment;
    }
}

void util::Logger::flush() {
    if (is_disabled())
        return;

    std::cout << std::flush;
}

void util::Logger::new_line() {
    if (is_disabled())
        return;

    ++m_n_cr_since_mark;

    echo("\n");
    m_indent_pending = true;
}

void util::Logger::begin_section() {
    ++m_section_index;

    if (m_section_index > m_log_level) {
        if (not m_disable_dots_for_hidden_sections) {
            *this << util::begin_comment << "..." << util::end_comment << util::cr;
        }

        m_depth_enabled = false;
    }
}

void util::Logger::end_section() {
    ASSERT_LT(0, m_section_index)
    --m_section_index;

    if (m_section_index <= m_log_level) {
        m_depth_enabled = true;
    }
}

void util::Logger::set_next_width(int n) {
    if (is_disabled())
        return;

    process_pending_indent();

    std::cout << std::setw(n);
}

void util::Logger::set_alignment(std::ios_base::fmtflags alignment) {
    if (is_disabled())
        return;

    process_pending_indent();

    std::cout.setf(alignment, std::ios_base::adjustfield);
}

void util::Logger::reset_indent() {
    if (m_log_level >= 1) {
        m_depth_enabled = true;
        m_section_index = 1;
    }
}

void util::Logger::echo_title(std::string const &name) {
    *this << util::Color::alt_green << name << util::Color::fg << util::cr;
}

void util::Logger::echo_header(std::string const &name) {
    *this << util::Color::alt_blue << name << util::Color::fg << util::cr;
}

std::string util::Logger::get_color_code(util::Color c) const {
    if (logger.get_color_mode() == util::Logger::ColorMode::ansi) {
        return util::get_ansi_escape_code(c);
    } else {
        return "";
    }
}
std::string util::Logger::get_colored_number_str(int number) const {
    std::string ret;

    switch (number) {
    case 0:
        ret += get_color_code(util::Color::red);
        break;
    case 1:
        ret += get_color_code(util::Color::green);
        break;
    case 2:
        ret += get_color_code(util::Color::blue);
        break;
    case 3:
        ret += get_color_code(util::Color::yellow);
        break;
    default:
        break;
    }
    ret += util::str(number) + get_color_code(util::Color::fg);

    return ret;
}

void util::Logger::echo_colored_number(int number) {
    util::logger << get_colored_number_str(number);
}

void util::Logger::echo_error(std::string const &what) {
    util::logger << util::reset_indent
                 << util::Color::alt_red << "Error: " << util::Color::fg
                 << what << util::cr;
}

std::string util::Logger::get_single_char(unsigned long int i) const {
    if (i <= 9) {
        return std::string(1, static_cast<char>('0' + i));
    } else {
        return std::string(1, static_cast<char>('a' + i - 10));
    }
}

void util::Logger::set_mark() {
    m_n_cr_since_mark = 0;
}

void util::Logger::go_back_to_mark() {
    m_max_n_cr_since_mark = m_n_cr_since_mark;
    this->move_up(m_n_cr_since_mark);
}

void util::Logger::cancel_go_back_to_mark() {
    this->move_down(m_max_n_cr_since_mark - m_n_cr_since_mark);
}

void util::Logger::move_up(int i) {
    if (is_disabled())
        return;

    m_n_cr_since_mark -= i;
    std::cout << "\033[" + util::str(i + 1) + "A\n";
}

void util::Logger::move_down(int i) {
    if (is_disabled())
        return;

    if (i > 1) {
        std::cout << "\033[" + util::str(i - 1) + "B";
    }
    *this << util::cr;
}

util::Logger &operator<<(util::Logger &logger, util::Logger::Action action) {
    switch (action) {
    case util::flush:
        logger.flush();
        break;

    case util::cr:
        logger.new_line();
        break;

    case util::endl:
        logger << util::cr << util::flush;
        break;

    case util::begin_section:
        logger.begin_section();
        break;

    case util::end_section:
        logger.end_section();
        break;

    case util::left:
        logger.set_alignment(std::ios_base::left);
        break;

    case util::right:
        logger.set_alignment(std::ios_base::right);
        break;

    case util::reset_indent:
        logger.reset_indent();
        break;

    case util::wait: {
        std::string s;
        std::cin >> s;
        break;
    }

    default:
        THROW_ERROR("did not recognize the util::Logger::Action")
    }

    return logger;
}

util::Logger::WidthManip util::setw(int width) {
    return util::Logger::WidthManip(width);
}

std::string util::phantom(std::string const &str) {
    return std::string(str.size(), ' ');
}

util::Logger &operator<<(util::Logger &logger, util::Logger::WidthManip width_manip) {
    logger.set_next_width(width_manip.width);
    return logger;
}

util::Logger &operator<<(util::Logger &logger, util::Color c) {
    logger << logger.get_color_code(c);
    return logger;
}
