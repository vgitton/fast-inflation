#include "inflation.h"
#include "../../util/debug.h"
#include "../../util/logger.h"
#include "../../util/math.h"
#include "../../util/permutations.h"
#include "../inf_problem/tree_filler.h"

void inf::Inflation::log(inf::Inflation::UseDistrSymmetries use_distr_symmetries) {
    util::logger << util::begin_comment << "inf::Inflation::UseDistrSymmetries::"
                 << util::end_comment;
    switch (use_distr_symmetries) {
    case inf::Inflation::UseDistrSymmetries::yes:
        util::logger << "yes";
        break;
    case inf::Inflation::UseDistrSymmetries::no:
        util::logger << "no";
        break;
    default:
        THROW_ERROR("switch")
    }
}

// INFLATION

inf::Inflation::Inflation(inf::TargetDistr::ConstPtr const &target_distr,
                          inf::Inflation::Size const &size,
                          inf::Inflation::UseDistrSymmetries use_distr_symmetries)
    : m_target_distr(target_distr),
      m_size(size),
      m_use_distr_symmetries(use_distr_symmetries),
      m_party_map{},
      m_name_map{},
      m_party_names{},
      m_source_induced_syms{},
      m_inflation_symmetries{},
      m_n_parties{},
      m_symtree(nullptr),
      m_rng(0, m_target_distr->get_network()->get_outcome_last()) {
    Index number_of_sources = 3;
    HARD_ASSERT_EQUAL(size.size(), number_of_sources)
    for (Index source_card : m_size) {
        if (source_card == 0) {
            THROW_ERROR("expected the inflation size to be at least {1,1,1}, got " + util::str(m_size))
        }
    }
    // This also initialize m_n_parties
    init_parties();
    // Needs to be called before init_inflation_symmetries()
    init_source_induced_syms();
    // To intialize the applicable symmetries
    init_inflation_symmetries(m_target_distr->get_sym_group());
}

std::string inf::Inflation::get_metadata() const {
    HARD_ASSERT_EQUAL(m_size.size(), 3)

    std::string ret =
        "Network name: " + m_target_distr->get_network()->get_name() + "; " +
        "Outcomes per party: " + util::str(static_cast<int>(m_target_distr->get_network()->get_n_outcomes())) + "; " +
        "Inflation size: " + util::str(m_size[0]) + "x" + util::str(m_size[1]) + "x" + util::str(m_size[2]) + "; ";

    if (m_use_distr_symmetries == inf::Inflation::UseDistrSymmetries::yes) {
        ret += "Using the " + util::str(m_target_distr->get_sym_group().size()) + " symmetries of the distribution " + m_target_distr->get_short_name() + "; ";
    }

    ret += "The inflation has " + util::str(m_inflation_symmetries.size()) + " symmetries";

    return ret;
}

std::string inf::Inflation::get_symtree_filename() const {
    HARD_ASSERT_EQUAL(m_size.size(), 3)
    return "data/symtree_" + m_target_distr->get_short_name() + "_" + util::str(m_size[0]) + util::str(m_size[1]) + util::str(m_size[2]);
}

void inf::Inflation::log() const {
    LOG_BEGIN_SECTION("inf::Inflation")

    util::logger << get_metadata() << util::cr;

    LOG_END_SECTION
}

void inf::Inflation::log_event(
    inf::Event const &event) const {
    ASSERT_EQUAL("Triangle network", m_target_distr->get_network()->get_name());
    ASSERT_EQUAL(event.size(), get_n_parties());

    std::vector<std::string> event_representation(this->get_n_parties());

    for (Index i : util::Range(event.size())) {
        event_representation[i] = m_target_distr->get_network()->outcome_to_str(event[i]);
    }

    this->log_event_strings(event_representation);
}

std::string inf::Inflation::get_marginal_name(std::vector<Index> const &marginal) const {
    std::string ret = "q(";
    bool first = true;
    for (Index party : marginal) {
        if (not first) {
            ret += ", ";
        } else {
            first = false;
        }
        ret += get_party_name(party);
    }
    ret += ")";
    return ret;
}

void inf::Inflation::log_marginal(std::vector<Index> const &marginal) const {
    ASSERT_EQUAL("Triangle network", m_target_distr->get_network()->get_name());
    ASSERT_LT(marginal.size(), get_n_parties() + 1);

    char const dot_in_ascii = 46;
    std::vector<std::string> marginal_representation(this->get_n_parties(), std::string(1, dot_in_ascii));

    for (Index i : util::Range(marginal.size())) {
        marginal_representation[marginal[i]] = util::logger.get_single_char(i);
    }

    this->log_event_strings(marginal_representation);
}

void inf::Inflation::log_event_strings(std::vector<std::string> const &event) const {
    Index const alice = 0;
    Index const bob = 1;
    Index const charlie = 2;

    Index const n_alpha = m_size[0];
    Index const n_beta = m_size[1];
    Index const n_gamma = m_size[2];

    util::logger << m_target_distr->get_network()->get_party_names()[charlie] << " ";
    for (Index beta : util::Range(n_beta)) {
        (void)beta;
        util::logger << "  ";
    }

    util::logger << m_target_distr->get_network()->get_party_names()[bob] << util::cr;
    for (Index alpha : util::Range(n_alpha)) {
        for (Index beta : util::Range(n_beta)) {
            util::logger
                << event[m_party_map.at(inf::Inflation::Party(charlie, {alpha, beta}))]
                << " ";
        }

        util::logger << "  ";

        for (Index gamma : util::Range(n_gamma)) {
            util::logger
                << event[m_party_map.at(inf::Inflation::Party(bob, {gamma, alpha}))]
                << " ";
        }

        util::logger << util::cr;
    }

    util::logger << m_target_distr->get_network()->get_party_names()[alice] << util::cr;
    for (Index gamma : util::Range(n_gamma)) {
        for (Index beta : util::Range(n_beta)) {
            util::logger
                << event[m_party_map.at(inf::Inflation::Party(alice, {beta, gamma}))]
                << " ";
        }

        util::logger << util::cr;
    }
}

inf::Event inf::Inflation::get_all_zero_event() const {
    return inf::Event(m_n_parties, 0);
}

inf::Event inf::Inflation::get_random_event() const {
    inf::Event ret(this->get_n_parties(), 0);
    for (inf::Outcome &outcome : ret) {
        outcome = m_rng.get_rand();
    }
    return ret;
}

inf::Event inf::Inflation::get_all_unknown_event() const {
    return inf::Event(m_n_parties, this->get_network()->get_outcome_unknown());
}

inf::EventRange inf::Inflation::get_event_range() const {
    return inf::EventRange(get_n_parties(), m_target_distr->get_network()->get_n_outcomes());
}

util::ProductRange<Index> inf::Inflation::get_all_source_range() const {
    return util::ProductRange(m_size);
}

std::vector<inf::Inflation::Party> inf::Inflation::get_parties() const {
    return m_parties;
}

Index inf::Inflation::get_party_index(
    inf::Inflation::Party const &party) const {
    ASSERT_MAP_CONTAINS(m_party_map, party);

    return m_party_map.at(party);
}

Index inf::Inflation::get_party_index(std::string const &party) const {
    ASSERT_MAP_CONTAINS(m_name_map, party)

    return m_name_map.at(party);
}

std::string const &inf::Inflation::get_party_name(Index party) const {
    ASSERT_LT(party, m_n_parties)

    return m_party_names[party];
}

bool inf::Inflation::is_valid_party_name(std::string const &party) const {
    return m_name_map.contains(party);
}

inf::Inflation::Size const &inf::Inflation::get_size() const {
    return m_size;
}

inf::Network::ConstPtr const &inf::Inflation::get_network() const {
    return m_target_distr->get_network();
}

inf::Symmetry::Group const &inf::Inflation::get_inflation_symmetries() const {
    return m_inflation_symmetries;
}

bool inf::Inflation::has_symmetries_compatible_with(inf::TargetDistr const &d) const {
    if (m_use_distr_symmetries == inf::Inflation::UseDistrSymmetries::no)
        return true;
    else
        return d.get_sym_group() == m_target_distr->get_sym_group();
}

std::vector<inf::PartySym> const &inf::Inflation::get_source_induced_syms() const {
    return m_source_induced_syms;
}

inf::PartySym inf::Inflation::source_sym_to_party_sym(std::vector<std::vector<Index>> const &perm_sources) const {
    // Initialize to values that are known to be too high to check at the end that we initialized each entry
    inf::PartySym::Bare party_sym(this->get_n_parties(), this->get_n_parties() + 1);
    Index const network_n_parties = m_target_distr->get_network()->get_n_parties();

    for (Index const network_party_index : util::Range(network_n_parties)) {
        std::vector<Index> const &perm_left = perm_sources[(network_party_index + 1) % network_n_parties];
        std::vector<Index> const &perm_right = perm_sources[(network_party_index + 2) % network_n_parties];

        for (Index s_left : util::Range(perm_left.size())) {
            for (Index s_right : util::Range(perm_right.size())) {
                // Reasoning: for {0,1,2} -> {1,2,0}, we want to store
                // {1,2,0}, i.e., {sigma(0), sigma(1), sigma(2)}
                party_sym[this->get_party_index(inf::Inflation::Party(
                    network_party_index, {s_left, s_right}))] =
                    this->get_party_index(inf::Inflation::Party(
                        network_party_index, {perm_left[s_left], perm_right[s_right]}));
            }
        }
    }

    DEBUG_RUN(
        for (Index party_sym_element
             : party_sym){
            ASSERT_LT(party_sym_element, this->get_n_parties() + 1)})

    return inf::PartySym(party_sym);
}

inf::PartySym inf::Inflation::network_party_to_inf_party_sym(
    inf::PartySym const &network_party_sym) const {
    inf::PartySym::Bare inf_sym(this->get_n_parties(), this->get_n_parties() + 1);
    inf::PartySym::Bare net_sym(network_party_sym.get_bare_sym());

    for (Index net_index : util::Range(net_sym.size())) {
        for (Index s_left : util::Range(m_size[(net_index + 1) % 3])) {
            for (Index s_right : util::Range(m_size[(net_index + 2) % 3])) {
                inf::Inflation::Sources s_image({s_left, s_right});
                if (not network_party_sym.is_even()) {
                    // Transpose if odd permutation
                    // Tr[ABC] = Tr[B^T A^T C^T]
                    s_image = {s_right, s_left};
                }

                inf_sym[this->get_party_index(inf::Inflation::Party(
                    net_index, {s_left, s_right}))] = this->get_party_index(inf::Inflation::Party(net_sym[net_index], s_image));
            }
        }
    }

    DEBUG_RUN(
        for (Index inf_sym_element
             : inf_sym){
            ASSERT_LT(inf_sym_element, this->get_n_parties() + 1)})

    return inf::PartySym(inf_sym);
}

inf::EventTree const &inf::Inflation::get_symtree(inf::EventTree::IO io) const {
    if (m_symtree == nullptr) {
        m_symtree = std::make_unique<inf::EventTree>(m_n_parties);

        if (io == inf::EventTree::IO::read) {
            util::logger << "Reading the symmetrized event tree..." << util::cr;

            util::InputFileStream stream(get_symtree_filename(), util::FileStream::Format::text, get_metadata());
            stream.io(*m_symtree);
        } else {
            util::logger << "Filling the symmetrized event tree..." << util::cr;

            inf::TreeFiller::fill_tree(*m_symtree, *this);

            if (io == inf::EventTree::IO::write) {
                util::logger << "Saving the symmetrized event tree..." << util::cr;

                util::OutputFileStream stream(get_symtree_filename(), util::FileStream::Format::text, get_metadata());
                stream.io(*m_symtree);
            }
        }
    }

    return *m_symtree;
}

bool inf::Inflation::are_d_separated(std::vector<Index> const &marg_1,
                                     std::vector<Index> const &marg_2) const {
    std::set<inf::Inflation::Source> const parents_1 = get_parents(marg_1);
    std::set<inf::Inflation::Source> const parents_2 = get_parents(marg_2);

    for (inf::Inflation::Source const &parent_1 : parents_1) {
        // If at least one source is a common parent, then the two marginals are not d-separated
        if (parents_2.contains(parent_1))
            return false;
    }

    return true;
}

std::set<inf::Inflation::Source> inf::Inflation::get_parents(std::vector<Index> const &marg) const {
    // This will be returned
    std::set<inf::Inflation::Source> parents;

    for (Index const party_index : marg) {
        inf::Inflation::Party const &party = m_parties[party_index];
        parents.insert(inf::Inflation::Source((party.first + 1) % 3, party.second[0]));
        parents.insert(inf::Inflation::Source((party.first + 2) % 3, party.second[1]));
    }

    return parents;
}

bool inf::Inflation::is_injectable_set(std::vector<Index> const &marg) const {
    // The empty set is not considered injectable
    if (marg.size() == 0)
        return false;

    std::set<Index> network_parties;
    for (Index const type : util::Range(get_network()->get_n_parties()))
        network_parties.insert(get_party_index(inf::Inflation::Party(type, {0, 0})));

    std::vector<Index> transformed_marg(marg.size());
    for (inf::PartySym const &source_sym : get_source_induced_syms()) {
        source_sym.act_entrywise(marg, transformed_marg);

        bool transformed_marg_in_network_parties = true;
        for (Index const transformed_party : transformed_marg) {
            if (not network_parties.contains(transformed_party)) {
                transformed_marg_in_network_parties = false;
                break;
            }
        }

        if (transformed_marg_in_network_parties)
            return true;
    }

    return false;
}

// Private methods

void inf::Inflation::init_parties() {
    // First, the inflation size steps

    std::vector<inf::Inflation::Size> inf_size_steps = {{1, 1, 1}};
    inf::Inflation::Size const &final_inf_size = m_size;

    while (inf_size_steps.back() != final_inf_size) {
        // Copy here
        inf::Inflation::Size current_inf_size = inf_size_steps.back();

        for (Index source_i : util::Range(final_inf_size.size())) {
            if (current_inf_size[source_i] < final_inf_size[source_i]) {
                current_inf_size[source_i] += 1;
                inf_size_steps.push_back(current_inf_size);
            }
        }
    }

    Index party_index = 0;
    for (inf::Inflation::Size const &inf_size : inf_size_steps) {
        for (inf::Inflation::Sources const &sources : util::ProductRange(inf_size)) {
            for (Index network_party_i : this->get_network()->get_party_range()) {
                Index left = sources[(network_party_i + 1) % 3];
                Index right = sources[(network_party_i + 2) % 3];

                inf::Inflation::Party party(network_party_i, {left, right});

                if (not m_party_map.contains(party)) {
                    m_party_map[party] = party_index;
                    m_parties.push_back(party);

                    std::string name = m_target_distr->get_network()->get_party_names()[network_party_i] + util::str(left) + util::str(right);
                    m_name_map[name] = party_index;
                    m_party_names.push_back(name);

                    ++party_index;
                }
            }
        }
    }

    m_n_parties = party_index;

    ASSERT_EQUAL(party_index, get_n_parties())
    ASSERT_EQUAL(m_party_map.size(), get_n_parties())
    ASSERT_EQUAL(m_name_map.size(), get_n_parties())
    ASSERT_EQUAL(m_party_names.size(), get_n_parties())
}

void inf::Inflation::init_source_induced_syms() {
    ASSERT_EQUAL(m_source_induced_syms.size(), 0)

    for (std::vector<Index> perm_alpha : util::Permutations(m_size[0])) {
        for (std::vector<Index> perm_beta : util::Permutations(m_size[1])) {
            for (std::vector<Index> perm_gamma : util::Permutations(m_size[2])) {
                std::vector<std::vector<Index>> const perm_sources({perm_alpha, perm_beta, perm_gamma});

                m_source_induced_syms.push_back(this->source_sym_to_party_sym(perm_sources));
            }
        }
    }

    m_source_induced_syms.shrink_to_fit();
}

void inf::Inflation::init_inflation_symmetries(inf::Symmetry::Group const &distribution_sym_group) {
    // This is only valid for the triangle
    HARD_ASSERT_EQUAL(m_size.size(), 3)
    // Should always have at least the identity
    HARD_ASSERT_LT(0, distribution_sym_group.size())

    // std::vector<Index> applicable_distribution_syms;
    inf::Symmetry::Group applicable_distribution_syms;

    switch (m_use_distr_symmetries) {
    case inf::Inflation::UseDistrSymmetries::yes:
        applicable_distribution_syms = inf::Inflation::get_applicable_symmetries(distribution_sym_group,
                                                                                 m_size);
        break;

    case inf::Inflation::UseDistrSymmetries::no:
        applicable_distribution_syms = m_target_distr->get_network()->get_trivial_symmetry_group();
        break;

    default:
        THROW_ERROR("switch")
    }

    // An extra check
    DEBUG_RUN(
        Index const expected_size = applicable_distribution_syms.size() * util::factorial(m_size[0]) * util::factorial(m_size[1]) * util::factorial(m_size[2]);)

    // All the party symmetries induced by inflation source exchanges
    std::vector<inf::PartySym> const &source_syms = this->get_source_induced_syms();

    for (inf::Symmetry const &network_sym : applicable_distribution_syms) {
        inf::PartySym const inf_party_sym = this->network_party_to_inf_party_sym(network_sym.get_party_sym());
        inf::OutcomeSym const &outcome_sym = network_sym.get_outcome_sym();

        for (inf::PartySym const &source_induced_party_sym : source_syms) {
            // It is here equivalent to take the left or right product,
            // i.e., individual elements are different but the whole group
            // that one obtains is the same
            inf::PartySym combined_party_sym = source_induced_party_sym.get_composition_after(inf_party_sym);

            DEBUG_RUN(auto status =)
            m_inflation_symmetries.insert(inf::Symmetry(combined_party_sym, outcome_sym));
            ASSERT_TRUE(status.second)
        }
    }

    ASSERT_EQUAL(m_inflation_symmetries.size(), expected_size)
}

inf::Symmetry::Group inf::Inflation::get_applicable_symmetries(inf::Symmetry::Group const &distribution_sym_group,
                                                               inf::Inflation::Size const &size) {
    inf::Symmetry::Group applicable_distribution_syms;

    // This works for the triangle networks, but not for general networks
    // The point is that exchanging parties is equivalent to exchanging sources
    // in a very similar way

    inf::Inflation::Size image_size(3, 0);

    for (inf::Symmetry const &network_sym : distribution_sym_group) {
        network_sym.get_party_sym().act_on_list(size, image_size);

        if (size == image_size) {
            DEBUG_RUN(auto status =)
            applicable_distribution_syms.insert(network_sym);

            ASSERT_TRUE(status.second)
        }
    }

    return applicable_distribution_syms;
}
