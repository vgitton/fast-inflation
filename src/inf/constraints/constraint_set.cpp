#include "constraint_set.h"
#include "../../util/logger.h"
#include "../../util/math.h"
#include "dual_vector.h"
// For exact arithmetic
#include <gmp.h>

inf::ConstraintSet::ConstraintSet(inf::Inflation::ConstPtr const &inflation,
                                  inf::ConstraintSet::Description const &constraint_set_description,
                                  inf::DualVector::StoreBounds store_bounds)
    : m_inflation(inflation),
      m_constraints{},
      m_store_bounds(store_bounds),
      m_quovec_size(0),
      // Arithmetic
      m_quovec_denom(1.0),
      m_max_dual_vector_component(0) {

    util::logger << "Constructing inf::ConstraintSet..." << util::cr;

    for (inf::Constraint::Description const &constraint_description : constraint_set_description)
        m_constraints.emplace_back(std::make_unique<inf::Constraint>(inflation, constraint_description, store_bounds));

    this->init_quovec_size();
}

// Logging

void inf::ConstraintSet::log() const {
    LOG_BEGIN_SECTION("inf::ConstraintSet")

    bool first = true;
    Index n_constraints = 0;
    for (inf::Constraint::UniquePtr const &constraint : m_constraints) {
        if (not first)
            util::logger << util::cr;
        else
            first = false;

        util::logger << "Constraint with " << constraint->get_quovec_size() << " rows:"
                     << util::cr
                     << *constraint;

        n_constraints += constraint->get_quovec_size();
    }

    util::logger << util::cr << "Total number of constraints: " << n_constraints << util::cr;

    LOG_END_SECTION
}

void inf::ConstraintSet::log_dual_vectors() const {
    LOG_BEGIN_SECTION_FUNC

    for (inf::Constraint::UniquePtr const &constraint : m_constraints) {
        util::logger << constraint->get_dual_vector() << util::cr;
    }

    LOG_END_SECTION
}

// Getters

inf::Inflation::ConstPtr const &inf::ConstraintSet::get_inflation() const {
    return m_inflation;
}

inf::DualVector::StoreBounds inf::ConstraintSet::get_store_bounds() const {
    return m_store_bounds;
}

Index inf::ConstraintSet::get_quovec_size() const {
    return m_quovec_size;
}

Num inf::ConstraintSet::get_max_dual_vector_component() const {
    return m_max_dual_vector_component;
}

inf::Marginal::EvaluatorSet inf::ConstraintSet::get_marg_evaluators() const {
    std::vector<inf::Marginal::Evaluator> evaluators{};

    for (inf::Constraint::UniquePtr const &constraint : m_constraints) {
        inf::Constraint::EvaluatorPair evaluator_pair = constraint->get_marg_evaluators();
        evaluators.emplace_back(evaluator_pair.first);
        evaluators.emplace_back(evaluator_pair.second);
    }

    return inf::Marginal::EvaluatorSet(evaluators);
}

// Setters

void inf::ConstraintSet::set_target_distribution(inf::TargetDistr const &d) {
    for (inf::Constraint::UniquePtr &constraint : m_constraints) {
        constraint->set_target_distribution(d);
    }

    update_constraint_scale_factors();
}

void inf::ConstraintSet::set_dual_vector_from_quovec(inf::Quovec const &coeffs) {
    Index offset = 0;
    for (inf::Constraint::UniquePtr &constraint : m_constraints) {
        constraint->set_dual_vector_from_quovec(coeffs, offset);
        offset += constraint->get_quovec_size();
    }
    ASSERT_EQUAL(offset, get_quovec_size())

    hard_assert_quovecs_within_bound();
}

// Evaluation

inf::Quovec inf::ConstraintSet::get_inflation_event_quovec(inf::Event const &inflation_event) const {
    inf::Quovec ret(m_quovec_size);

    Index offset = 0;
    for (inf::Constraint::UniquePtr const &constraint : m_constraints) {
        constraint->compute_inflation_event_quovec(inflation_event, ret, offset);
        offset += constraint->get_quovec_size();
    }
    ASSERT_EQUAL(offset, get_quovec_size())

    return ret;
}

double inf::ConstraintSet::get_quovec_denom() const {
    return m_quovec_denom;
}

// Serialization

void inf::ConstraintSet::write_dual_vector_to_file(std::string const &filename, std::string const &metadata) {
    util::OutputFileStream ofs(filename, util::FileStream::Format::text, metadata);
    io_dual_vector(ofs);
}

void inf::ConstraintSet::read_dual_vector_from_file(std::string const &filename, std::string const &metadata) {
    util::InputFileStream ifs(filename, util::FileStream::Format::text, metadata);
    io_dual_vector(ifs);

    hard_assert_quovecs_within_bound();
}

// Private methods

void inf::ConstraintSet::io_dual_vector(util::FileStream &stream) {
    stream.write_or_read_and_hard_assert("METADATA");
    stream.write_or_read_and_hard_assert(m_inflation->get_metadata());
    stream.write_or_read_and_hard_assert("CONSTRAINT SET");
    for (inf::Constraint::UniquePtr const &constraint : m_constraints)
        stream.write_or_read_and_hard_assert(constraint->get_pretty_description());

    stream.write_or_read_and_hard_assert("DUAL VECTOR");
    for (inf::Constraint::UniquePtr const &constraint : m_constraints)
        constraint->io_dual_vector(stream);
}

void inf::ConstraintSet::init_quovec_size() {
    m_quovec_size = 0;
    for (inf::Constraint::UniquePtr const &constraint : m_constraints) {
        m_quovec_size += constraint->get_quovec_size();
    }
}

//! \cond
namespace util {

/*! \return The gcd */
double simplify_by_gcd(std::vector<mpz_t> &scale_factors) {
    mpz_t one;
    mpz_init_set_si(one, 1);

    mpz_t the_gcd;
    mpz_init_set(the_gcd, scale_factors[0]);

    for (Index const i : util::Range(Index(1), scale_factors.size())) {
        mpz_gcd(the_gcd, the_gcd, scale_factors[i]);
        if (mpz_cmp(the_gcd, one) == 0)
            return 1.0;
    }

    for (mpz_t &scale_factor : scale_factors) {
        // Floor division
        mpz_fdiv_q(scale_factor, scale_factor, the_gcd);
    }

    return mpz_get_d(the_gcd);
}

std::string str(mpz_t const &n) {
    return mpz_get_str(NULL, 10, n);
}

signed long int safe_ll_to_l(Num n) {
    HARD_ASSERT_LT(n, static_cast<Num>(std::numeric_limits<signed long int>::max()))
    return static_cast<signed long int>(n);
}

Num mpz_to_ll(const mpz_t n) {
    // We're cheating here: instead of doing n -> ll, we're doing n -> l -> ll
    // If it doesn't fit, we could use more fancy arithmetic
    HARD_ASSERT_TRUE(mpz_fits_slong_p(n) != 0)

    return mpz_get_si(n);
}
} // namespace util
//! \endcond

void inf::ConstraintSet::update_constraint_scale_factors() {
    ASSERT_LT(0, m_constraints.size())

    // One scale factor for the LHS and the RHS of each constraint
    std::vector<mpz_t> scale_factors(2 * m_constraints.size());
    for (mpz_t &scale_factor : scale_factors) {
        mpz_init_set_si(scale_factor, 1);
    }

    // We want
    // scale_factors[0] =        RHS[0]*LHS[1]*RHS[1]*...
    // scale_factors[1] = LHS[0]       *LHS[1]*RHS[1]*...
    // scale_factors[2] = LHS[0]*RHS[0]       *RHS[1]*...
    // scale_factors[3] = LHS[0]*RHS[0]*LHS[1]       *...

    m_quovec_denom = 1.0;

    mpz_t lhs_denom, rhs_denom;
    mpz_init(lhs_denom);
    mpz_init(rhs_denom);

    for (Index const constraint_index : util::Range(m_constraints.size())) {
        m_quovec_denom *=
            static_cast<double>(m_constraints[constraint_index]->get_lhs_denom()) * static_cast<double>(m_constraints[constraint_index]->get_rhs_denom());

        signed long int lhs_denom = util::safe_ll_to_l(m_constraints[constraint_index]->get_lhs_denom());
        signed long int rhs_denom = util::safe_ll_to_l(m_constraints[constraint_index]->get_rhs_denom());

        for (Index const constraint_index_bis : util::Range(m_constraints.size())) {
            mpz_t *lhs_scale_factor = &scale_factors[2 * constraint_index_bis];
            mpz_t *rhs_scale_factor = &scale_factors[2 * constraint_index_bis + 1];

            mpz_mul_si(*lhs_scale_factor, *lhs_scale_factor, rhs_denom);
            mpz_mul_si(*rhs_scale_factor, *rhs_scale_factor, lhs_denom);

            if (constraint_index == constraint_index_bis)
                continue;

            mpz_mul_si(*lhs_scale_factor, *lhs_scale_factor, lhs_denom);
            mpz_mul_si(*rhs_scale_factor, *rhs_scale_factor, rhs_denom);
        }
    }

    double const the_gcd = util::simplify_by_gcd(scale_factors);
    m_quovec_denom /= the_gcd;

    for (Index const constraint_index : util::Range(m_constraints.size())) {
        inf::Constraint &constraint = *m_constraints[constraint_index];
        constraint.set_lhs_scale(
            util::mpz_to_ll(scale_factors[2 * constraint_index]));
        constraint.set_rhs_scale(
            util::mpz_to_ll(scale_factors[2 * constraint_index + 1]));
    }

    // Max dual_vector component

    // m_max_dual_vector_component = MAX * gcd / (LHS[0]*RHS[0]*... * 2 * #constraints )
    m_max_dual_vector_component = std::numeric_limits<Num>::max();
    // Just to be safe
    Num const safety_factor = 10;

    Num const divide_by = safety_factor * util::mpz_to_ll(scale_factors[0]) * m_constraints[0]->get_lhs_denom() * 2 * static_cast<Num>(m_constraints.size());

    m_max_dual_vector_component /= divide_by;

    // To ensure that the quovecs always have small enough values, we can reset it here
    {
        inf::Quovec const zero_quovec(get_quovec_size(), 0);
        set_dual_vector_from_quovec(zero_quovec);
    }
}

void inf::ConstraintSet::hard_assert_quovecs_within_bound() const {
    for (inf::Constraint::UniquePtr const &constraint : m_constraints) {
        constraint->get_dual_vector().hard_assert_within_bound(m_max_dual_vector_component);
    }
}
