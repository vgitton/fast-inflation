#include "tensor_with_orbits.h"
#include "../../util/debug.h"
#include "../../util/frac.h"
#include "../../util/logger.h"
#include "../../util/math.h"

// Constructor

inf::TensorWithOrbits::TensorWithOrbits(
    Index n_parties,
    inf::Outcome base)
    : inf::Orbitable(),
      m_event_tensor(n_parties, base) {}

inf::EventTensor const &inf::TensorWithOrbits::get_event_tensor() const {
    return m_event_tensor;
}

void inf::TensorWithOrbits::init_event_tensor(inf::EventTensor const &event_tensor) {
    HARD_ASSERT_TRUE(not orbits_initialized())
    HARD_ASSERT_TRUE(m_event_tensor.has_same_shape_as(event_tensor))

    m_event_tensor = event_tensor;
    m_event_tensor.simplify();
}

// Strings

void inf::TensorWithOrbits::log_short() const {
    this->log_header();
    inf::TensorWithOrbits::log_data_short();

    // The LOG_BEGIN_SECTION should be in this->log_header()
    LOG_END_SECTION
}
void inf::TensorWithOrbits::log() const {
    this->log_header();
    inf::TensorWithOrbits::log_data_short();
    inf::Orbitable::log_sym_group();

    // The LOG_BEGIN_SECTION should be in this->log_header()
    LOG_END_SECTION
}

void inf::TensorWithOrbits::log_long() const {
    this->log_header();
    inf::TensorWithOrbits::log_data_short();
    inf::Orbitable::log_sym_group();
    inf::Orbitable::log_orbits(false);

    // The LOG_BEGIN_SECTION should be in this->log_header()
    LOG_END_SECTION
}

void inf::TensorWithOrbits::log_data_short() const {
    Index count = 0;
    bool first_element = true;

    for (inf::Orbitable::OrbitPair const &orbit_pair : get_orbits()) {
        m_event_tensor.log_single_event(this->get_math_name(), first_element, count, 4, orbit_pair.first);
    }
    util::logger << util::cr;
}

void inf::TensorWithOrbits::log_data_long() const {
    LOG_BEGIN_SECTION_FUNC

    m_event_tensor.log(this->get_math_name());

    LOG_END_SECTION
}

// Overrides from inf::Orbitable

inf::EventRange inf::TensorWithOrbits::get_event_range() const {
    return m_event_tensor.get_event_range();
}

Index inf::TensorWithOrbits::get_n_events() const {
    return util::pow(static_cast<Index>(m_event_tensor.get_base()), this->get_n_parties());
}

Index inf::TensorWithOrbits::get_n_parties() const {
    return m_event_tensor.get_n_parties();
}
