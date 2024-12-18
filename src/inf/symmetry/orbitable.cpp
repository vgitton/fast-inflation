#include "orbitable.h"
#include "../../util/chrono.h"
#include "../../util/debug.h"
#include "../../util/logger.h"

inf::Orbitable::Orbitable()
    : m_orbits{},
      m_orbits_initialized(false),
      m_sym_group{} {}

inf::Symmetry::Group const &inf::Orbitable::get_sym_group() const {
    ASSERT_TRUE(m_orbits_initialized);
    return m_sym_group;
}

Index inf::Orbitable::get_n_orbits() const {
    ASSERT_TRUE(m_orbits_initialized);

    return m_orbits.size();
}

inf::Orbitable::OrbitMap const &inf::Orbitable::get_orbits() const {
    ASSERT_TRUE(m_orbits_initialized);

    return m_orbits;
}

void inf::Orbitable::log_sym_group() const {
    ASSERT_TRUE(m_orbits_initialized);

    util::logger << "The underlying symmetry group has cardinality "
                 << m_sym_group.size() << "." << util::cr
                 << "The symmetrization resulted in " << m_orbits.size()
                 << " orbits starting from " << get_n_events() << " events." << util::cr;
}

void inf::Orbitable::log_orbits(bool shorten) const {
    ASSERT_TRUE(m_orbits_initialized);

    LOG_BEGIN_SECTION_FUNC

    for (inf::Orbitable::OrbitPair const &orbit_pair : m_orbits) {
        inf::Orbitable::Orbit const &orbit = orbit_pair.second;

        util::logger << "Orbit (card = " << orbit.size() << "):" << util::cr;

        Index count = 0;
        bool first_element = true;
        Index items_per_line = 8;

        util::logger << util::begin_section;

        for (inf::Event const &event_in_orbit : orbit) {
            if (not first_element) {
                util::logger << " -> ";

                if (count % items_per_line == 0) {
                    if (shorten) {
                        util::logger << "...";
                        break;
                    } else {
                        util::logger << util::cr;
                    }
                }
            }

            if (first_element) {
                first_element = false;
            }

            ++count;

            log_event(event_in_orbit);
        }
        util::logger << util::cr << util::end_section;
    }

    LOG_END_SECTION
}

void inf::Orbitable::init_orbits(inf::Symmetry::Group const &sym_group) {
    ASSERT_TRUE(not m_orbits_initialized);

    m_sym_group = sym_group;

    // An inf::Orbitable with no parties is just a scalar, there are no orbits
    if (get_n_parties() == 0) {
        ASSERT_EQUAL(m_sym_group.size(), 0)

        m_orbits_initialized = true;

        return;
    }

    ASSERT_LT(0, m_sym_group.size());

    inf::Event new_event(this->get_n_parties());
    std::set<inf::Event> encountered_events;

    for (inf::Event const &event : this->get_event_range()) {
        bool const event_already_encountered = (encountered_events.find(event) != encountered_events.end());
        if (event_already_encountered)
            continue;

        m_orbits[event] = inf::Orbitable::Orbit({event});

        for (inf::Symmetry const &sym : m_sym_group) {
            sym.act_on_event(event, new_event);
            m_orbits[event].insert(new_event);
            encountered_events.insert(new_event);
        }
    }

    DEBUG_RUN(
        Index other_event_count = 0;
        for (inf::Orbitable::OrbitPair const &orbit_pair
             : m_orbits) {
            other_event_count += orbit_pair.second.size();
        } ASSERT_EQUAL(this->get_n_events(), other_event_count);)

    m_orbits_initialized = true;
}
