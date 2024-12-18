#include "symmetry.h"

#include "../../util/debug.h"
#include "../../util/logger.h"

inf::Symmetry::Symmetry()
    : m_party_sym({}),
      m_outcome_sym({}) {}

inf::Symmetry::Symmetry(inf::PartySym const &party_sym, inf::OutcomeSym const &outcome_sym)
    : m_party_sym(party_sym),
      m_outcome_sym(outcome_sym) {}

void inf::Symmetry::act_on_event(inf::Event const &input, inf::Event &output) const {
    m_party_sym.act_on_list(input, output);
    m_outcome_sym.act_on_event(output, output);
}

inf::Symmetry inf::Symmetry::get_composition_after(inf::Symmetry const &other) const {
    return inf::Symmetry(
        this->get_party_sym().get_composition_after(other.get_party_sym()),
        this->get_outcome_sym().get_composition_after(other.get_outcome_sym()));
}

void inf::Symmetry::log() const {
    util::logger << "inf::Symmetry { " << m_party_sym << ", " << m_outcome_sym << " }";
}

bool inf::Symmetry::operator==(inf::Symmetry const &other) const {
    return (m_party_sym == other.m_party_sym) and (m_outcome_sym == other.m_outcome_sym);
}

bool inf::Symmetry::operator<(inf::Symmetry const &other) const {
    if (this->get_outcome_sym() == other.get_outcome_sym()) {
        return this->get_party_sym() < other.get_party_sym();
    } else {
        return this->get_outcome_sym() < other.get_outcome_sym();
    }
}
