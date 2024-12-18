#include "constraint_parser.h"

inf::ConstraintParser::ConstraintParser(inf::Inflation::ConstPtr const &inflation,
                                        inf::Constraint::Description const &description)
    : m_lhs_marginal_parties{},
      m_target_marginal_names{},
      m_rhs_marginal_parties{} {

    if (description.size() < 2)
        THROW_ERROR("Expected the inf::Constraint::Description to have at least two elements, got " + util::str(description))

    // This is equivalent to description but splitting each inflation marginal
    // from "A00,A01,..." to {index of A00,index of A01,...}
    // This checks that each marginal is really a set (no duplicates in the list)
    // and that all the agent names are valid
    std::vector<std::vector<Index>> description_as_indices;
    for (std::string const &inf_marg_name : description) {
        std::vector<Index> const inf_marg_indices = inf::ConstraintParser::parse_inflation_marginal(*inflation, inf_marg_name);

        std::set<Index> set_of_marg_indices(inf_marg_indices.begin(), inf_marg_indices.end());
        if (set_of_marg_indices.size() != inf_marg_indices.size())
            THROW_ERROR("Expected a set of inflation parties but got duplicates in the marginal " + inf_marg_name)

        description_as_indices.push_back(inf_marg_indices);
    }

    // Here check d-separation
    for (Index const marg_index_1 : util::Range(description.size())) {
        for (Index const marg_index_2 : util::Range(marg_index_1 + 1, description.size())) {
            if (not inflation->are_d_separated(description_as_indices[marg_index_1], description_as_indices[marg_index_2]))
                THROW_ERROR("Expected the marginal " + description[marg_index_1] + " to be d-separated from the marginal " + description[marg_index_2])
        }
    }

    // Here check injectable sets, which also checks that they are non-empty
    for (Index const marg_index : util::Range(description.size() - 1)) {
        if (not inflation->is_injectable_set(description_as_indices[marg_index]))
            THROW_ERROR("Expected the marginal " + description[marg_index] + " to be an injectable set")
    }

    // Append all the parties together to form the left-hand side marginal
    for (std::vector<Index> const &inf_marg_indices : description_as_indices) {
        for (Index const party_index : inf_marg_indices)
            m_lhs_marginal_parties.push_back(party_index);
    }

    // The right-hand side marginal just consists of the last inflation marginal passed in the description
    m_rhs_marginal_parties = description_as_indices.back();

    // For all except the last inflation marginal, extract a target distribution marginal, throwing away copy indices
    m_target_marginal_names = std::vector<inf::TargetDistr::MarginalName>(description.size() - 1);
    for (Index const marg_index : util::Range(description.size() - 1)) {
        m_target_marginal_names[marg_index] = inf::TargetDistr::MarginalName(description_as_indices[marg_index].size());

        for (Index const party_index : util::Range(description_as_indices[marg_index].size())) {
            std::string const network_party_name(1, inflation->get_party_name(description_as_indices[marg_index][party_index])[0]);
            HARD_ASSERT_MAP_CONTAINS(inflation->get_network()->get_party_name_map(), network_party_name)
            m_target_marginal_names[marg_index][party_index] = network_party_name;
        }
    }
}

std::vector<Index> inf::ConstraintParser::parse_inflation_marginal(inf::Inflation const &inflation,
                                                                   std::string const &name) {
    // Will be returned
    std::vector<Index> marg_indices;

    std::string stripped_name = util::remove_spaces(name);

    if (stripped_name.size() == 0)
        return marg_indices;

    std::istringstream stream(stripped_name);
    std::string party_string;

    while (std::getline(stream, party_string, ',')) {
        if (party_string.size() == 0)
            THROW_ERROR("Could not parse the marginal name \"" + name + "\", is there an extra comma in there?")

        if (not inflation.is_valid_party_name(party_string))
            THROW_ERROR("The inflation marginal \"" + name + "\" contains " + party_string + ", which is not a valid party name ")

        marg_indices.push_back(inflation.get_party_index(party_string));
    }

    return marg_indices;
}

std::vector<Index> inf::ConstraintParser::get_lhs_marg_parties() const {
    return m_lhs_marginal_parties;
}

std::vector<Index> inf::ConstraintParser::get_rhs_marg_parties() const {
    return m_rhs_marginal_parties;
}

std::vector<inf::TargetDistr::MarginalName> inf::ConstraintParser::get_target_marg_names() const {
    return m_target_marginal_names;
}
