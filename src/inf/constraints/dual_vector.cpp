#include "dual_vector.h"
#include "../../util/debug.h"
#include "../../util/logger.h"
#include "../../util/math.h"
#include "marginal.h"

// For std::count, std::find
#include <algorithm>

void inf::DualVector::log(inf::DualVector::StoreBounds store_bounds) {
    util::logger << util::begin_comment << "inf::DualVector::StoreBounds::" << util::end_comment;
    if (store_bounds == inf::DualVector::StoreBounds::yes) {
        util::logger << "yes";
    } else if (store_bounds == inf::DualVector::StoreBounds::no) {
        util::logger << "no";
    } else {
        THROW_ERROR("switch")
    }
}

inf::Outcome inf::DualVector::get_outcomes_per_party(inf::Outcome n_outcomes, inf::DualVector::StoreBounds store_bounds) {
    if (store_bounds == inf::DualVector::StoreBounds::yes)
        ++n_outcomes;

    return n_outcomes;
}

std::vector<Index> inf::DualVector::compute_weights(Index n_parties, inf::Outcome n_outcomes, inf::DualVector::StoreBounds store_bounds) {
    return inf::EventTensor::compute_weights(n_parties, inf::DualVector::get_outcomes_per_party(n_outcomes, store_bounds));
}

// Constructor

inf::DualVector::DualVector(inf::Marginal const &marginal,
                            inf::DualVector::BoundType bound_type)
    : inf::TensorWithOrbits(
          marginal.get_n_parties(),
          inf::DualVector::get_outcomes_per_party(marginal.get_inflation()->get_network()->get_n_outcomes(),
                                                  marginal.get_store_bounds())),
      m_inflation(marginal.get_inflation()),
      m_store_bounds(marginal.get_store_bounds()),
      // These are initialized in init_quovec_index_maps()
      m_n_orbits_no_unknown(),
      m_orbit_repr_no_unknown(),
      m_quovec_index_to_orbit{},
      m_event_to_quovec_index{},
      // -----------------------
      m_bound_type(bound_type),
      // These are initialized in init_bound_rules()
      m_bound_rules{} {

    init_orbits(marginal.get_marginal_symmetries());

    init_quovec_index_maps();

    init_bound_rules();
}

// Getters

inf::Inflation::ConstPtr const &inf::DualVector::get_inflation() const {
    return m_inflation;
}

std::vector<inf::QuovecIndex> const &inf::DualVector::get_event_to_quovec_index() const {
    return m_event_to_quovec_index;
}

void inf::DualVector::set_from_quovec(inf::Quovec const &quovec, inf::QuovecIndex start_pos) {
    // If we have a scalar dual_vector, expect a single coefficient
    if (get_n_parties() == 0) {
        // We could in principle allow to read the coefficient in the list,
        // but that's not the use case we have.
        // Feel free to generalize the below if this changes.
        ASSERT_EQUAL(start_pos, 0)
        ASSERT_EQUAL(quovec.size(), 1)

        m_event_tensor.get_num(0) = quovec[0];

        return;
    }

    ASSERT_LT(m_n_orbits_no_unknown, quovec.size() - start_pos + 1);

    // Directly set the orbit coeffs with no unknowns
    for (inf::QuovecIndex const quovec_index : util::Range(m_n_orbits_no_unknown)) {
        set_orbit_coeff(quovec_index, quovec[start_pos + quovec_index]);
    }

    // The rest is only executed when we store the bounds
    if (m_store_bounds == inf::DualVector::StoreBounds::yes) {
        inf::Quovec quovec_with_unknown(get_n_orbits_with_unknown());
        for (Index const i : util::Range(m_n_orbits_no_unknown)) {
            quovec_with_unknown[i] = quovec[start_pos + i];
        }

        for (BoundRule const &bound_rule : m_bound_rules) {

            // Easter egg, this value is unused
            Num bound = 42;

            for (Index i : util::Range(bound_rule.second.size())) {
                Num const potential_bound = quovec_with_unknown[bound_rule.second[i]];

                if (i == 0)
                    bound = potential_bound;
                else
                    bound = min_or_max(bound, potential_bound);
            }

            quovec_with_unknown[bound_rule.first] = bound;

            set_orbit_coeff(bound_rule.first, bound);
        }
    }
}

void inf::DualVector::hard_assert_within_bound(Num const bound) const {
    for (inf::Event const &orbit_repr : get_orbit_repr_no_unknown()) {
        Num const component = get_event_tensor().get_num(orbit_repr);
        if (component >= bound or component <= -bound) {
            THROW_ERROR(
                "The quovec component " + util::str(component) + " is higher then the max safe value, " + util::str(bound) + ".\n" +
                "Suggested fix if the distribution might be nonlocal still:\n" +
                "    - try to decrease the denominator of the target distribution,\n" +
                "    - decrease safety_factor in inf::ConstraintSet::update_constraint_scale_factors()\n" +
                "    - switch to more precise arithmetic types (e.g., use GNU's GMP),\n" +
                "    - do lhs < rhs instead of lhs - rhs < 0 when checking the dual gap to improve by a factor of 2")
        }
    }
}

// Overrides from inf::TensorWithOrbits

std::string inf::DualVector::get_math_name() const {
    return "f";
}

void inf::DualVector::log_header() const {
    LOG_BEGIN_SECTION("inf::DualVector")
}

// Overrides from inf::Orbitable

void inf::DualVector::log_event(inf::Event const &event) const {
    util::logger << "(";
    for (inf::Outcome o : event) {
        m_inflation->get_network()->log_outcome(o);
    }
    util::logger << ")";
}

Index inf::DualVector::get_n_orbits_with_unknown() const {
    return get_n_orbits();
}

Index inf::DualVector::get_n_orbits_no_unknown() const {
    return m_n_orbits_no_unknown;
}

std::vector<inf::Event> const &inf::DualVector::get_orbit_repr_no_unknown() const {
    return m_orbit_repr_no_unknown;
}

// util::Serializable

void inf::DualVector::io(util::FileStream &stream) {

    inf::Quovec quovec;

    if (not stream.is_reading()) {
        quovec.resize(m_n_orbits_no_unknown);
        for (Index const quovec_index : util::Range(m_n_orbits_no_unknown))
            quovec[quovec_index] = m_event_tensor.get_num(m_quovec_index_to_orbit[quovec_index][0]);
    }

    stream.io(quovec);

    if (stream.is_reading()) {
        if (quovec.size() != m_n_orbits_no_unknown)
            THROW_ERROR("Error while reading an inf::DualVector: the input file does not contain the correct number of values")

        set_from_quovec(quovec, 0);
    }
}

// Private methods

void inf::DualVector::init_orbits(inf::Symmetry::Group const &marginal_symmetries) {
    // Set the symmetries depending on the bounds: if we store the bounds,
    // we need to account for the trivial action of the outcome symmetries
    // on the unknown outcome
    if (m_store_bounds == inf::DualVector::StoreBounds::yes) {
        inf::Symmetry::Group const &group_no_unknown = marginal_symmetries;
        inf::Symmetry::Group group_with_unknown;

        const inf::Outcome unknown_outcome = m_inflation->get_network()->get_outcome_unknown();

        for (inf::Symmetry const &sym_no_unknown : group_no_unknown) {
            inf::OutcomeSym::Bare bare_outcome_sym = sym_no_unknown.get_outcome_sym().get_bare_sym();
            // The outcome sym sends unknown_outcome to unknown_outcome
            bare_outcome_sym.push_back(unknown_outcome);

            group_with_unknown.insert(inf::Symmetry(sym_no_unknown.get_party_sym(),
                                                    inf::OutcomeSym(bare_outcome_sym)));
        }

        inf::Orbitable::init_orbits(group_with_unknown);
    } else {
        inf::Orbitable::init_orbits(marginal_symmetries);
    }
}

void inf::DualVector::init_quovec_index_maps() {
    ASSERT_TRUE(orbits_initialized())
    ASSERT_EQUAL(m_quovec_index_to_orbit.size(), 0)
    ASSERT_EQUAL(m_event_to_quovec_index.size(), 0)
    ASSERT_EQUAL(m_n_orbits_no_unknown, 0)
    ASSERT_EQUAL(m_orbit_repr_no_unknown.size(), 0)

    // Init storage
    m_quovec_index_to_orbit = std::vector<std::vector<inf::EventTensor::EventHash>>(
        get_n_orbits(),
        std::vector<inf::EventTensor::EventHash>());

    m_event_to_quovec_index = std::vector<inf::QuovecIndex>(get_n_events());

    inf::QuovecIndex quovec_index = 0;

    for (bool store_unknowns : std::vector<bool>({false, true})) {
        if (store_unknowns)
            m_n_orbits_no_unknown = quovec_index;

        for (inf::Orbitable::OrbitPair const &orbit_pair : get_orbits()) {
            bool contains_unknown = false;
            for (inf::Outcome outcome : orbit_pair.first) {
                if (outcome == m_inflation->get_network()->get_outcome_unknown()) {
                    contains_unknown = true;
                    break;
                }
            }

            // The first pass only stores the orbits with no unknowns, the second round
            // stores only the orbits with unknowns
            if (contains_unknown) {
                if (not store_unknowns)
                    continue;
            } else {
                if (store_unknowns)
                    continue;

                m_orbit_repr_no_unknown.push_back(orbit_pair.first);
            }

            for (inf::Event const &event : orbit_pair.second) {
                inf::EventTensor::EventHash const event_hash = m_event_tensor.get_event_hash(event);

                // Append the hashes of the orbit
                m_quovec_index_to_orbit[quovec_index].push_back(event_hash);

                // Mark the event as pointing to the current orbit
                m_event_to_quovec_index[event_hash] = quovec_index;
            }

            ++quovec_index;
        }
    }

    ASSERT_EQUAL(quovec_index, get_n_orbits());
}

void inf::DualVector::set_orbit_coeff(inf::QuovecIndex quovec_index, Num coeff) {
    ASSERT_TRUE(orbits_initialized())
    ASSERT_LT(quovec_index, get_n_orbits())

    for (inf::EventTensor::EventHash const event_hash : m_quovec_index_to_orbit[quovec_index]) {
        m_event_tensor.get_num(event_hash) = coeff;
    }
}

void inf::DualVector::init_bound_rules() {
    if (m_store_bounds == inf::DualVector::StoreBounds::no)
        return;

    inf::Outcome const unknown_outcome = m_inflation->get_network()->get_outcome_unknown();

    for (inf::Orbitable::OrbitPair const &orbit_pair : get_orbits()) {
        inf::Event const &repr_event = orbit_pair.first;

        // Ensure that repr_event has at least one unknown
        Index const current_n_unknowns = std::count(repr_event.begin(), repr_event.end(), unknown_outcome);
        if (current_n_unknowns == 0)
            continue;

        // Now can append to the update rules
        inf::QuovecIndex const to_update = m_event_to_quovec_index[get_event_tensor().get_event_hash(repr_event)];

        inf::Event other_event = repr_event;
        inf::Event::iterator const outcome_to_modify = std::find(other_event.begin(), other_event.end(), unknown_outcome);

        // This is essentially the orbit of repr_event, but stored as quovec indices
        std::set<inf::QuovecIndex> feasible_set;

        for (inf::Outcome const other_outcome : util::Range(m_inflation->get_network()->get_n_outcomes())) {
            *outcome_to_modify = other_outcome;

            feasible_set.insert(m_event_to_quovec_index[get_event_tensor().get_event_hash(other_event)]);
        }

        std::vector<inf::QuovecIndex> const feasible_vec(feasible_set.begin(), feasible_set.end());

        m_bound_rules.push_back(inf::DualVector::BoundRule(to_update, feasible_vec));
    }
}

Num inf::DualVector::min_or_max(Num const a, Num const b) const {
    if (m_bound_type == inf::DualVector::BoundType::upper)
        return std::max(a, b);
    else
        return std::min(a, b);
}
