#include "party_sym.h"
#include "../../util/debug.h"
#include "../../util/logger.h"

inf::PartySym::PartySym(
    inf::PartySym::Bare const &sym, bool even)
    : m_sym(sym),
      m_inverse_sym(sym.size(), 0),
      m_even(even) {
    for (Index const before_perm : util::Range(m_sym.size())) {
        m_inverse_sym[m_sym[before_perm]] = before_perm;
    }
}

void inf::PartySym::act_entrywise(std::vector<Index> const &input, std::vector<Index> &output) const {
    ASSERT_EQUAL(input.size(), output.size())

    for (Index const i : util::Range(input.size())) {
        output[i] = m_sym[input[i]];
    }
}

template <typename T>
void inf::PartySym::act_on_list(std::vector<T> const &input, std::vector<T> &output) const {
    ASSERT_TRUE(&input != &output);
    ASSERT_EQUAL(input.size(), output.size());
    ASSERT_EQUAL(input.size(), m_sym.size());

    for (Index const i : util::Range(m_sym.size())) {
        output[m_sym[i]] = input[i];
    }
}

template void inf::PartySym::act_on_list<inf::Outcome>(inf::Event const &source, inf::Event &dest) const;
template void inf::PartySym::act_on_list<Index>(std::vector<Index> const &source, std::vector<Index> &dest) const;

inf::PartySym inf::PartySym::get_composition_after(inf::PartySym const &other) const {
    ASSERT_EQUAL(this->m_sym.size(), other.m_sym.size());

    inf::PartySym::Bare composed_sym(this->m_sym.size());

    for (Index const i : util::Range(m_sym.size())) {
        composed_sym[i] = this->m_sym[other.m_sym[i]];
    }

    return inf::PartySym(composed_sym);
}

void inf::PartySym::log() const {
    util::logger << "inf::PartySym " << m_sym;
}

bool inf::PartySym::operator==(inf::PartySym const &other) const {
    return this->get_bare_sym() == other.get_bare_sym();
}

bool inf::PartySym::operator<(inf::PartySym const &other) const {
    return this->get_bare_sym() < other.get_bare_sym();
}
