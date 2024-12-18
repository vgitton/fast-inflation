#include "network.h"
#include "../../util/debug.h"
#include "../../util/logger.h"
#include "../../util/math.h"
#include "../../util/permutations.h"

inf::Network::ConstPtr inf::Network::create_triangle(inf::Outcome n_outcomes) {
    return std::make_shared<const inf::Network>(
        "Triangle network",
        3, std::vector<std::string>({"A", "B", "C"}), n_outcomes);
}

inf::Network::Network(
    std::string const &name,
    Index n_parties,
    std::vector<std::string> const &party_names,
    inf::Outcome n_outcomes)
    : m_name(name),
      m_n_parties(n_parties),
      m_party_names(party_names),
      m_party_name_map(),
      m_n_outcomes(n_outcomes),
      m_outcome_last(static_cast<inf::Outcome>(n_outcomes - 1)),
      m_outcome_unknown(n_outcomes) {
    HARD_ASSERT_EQUAL(n_parties, party_names.size())
    HARD_ASSERT_LT(1, n_outcomes)

    for (Index party_i : util::Range(party_names.size())) {
        HARD_ASSERT_EQUAL(party_names[party_i].size(), 1)

        bool party_name_is_new = m_party_name_map.insert(std::make_pair(party_names[party_i], party_i)).second;

        HARD_ASSERT_TRUE(party_name_is_new)
    }
}

void inf::Network::log() const {
    LOG_BEGIN_SECTION("inf::Network")

    util::logger
        << "Name:   " << m_name << util::cr
        << "Parties: ";
    for (std::string const &party_name : get_party_names()) {
        util::logger << party_name << " ";
    }
    util::logger << "with " << static_cast<Num>(get_n_outcomes()) << " outcomes per party." << util::cr;
    util::logger << util::end_section;
}

void inf::Network::log_event(inf::Event const &e) const {
    ASSERT_EQUAL(e.size(), get_n_parties());

    bool first = true;
    for (Index p : get_party_range()) {
        bool show_detailed_names = false;
        if (show_detailed_names) {
            if (first) {
                first = false;
            } else {
                util::logger << " ";
            }
            util::logger << m_party_names[p] << ":";
            log_outcome(e[p]);
        } else {
            log_outcome(e[p]);
        }
    }
}

std::string inf::Network::outcome_to_str(inf::Outcome outcome) const {
    if (outcome <= get_outcome_last()) {
        return util::logger.get_colored_number_str(outcome);
    } else if (outcome == get_outcome_unknown()) {
        return "?";
    } else {
        THROW_ERROR("The outcome " + util::str(int(outcome)) + " is outside the expected range")
    }
}
void inf::Network::log_outcome(inf::Outcome outcome) const {
    util::logger << outcome_to_str(outcome);
}

Index inf::Network::get_n_events() const {
    return util::pow(static_cast<Index>(m_n_outcomes), m_n_parties);
}

inf::EventRange inf::Network::get_event_range() const {
    return inf::EventRange(get_n_parties(), get_n_outcomes());
}

std::vector<inf::PartySym> inf::Network::get_all_party_sym() const {
    HARD_ASSERT_EQUAL(get_n_parties(), 3);
    HARD_ASSERT_EQUAL(get_name(), "Triangle network");

    return {
        inf::PartySym({0, 1, 2}, true),
        inf::PartySym({0, 2, 1}, false),
        inf::PartySym({1, 0, 2}, false),
        inf::PartySym({1, 2, 0}, true),
        inf::PartySym({2, 0, 1}, true),
        inf::PartySym({2, 1, 0}, false)};
}

std::vector<inf::OutcomeSym> inf::Network::get_all_outcome_sym() const {
    std::vector<inf::OutcomeSym> ret(
        util::factorial<inf::Outcome>(get_n_outcomes()),
        inf::OutcomeSym({}));

    Index perm_index = 0;
    for (std::vector<inf::Outcome> const &bare_outcome_perm : util::Permutations(get_n_outcomes())) {
        ret[perm_index] = inf::OutcomeSym(bare_outcome_perm);
        ++perm_index;
    }

    ASSERT_EQUAL(perm_index, ret.size());

    return ret;
}

inf::Symmetry::Group inf::Network::get_all_sym() const {
    std::vector<inf::PartySym> party_syms = get_all_party_sym();
    std::vector<inf::OutcomeSym> outcome_syms = get_all_outcome_sym();

    inf::Symmetry::Group ret;

    for (inf::PartySym const &party_sym : party_syms) {
        for (inf::OutcomeSym const &outcome_sym : outcome_syms) {
            DEBUG_RUN(auto status =)
            ret.insert(inf::Symmetry(party_sym, outcome_sym));

            ASSERT_TRUE(status.second)
        }
    }

    return ret;
}

inf::Symmetry::Group inf::Network::get_trivial_symmetry_group() const {
    inf::Symmetry::Group ret;

    inf::PartySym::Bare party_sym(this->get_n_parties());
    for (Index party : util::Range(this->get_n_parties())) {
        party_sym[party] = party;
    }

    inf::OutcomeSym::Bare outcome_sym(this->get_n_outcomes());
    for (inf::Outcome outcome : util::Range(this->get_n_outcomes())) {
        outcome_sym[outcome] = outcome;
    }

    DEBUG_RUN(auto status =)
    ret.insert(
        inf::Symmetry(
            inf::PartySym(party_sym),
            inf::OutcomeSym(outcome_sym)));

    ASSERT_TRUE(status.second)

    return ret;
}
