#include "frac.h"
#include "debug.h"
#include "logger.h"
#include <limits>

util::Frac::Frac(Num num, Num denom)
    : m_num(num),
      m_denom(denom) {
    ASSERT_LT(0, denom);
}

Num util::Frac::get_num() const {
    return m_num;
}

Num &util::Frac::get_num() {
    return m_num;
}

Num util::Frac::get_denom() const {
    return m_denom;
}

bool util::Frac::operator<(
    util::Frac const &other) const {
    HARD_ASSERT_LT(m_num, std::numeric_limits<Num>::max() / other.m_denom)
    HARD_ASSERT_LT(other.m_num, std::numeric_limits<Num>::max() / m_denom)
    return (m_num * other.m_denom) < (other.m_num * m_denom);
}

bool util::Frac::operator>(
    util::Frac const &other) const {
    HARD_ASSERT_LT(m_num, std::numeric_limits<Num>::max() / other.m_denom)
    HARD_ASSERT_LT(other.m_num, std::numeric_limits<Num>::max() / m_denom)
    return (m_num * other.m_denom) > (other.m_num * m_denom);
}

bool util::Frac::operator<=(
    util::Frac const &other) const {
    HARD_ASSERT_LT(m_num, std::numeric_limits<Num>::max() / other.m_denom)
    HARD_ASSERT_LT(other.m_num, std::numeric_limits<Num>::max() / m_denom)
    return (m_num * other.m_denom) <= (other.m_num * m_denom);
}

bool util::Frac::operator>=(
    util::Frac const &other) const {
    HARD_ASSERT_LT(m_num, std::numeric_limits<Num>::max() / other.m_denom)
    HARD_ASSERT_LT(other.m_num, std::numeric_limits<Num>::max() / m_denom)
    return (m_num * other.m_denom) >= (other.m_num * m_denom);
}

bool util::Frac::operator!=(
    util::Frac const &other) const {
    HARD_ASSERT_LT(m_num, std::numeric_limits<Num>::max() / other.m_denom)
    HARD_ASSERT_LT(other.m_num, std::numeric_limits<Num>::max() / m_denom)
    return (m_num * other.m_denom) != (other.m_num * m_denom);
}

bool util::Frac::operator==(util::Frac const &other) const {
    HARD_ASSERT_LT(m_num, std::numeric_limits<Num>::max() / other.m_denom)
    HARD_ASSERT_LT(other.m_num, std::numeric_limits<Num>::max() / m_denom)
    return (m_num * other.m_denom) == (other.m_num * m_denom);
}

void util::Frac::log() const {
    if (m_denom == 1) {
        util::logger << m_num;
    } else {
        util::logger << m_num << "/" << m_denom
                     << util::begin_comment
                     << " (" << static_cast<double>(m_num) / static_cast<double>(m_denom) << ")"
                     << util::end_comment;
    }
}
