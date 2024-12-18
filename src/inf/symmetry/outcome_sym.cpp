#include "outcome_sym.h"
#include "../../util/debug.h"
#include "../../util/logger.h"
#include "../../util/math.h"

inf::OutcomeSym::OutcomeSym(inf::OutcomeSym::Bare const &sym)
    : m_sym(sym),
      m_inverse_sym(sym.size(), 0) {

    for (inf::Outcome const before_perm : util::Range(static_cast<inf::Outcome>(m_sym.size()))) {
        m_inverse_sym[m_sym[before_perm]] = before_perm;
    }
}

void inf::OutcomeSym::act_on_event(inf::Event const &input, inf::Event &output) const {
    ASSERT_EQUAL(input.size(), output.size());

    for (Index i : util::Range(input.size())) {
        output[i] = m_sym[input[i]];
    }
}

inf::OutcomeSym inf::OutcomeSym::get_composition_after(inf::OutcomeSym const &other) const {
    ASSERT_EQUAL(this->m_sym.size(), other.m_sym.size())

    inf::OutcomeSym::Bare composed_sym(this->m_sym.size());

    for (Index i : util::Range(m_sym.size())) {
        composed_sym[i] = this->m_sym[other.m_sym[i]];
    }

    return inf::OutcomeSym(composed_sym);
}

inf::OutcomeSym inf::OutcomeSym::get_inverse() const {
    return inf::OutcomeSym(m_inverse_sym);
}

void inf::OutcomeSym::log() const {
    util::logger << "inf::OutcomeSym (";
    for (inf::Outcome permuted : m_sym) {
        util::logger.echo_colored_number(permuted);
    }
    util::logger << ")";
}

bool inf::OutcomeSym::operator==(inf::OutcomeSym const &other) const {
    return this->get_bare_sym() == other.get_bare_sym();
}

bool inf::OutcomeSym::operator<(inf::OutcomeSym const &other) const {
    return this->get_bare_sym() < other.get_bare_sym();
}

bool inf::OutcomeSym::is_trivial() const {
    for (Index i : util::Range(m_sym.size())) {
        if (m_sym[i] != i)
            return false;
    }
    return true;
}
