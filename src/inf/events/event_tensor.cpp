#include "event_tensor.h"
#include "../../util/logger.h"
#include "../../util/math.h"

#include <numeric>

std::vector<Index> inf::EventTensor::compute_weights(
    Index const n_parties,
    Index const base) {
    std::vector<Index> weights(n_parties);
    for (Index party : util::Range(n_parties)) {
        weights[party] = util::pow(base, party);
    }
    return weights;
}

inf::EventTensor::EventTensor(
    Index const n_parties,
    inf::Outcome const base)
    : m_n_parties(n_parties),
      m_base(base),
      m_data(util::pow(static_cast<Index>(base), n_parties), Num(0)),
      m_weights(inf::EventTensor::compute_weights(n_parties, static_cast<Index>(base))),
      m_denom(1) {
    ASSERT_LT(1, base)
}

bool inf::EventTensor::is_scalar() const {
    return m_n_parties == 0;
}

void inf::EventTensor::log(std::string const &math_name) const {
    if (is_scalar()) {
        util::logger << math_name << " = " << get_frac(0) << util::cr;

        return;
    }

    Index count = 0;
    bool first_element = true;

    for (inf::Event const &e : get_event_range()) {
        log_single_event(math_name, first_element, count, 4, e);
    }

    util::logger << util::cr;
}

void inf::EventTensor::log_single_event(
    std::string const &math_name,
    bool &first_element,
    Index &count,
    Index const items_per_line,
    inf::Event const &event) const {
    if (not first_element and count % items_per_line == 0)
        util::logger << util::cr;
    if (first_element)
        first_element = false;

    util::logger << math_name << "(";
    for (inf::Outcome out : event) {
        util::logger.echo_colored_number(out);
    }
    util::logger << ") = " << this->get_frac(event) << "\t";

    ++count;
}

Num inf::EventTensor::get_num(inf::EventTensor::EventHash const hash) const {
    ASSERT_LT(hash, m_data.size())

    return m_data[hash];
}

Num &inf::EventTensor::get_num(inf::EventTensor::EventHash const hash) {
    ASSERT_LT(hash, m_data.size())

    return m_data[hash];
}

Num inf::EventTensor::get_num(inf::Event const &event) const {
    return m_data[get_event_hash(event)];
}

Num &inf::EventTensor::get_num(inf::Event const &event) {
    return m_data[get_event_hash(event)];
}

util::Frac inf::EventTensor::get_frac(inf::EventTensor::EventHash const hash) const {
    return util::Frac(get_num(hash), get_denom());
}

util::Frac inf::EventTensor::get_frac(inf::Event const &event) const {
    return get_frac(get_event_hash(event));
}

bool inf::EventTensor::has_same_shape_as(inf::EventTensor const &other) const {
    return this->m_n_parties == other.m_n_parties and this->m_base == other.m_base;
}

util::ProductRange<inf::Outcome> inf::EventTensor::get_event_range() const {
    ASSERT_TRUE(not is_scalar())
    return util::ProductRange<inf::Outcome>(m_n_parties, m_base);
}

void inf::EventTensor::set_to_tensor_product(std::vector<inf::EventTensor const *> const &factors) {
    DEBUG_RUN(
        Index tot_n_parties = 0;
        for (inf::EventTensor const *u
             : factors) {
            ASSERT_TRUE(u != nullptr)
            ASSERT_EQUAL(this->get_base(), u->get_base())
            tot_n_parties += u->get_n_parties();
        } ASSERT_EQUAL(this->get_n_parties(), tot_n_parties))

    std::vector<inf::Event> event_factors;
    for (inf::EventTensor const *u : factors) {
        event_factors.push_back(inf::Event(u->get_n_parties(), 0));
    }

    for (inf::Event const &e_this : this->get_event_range()) {
        this->get_num(e_this) = 1;

        Index offset = 0;
        for (Index factor_i : util::Range(factors.size())) {
            inf::Event &e_factor = event_factors[factor_i];
            inf::EventTensor const *u = factors[factor_i];

            for (Index i : util::Range(e_factor.size())) {
                e_factor[i] = e_this[offset + i];
            }

            this->get_num(e_this) *= u->get_num(e_factor);

            offset += u->get_n_parties();
        }

        ASSERT_EQUAL(offset, this->get_n_parties())
    }

    Num denom = 1;
    for (inf::EventTensor const *u : factors) {
        denom *= u->get_denom();
    }

    this->set_denom(denom);

    this->simplify();
}

void inf::EventTensor::simplify() {
    Num the_gcd = get_denom();

    for (inf::EventTensor::EventHash hash : get_hash_range()) {
        the_gcd = std::gcd(get_num(hash), the_gcd);

        if (the_gcd == 1) {
            return;
        }
    }

    for (inf::EventTensor::EventHash hash : get_hash_range()) {
        get_num(hash) /= the_gcd;
    }
    set_denom(get_denom() / the_gcd);
}

std::vector<Index> const &inf::EventTensor::get_party_weights() const {
    return m_weights;
}

Num inf::EventTensor::get_denom() const {
    return m_denom;
}

void inf::EventTensor::set_denom(Num const denom) {
    ASSERT_LT(0, denom)

    m_denom = denom;
}

Index inf::EventTensor::get_n_parties() const {
    return m_n_parties;
}

inf::Outcome inf::EventTensor::get_base() const {
    return m_base;
}

util::Range<inf::EventTensor::EventHash> inf::EventTensor::get_hash_range() const {
    return util::Range(m_data.size());
}

inf::EventTensor::EventHash inf::EventTensor::get_event_hash(inf::Event const &event) const {
    ASSERT_EQUAL(event.size(), m_n_parties)

    inf::EventTensor::EventHash hash = 0;
    for (Index party : util::Range(m_n_parties)) {
        hash += event[party] * m_weights[party];
    }
    return hash;
}
