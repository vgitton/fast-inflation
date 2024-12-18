#include "constraint.h"
#include "../../util/logger.h"
#include "../../util/math.h"
#include "constraint_parser.h"

std::string inf::Constraint::pretty_description(inf::Constraint::Description description) {
    for (std::string &marg : description)
        marg = util::remove_spaces(marg);

    std::string ret = "q(";
    bool first = true;
    for (Index const marg_i : util::Range(description.size() - 1)) {
        if (not first)
            ret += " , ";
        else
            first = false;
        ret += description[marg_i];
    }

    if (description.back().size() != 0) {
        ret += " , " + description.back();
    }

    ret += ") = ";

    first = true;
    for (Index const marg_i : util::Range(description.size() - 1)) {
        if (not first)
            ret += " * ";
        else
            first = false;
        ret += "p(" + description[marg_i] + ")";
    }

    if (description.back().size() != 0)
        ret += " * q(" + description.back() + ")";

    return ret;
}

inf::Symmetry::Group inf::Constraint::get_constraint_group(inf::Inflation const &inflation,
                                                           std::vector<Index> const &lhs_marg,
                                                           std::vector<Index> const &rhs_marg) {
    ASSERT_LT(0, lhs_marg.size())

    // Will be returned
    inf::Symmetry::Group constraint_group;

    std::set<Index> const B_parties(rhs_marg.begin(), rhs_marg.end());

    std::set<Index> A_parties;
    for (Index const party_index : lhs_marg) {
        if (not B_parties.contains(party_index))
            A_parties.insert(party_index);
    }

    for (inf::Symmetry const &inf_sym : inflation.get_inflation_symmetries()) {
        // inf_sym needs to leave the union A_0, ..., A_{k-1} invariant
        std::set<Index> transformed_A_parties;
        for (Index const party_index : A_parties)
            transformed_A_parties.insert(inf_sym.get_party_sym().get_bare_sym()[party_index]);

        if (transformed_A_parties != A_parties)
            continue;

        // inf_sym also needs to leave B invariant
        std::set<Index> transformed_B_parties;
        for (Index const party_index : B_parties)
            transformed_B_parties.insert(inf_sym.get_party_sym().get_bare_sym()[party_index]);

        if (transformed_B_parties != B_parties)
            continue;

        // inf_sym has the right property, so it goes in the constraint group
        constraint_group.insert(inf_sym);
    }

    return constraint_group;
}

inf::Constraint::Constraint(inf::Inflation::ConstPtr const &inflation,
                            inf::Constraint::Description const &description,
                            inf::DualVector::StoreBounds store_bounds)
    : m_pretty_description(inf::Constraint::pretty_description(description)),
      m_inflation(inflation),
      m_constraint_group(),
      // Marginals
      m_lhs_inflation_marginal(nullptr),
      m_rhs_inflation_marginal(nullptr),
      // Target distribution
      m_target_distribution_fixed(false),
      m_target_marginal_names{},
      m_rhs_target_tensor(nullptr),
      // Dual vectors
      m_lhs_dual_vector(nullptr),
      m_rhs_reduced_dual_vector(nullptr),
      // Scales
      m_lhs_scale(1),
      m_rhs_scale(1) {

    util::logger << "Constructing inf::Constraint..." << util::cr;

    // Note: intializing everything with nullptr to be able to first invoke an inf::ConstraintParser
    // to validate the data, and then initialize all the relevant members

    inf::ConstraintParser parser(inflation, description);

    m_constraint_group = get_constraint_group(*inflation, parser.get_lhs_marg_parties(), parser.get_rhs_marg_parties());

    m_lhs_inflation_marginal = std::make_unique<inf::Marginal>(inflation, parser.get_lhs_marg_parties(), m_constraint_group, store_bounds);
    m_rhs_inflation_marginal = std::make_unique<inf::Marginal>(inflation, parser.get_rhs_marg_parties(), m_constraint_group, store_bounds);

    m_rhs_target_tensor = std::make_unique<inf::EventTensor>(
        parser.get_lhs_marg_parties().size() - parser.get_rhs_marg_parties().size(), m_inflation->get_network()->get_n_outcomes());

    m_target_marginal_names = parser.get_target_marg_names();

    // The dual_vector knows from the inflation marginal whether or not to store bounds.
    // See inf::DualVector for a description of why the bound types are set up in this way.
    m_lhs_dual_vector = std::make_unique<inf::DualVector>(*m_lhs_inflation_marginal, inf::DualVector::BoundType::lower);
    m_rhs_reduced_dual_vector = std::make_unique<inf::DualVector>(*m_rhs_inflation_marginal, inf::DualVector::BoundType::upper);
}

void inf::Constraint::log() const {
    util::Color c = util::Color::red;
    std::vector<util::Color> color_marginal_parties;

    for (inf::TargetDistr::MarginalName const &marg : m_target_marginal_names) {
        for (std::string const &party : marg) {
            (void)party;
            color_marginal_parties.push_back(c);
        }

        switch (c) {
        case util::Color::red:
            c = util::Color::green;
            break;
        case util::Color::green:
            c = util::Color::blue;
            break;
        case util::Color::blue:
            c = util::Color::yellow;
            break;
        case util::Color::yellow:
            c = util::Color::cyan;
            break;
        default:
            THROW_ERROR("Too many factors, need more colors")
        }
    }

    for (Index party : util::Range(m_rhs_inflation_marginal->get_n_parties())) {
        (void)party;
        color_marginal_parties.push_back(c);
    }

    std::vector<std::string> lhs_formatted(m_inflation->get_n_parties(), ".");

    for (Index i : util::Range(m_lhs_inflation_marginal->get_n_parties())) {
        lhs_formatted[m_lhs_inflation_marginal->get_parties()[i]] =
            util::logger.get_color_code(color_marginal_parties[i]) + util::logger.get_single_char(i) + util::logger.get_color_code(util::Color::fg);
    }

    m_inflation->log_event_strings(lhs_formatted);

    util::logger << util::cr << "= ";
    bool first = true;
    for (inf::TargetDistr::MarginalName const &marg : m_target_marginal_names) {
        if (not first)
            util::logger << " * ";
        else
            first = false;

        util::logger << "p" << marg;
    }

    if (m_rhs_inflation_marginal->get_n_parties() != 0) {
        util::logger << " * ";
        util::logger << util::cr << util::cr;

        std::vector<std::string> rhs_formatted(m_inflation->get_n_parties(), ".");
        for (Index i : util::Range(m_rhs_inflation_marginal->get_n_parties())) {
            rhs_formatted[m_rhs_inflation_marginal->get_parties()[i]] =
                util::logger.get_color_code(c) +
                util::logger.get_single_char(
                    m_lhs_inflation_marginal->get_n_parties() - m_rhs_inflation_marginal->get_n_parties() + i) +
                util::logger.get_color_code(util::Color::fg);
        }

        m_inflation->log_event_strings(rhs_formatted);
    } else {
        util::logger << util::cr;
    }
}

// Getters

inf::DualVector const &inf::Constraint::get_dual_vector() const {
    return *m_lhs_dual_vector;
}

Index inf::Constraint::get_quovec_size() const {
    return m_lhs_dual_vector->get_n_orbits_no_unknown();
}

inf::Constraint::EvaluatorPair inf::Constraint::get_marg_evaluators() const {
    inf::Constraint::EvaluatorPair ret(m_lhs_inflation_marginal->get_evaluator(),
                                       m_rhs_inflation_marginal->get_evaluator());

    // LHS
    ret.first.set_dual_vector_reference(&m_lhs_dual_vector->get_event_tensor());
    ret.first.set_scale_reference(&m_lhs_scale);
    // RHS
    ret.second.set_dual_vector_reference(&m_rhs_reduced_dual_vector->get_event_tensor());
    ret.second.set_scale_reference(&m_rhs_scale);

    return ret;
}

std::string const &inf::Constraint::get_pretty_description() const {
    return m_pretty_description;
}

// Arithmetic

Num inf::Constraint::get_lhs_denom() const {
    return m_lhs_inflation_marginal->get_denom();
}

void inf::Constraint::set_lhs_scale(Num scale) {
    ASSERT_LT(0, scale)
    m_lhs_scale = scale;
}

Num inf::Constraint::get_rhs_denom() const {
    return m_rhs_target_tensor->get_denom() * m_rhs_inflation_marginal->get_denom();
}

void inf::Constraint::set_rhs_scale(Num scale) {
    ASSERT_LT(0, scale)
    // The minus sign here is crucial!
    m_rhs_scale = -scale;
}

// Setters

void inf::Constraint::set_target_distribution(inf::TargetDistr const &d) {
    // First, check that the symmetry group of the distribution hasn't changed
    // This is slightly sub-optimal once we start having several Constraints,
    // but this is anyway only ran on changing the target distribution
    // (plus it's very quick)
    if (not m_inflation->has_symmetries_compatible_with(d)) {
        throw inf::Symmetry::SymmetriesHaveChanged();
    }

    m_target_distribution_fixed = true;

    std::vector<inf::EventTensor const *> target_marginals;
    for (inf::TargetDistr::MarginalName const &marginal_name : m_target_marginal_names) {
        target_marginals.push_back(&d.get_marginal(marginal_name));
    }

    m_rhs_target_tensor->set_to_tensor_product(target_marginals);

    update_rhs_reduced_dual_vector();
}

void inf::Constraint::set_dual_vector_from_quovec(inf::Quovec const &coeffs,
                                                  Index start_pos) {
    m_lhs_dual_vector->set_from_quovec(coeffs, start_pos);

    update_rhs_reduced_dual_vector();
}

void inf::Constraint::compute_inflation_event_quovec(inf::Event const &inflation_event,
                                                     inf::Quovec &ret,
                                                     const Index offset) const {
    ASSERT_TRUE(m_target_distribution_fixed)
    ASSERT_EQUAL(inflation_event.size(), m_inflation->get_n_parties())
    ASSERT_LT(get_quovec_size(), ret.size() - offset + 1)
    DEBUG_RUN(
        // We'll be adding to the quovec, so it better be initialized to zero
        for (Index i
             : util::Range(get_quovec_size())){
            ASSERT_EQUAL(ret[offset + i], 0)})

    std::vector<inf::QuovecIndex> const &event_to_quovec_index = m_lhs_dual_vector->get_event_to_quovec_index();

    for (inf::Event const &lhs_marg_event : m_lhs_inflation_marginal->extract_marg_perm_events(inflation_event)) {
        Index const lhs_marg_hash = m_lhs_dual_vector->get_event_tensor().get_event_hash(lhs_marg_event);

        ret[offset + event_to_quovec_index[lhs_marg_hash]] += m_lhs_scale;
    }

    DEBUG_RUN(
        Num sum = 0;
        for (Index i
             : util::Range(get_quovec_size())) {
            sum += ret[offset + i];
        } ASSERT_EQUAL(sum, m_lhs_inflation_marginal->get_denom() * m_lhs_scale))

    if (m_rhs_inflation_marginal->get_n_parties() == 0) {
        for (inf::Event const &target_tensor_event : m_rhs_target_tensor->get_event_range()) {
            // Achtung! The event_to_quovec_index map refers to hashes obtained *with unknowns*.
            // Using the m_rhs_target_tensor.get_event_hash() would not work properly.
            // On the other hand, the m_lhs_dual_vector is aware of unknowns.
            // Note: here there used to be a - sign for the RHS, but now this
            // minus sign is contained in m_rhs_scale (see inf::Constraint::set_rhs_scale())
            inf::EventTensor::EventHash const target_tensor_hash = m_lhs_dual_vector->get_event_tensor().get_event_hash(target_tensor_event);
            ret[offset + event_to_quovec_index[target_tensor_hash]] += m_rhs_target_tensor->get_num(target_tensor_event) * 1 * m_rhs_scale;
        }
    } else {
        inf::Event rhs_total_event(m_lhs_dual_vector->get_n_parties(), 0);

        for (inf::Event const &target_tensor_event : m_rhs_target_tensor->get_event_range()) {
            for (Index i : util::Range(target_tensor_event.size())) {
                rhs_total_event[i] = target_tensor_event[i];
            }

            for (inf::Event const &rhs_marg_event : m_rhs_inflation_marginal->extract_marg_perm_events(inflation_event)) {
                for (Index i : util::Range(rhs_marg_event.size())) {
                    rhs_total_event[target_tensor_event.size() + i] = rhs_marg_event[i];
                }

                Index const rhs_total_hash = m_lhs_dual_vector->get_event_tensor().get_event_hash(rhs_total_event);

                // Note: here there used to be a - sign for the RHS, but now this
                // minus sign is contained in m_rhs_scale (see inf::Constraint::set_rhs_scale())
                ret[offset + event_to_quovec_index[rhs_total_hash]] += m_rhs_target_tensor->get_num(target_tensor_event) * 1 * m_rhs_scale;
            }
        }
    }

    DEBUG_RUN(
        sum = 0;
        for (Index i
             : util::Range(get_quovec_size())) {
            sum += ret[offset + i];
        } ASSERT_EQUAL(sum, 0))
}

// Serialization

void inf::Constraint::io_dual_vector(util::FileStream &stream) {
    stream.write_or_read_and_hard_assert(m_pretty_description);

    stream.io(*m_lhs_dual_vector);

    update_rhs_reduced_dual_vector();
}

// Private methods

void inf::Constraint::update_rhs_reduced_dual_vector() {
    ASSERT_TRUE(m_target_distribution_fixed)

    ASSERT_EQUAL(m_lhs_dual_vector->get_event_tensor().get_denom(), 1)
    ASSERT_EQUAL(m_rhs_reduced_dual_vector->get_event_tensor().get_denom(), 1)

    inf::Quovec rhs_reduced_quovec;

    if (m_rhs_reduced_dual_vector->get_event_tensor().is_scalar()) {
        rhs_reduced_quovec = inf::Quovec(1, 0);

        for (inf::Event const &target_tensor_event : m_rhs_target_tensor->get_event_range()) {
            // Achtung! Need to get the hash of the target tensor (who is always unknown-unaware)
            // from something that is unknown-aware, such a m_lhs_dual_vector.
            rhs_reduced_quovec[0] += m_lhs_dual_vector->get_event_tensor().get_num(target_tensor_event) * m_rhs_target_tensor->get_num(target_tensor_event);
        }
    } else {
        rhs_reduced_quovec = inf::Quovec(m_rhs_reduced_dual_vector->get_n_orbits_no_unknown(), 0);

        inf::Event lhs_dual_vector_event(m_lhs_dual_vector->get_n_parties(), 0);

        // Achtung! Need to loop over the orbits without unknowns only.
        // for(inf::Orbitable::OrbitPair const& orbit_pair : m_rhs_reduced_dual_vector.get_orbits())
        // {
        //     inf::Event const& rhs_dual_vector_event = orbit_pair.first;
        for (inf::Event const &rhs_dual_vector_event : m_rhs_reduced_dual_vector->get_orbit_repr_no_unknown()) {
            for (Index i : util::Range(rhs_dual_vector_event.size())) {
                lhs_dual_vector_event[m_rhs_target_tensor->get_n_parties() + i] = rhs_dual_vector_event[i];
            }

            for (inf::Event const &target_tensor_event : m_rhs_target_tensor->get_event_range()) {
                for (Index i : util::Range(target_tensor_event.size())) {
                    lhs_dual_vector_event[i] = target_tensor_event[i];
                }

                Index const hash_rhs_dual_vector_event = m_rhs_reduced_dual_vector->get_event_tensor().get_event_hash(rhs_dual_vector_event);

                rhs_reduced_quovec[m_rhs_reduced_dual_vector->get_event_to_quovec_index()[hash_rhs_dual_vector_event]] += m_rhs_target_tensor->get_num(target_tensor_event) * m_lhs_dual_vector->get_event_tensor().get_num(lhs_dual_vector_event);
            }
        }
    }

    // Start position = 0
    m_rhs_reduced_dual_vector->set_from_quovec(rhs_reduced_quovec, 0);
}
