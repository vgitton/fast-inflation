#include "chrono.h"

#include "debug.h"
#include "logger.h"
#include <chrono>

const util::Chrono util::Chrono::process_clock(util::Chrono::State::running);

util::Chrono::Chrono(util::Chrono::State state)
    : m_start_time{},
      m_duration(0),
      m_state(util::Chrono::State::paused) {
    if (state == util::Chrono::State::running)
        start();
}

void util::Chrono::start() {
    ASSERT_TRUE(m_state == util::Chrono::State::paused);
    m_state = util::Chrono::State::running;
    m_start_time = std::chrono::high_resolution_clock::now();
}

void util::Chrono::pause() {
    ASSERT_TRUE(m_state == util::Chrono::State::running);
    m_state = util::Chrono::State::paused;
    m_duration += std::chrono::high_resolution_clock::now() - m_start_time;
}

void util::Chrono::reset() {
    ASSERT_TRUE(m_state == util::Chrono::State::paused)
    m_duration = std::chrono::nanoseconds::zero();
}

double util::Chrono::get_seconds() const {
    std::chrono::nanoseconds duration = m_duration;
    if (m_state == util::Chrono::State::running)
        duration += std::chrono::high_resolution_clock::now() - m_start_time;

    return 1.0e-9 * static_cast<double>(duration.count());
}

void util::Chrono::log() const {
    int64_t ms_count = static_cast<int64_t>(1000.0 * get_seconds());

    int64_t s_count = ms_count / 1000;
    int64_t ms_leftover = ms_count % 1000;
    if (s_count == 0) {
        util::logger << ms_count << "ms";
        return;
    }

    int64_t min_count = s_count / 60;
    int64_t s_leftover = s_count % 60;
    if (min_count == 0) {
        util::logger << s_count << "s";
        if (ms_leftover < 10) {
            util::logger << "00";
        } else if (ms_leftover < 100) {
            util::logger << "0";
        }
        util::logger << ms_leftover << "ms";
        return;
    }

    int64_t hour_count = min_count / 60;
    int64_t min_leftover = min_count % 60;
    if (hour_count == 0) {
        util::logger << min_count << "m";
        if (s_leftover < 10)
            util::logger << "0";
        util::logger << s_leftover << "s";
        return;
    }

    util::logger << hour_count << "h";
    if (min_leftover < 10)
        util::logger << "0";
    util::logger << min_leftover << "m";
}
