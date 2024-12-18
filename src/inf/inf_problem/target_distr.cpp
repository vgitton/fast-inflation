#include "target_distr.h"
#include "../../util/debug.h"
#include "../../util/logger.h"
#include "../../util/math.h"

// Constructor

inf::TargetDistr::TargetDistr(
    std::string const &name,
    std::string const &short_name,
    inf::Network::ConstPtr const &net,
    inf::EventTensor const &data)
    : inf::TensorWithOrbits(
          net->get_n_parties(),
          net->get_n_outcomes()),
      m_name(name),
      m_short_name(short_name),
      m_network(net) {
    this->inf::TensorWithOrbits::init_event_tensor(data);

    HARD_ASSERT_TRUE(inf::TargetDistr::is_probability_distribution(get_event_tensor()));

    this->init_symmetries();
}

// Getters

inf::Network::ConstPtr const &inf::TargetDistr::get_network() const {
    return m_network;
}

inf::EventTensor const &inf::TargetDistr::get_marginal(inf::TargetDistr::MarginalName const &name) const {
    if (name == this->get_network()->get_party_names()) {
        return this->m_event_tensor;
    }

    if (not m_marginals.contains(name)) {
        m_marginals[name] = this->compute_marginal(name);
    }

    return *m_marginals.at(name);
}

std::string const &inf::TargetDistr::get_name() const {
    return m_name;
}

std::string const &inf::TargetDistr::get_short_name() const {
    return m_short_name;
}

// Overrides from inf::TensorWithOrbits

std::string inf::TargetDistr::get_math_name() const {
    return "p";
}

void inf::TargetDistr::log_header() const {
    LOG_BEGIN_SECTION("inf::TargetDistr")

    util::logger << "Name: " << m_name << util::cr;
}

// Overrides from inf::Orbitable

void inf::TargetDistr::log_event(inf::Event const &event) const {
    util::logger << "(";
    m_network->log_event(event);
    util::logger << ")";
}

// Private

bool inf::TargetDistr::is_probability_distribution(inf::EventTensor const &event_tensor) {
    Num norm = 0;

    for (Index hash : event_tensor.get_hash_range()) {
        Num const the_numerator = event_tensor.get_num(hash);

        if (the_numerator < 0)
            return false;

        norm += the_numerator;
    }

    return norm == event_tensor.get_denom();
}

bool inf::TargetDistr::is_symmetrized_by(inf::Symmetry const &network_sym) const {
    inf::Event other_e(this->get_n_parties());
    inf::EventTensor const &data = this->get_event_tensor();

    for (inf::Event const &e : get_event_range()) {
        network_sym.act_on_event(e, other_e);

        if (data.get_num(e) != data.get_num(other_e))
            return false;
    }

    return true;
}

void inf::TargetDistr::init_symmetries() {
    inf::Symmetry::Group sym_group{};

    for (inf::Symmetry const &network_sym : m_network->get_all_sym()) {
        if (this->is_symmetrized_by(network_sym)) {
            DEBUG_RUN(auto status =)
            sym_group.insert(network_sym);
            ASSERT_TRUE(status.second)
        }
    }

    inf::Orbitable::init_orbits(sym_group);
}

inf::EventTensor::UniqueConstPtr inf::TargetDistr::compute_marginal(inf::TargetDistr::MarginalName const &name) const {
    // We don't want to recompute the full distribution as a marginal,
    // this is accounted for already in inf::TargetDistr::get_marginal().
    ASSERT_TRUE(name != this->get_network()->get_party_names())

    // Find the party indices that the name of the marginal refers to
    std::vector<Index> party_indices;
    {
        std::map<std::string, Index> const &party_name_map = this->get_network()->get_party_name_map();

        bool first_party = true;
        Index previous_party_i = 0;

        for (std::string const &party : name) {
            HARD_ASSERT_MAP_CONTAINS(party_name_map, party)
            Index party_i = party_name_map.at(party);
            if (not first_party) {
                HARD_ASSERT_LT(previous_party_i, party_i)
                if (previous_party_i >= party_i)
                    THROW_ERROR("The marginal " + util::str(name) + " is not ordered correctly")
            } else {
                first_party = false;
            }

            party_indices.push_back(party_i);

            previous_party_i = party_i;
        }
    }

    Index const marg_n_parties = name.size();
    inf::EventTensor::UniquePtr marginal = std::make_unique<inf::EventTensor>(marg_n_parties, this->get_network()->get_n_outcomes());
    marginal->set_denom(m_event_tensor.get_denom());
    // Initialize the marginal
    {
        inf::Event marg_e(marg_n_parties, 0);

        for (inf::Event const &full_e : this->get_event_range()) {
            for (Index marg_party_i : util::Range(marg_n_parties)) {
                marg_e[marg_party_i] = full_e[party_indices[marg_party_i]];
            }

            marginal->get_num(marg_e) += m_event_tensor.get_num(full_e);
        }
    }
    marginal->simplify();

    ASSERT_TRUE(is_probability_distribution(*marginal))

    return marginal;
}
