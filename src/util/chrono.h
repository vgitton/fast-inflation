#pragma once

#include "loggable.h"

#include <chrono>

/*! \file */

/*! \defgroup misc Miscellaneous
    \ingroup util

    \brief Time, randomness, command-line argument parsing.
 */

namespace util {

/*! \ingroup misc

    \brief A simple stopwatch class.

    Usage:
    \code{.cpp}
    util::Chrono c_f, c_total;

    c_total.start();

    for(Index i : util::Range(100))
    {
        // here untimed code with respect to c_f...

        // start the stopwatch:
        c_f.start();
        // here, code that is timed
        Num x = f(i);
        c_f.pause();

        // here untimed code with respect to c_f...
    }

    util::logger << "The total time used to compute f is: " << c_f << util::cr
        << "The whole script took " << c_total << util::cr;
    \endcode
    */
class Chrono : public util::Loggable {
  public:
    /*! \brief The stopwatch is either paused or running */
    enum class State {
        running,
        paused
    };

    /*! \brief The globally accessible time of execution of the whole program */
    static const util::Chrono process_clock;

    /*! \brief Create a stopwatch that is initially running or paused. */
    Chrono(util::Chrono::State state);
    //! \cond
    Chrono(Chrono const &other) = delete;
    Chrono(Chrono &&other) = delete;
    Chrono &operator=(Chrono const &other) = delete;
    Chrono &operator=(Chrono &&other) = delete;
    //! \endcond

    /*! \brief Starts or resumes the stopwatch. Requires the state to be util::Chrono::State::paused. */
    void start();
    /*! \brief Pauses or stops the stopwatch. Requires the state to be util::Chrono::State::running. */
    void pause();
    /*! \brief Resets the stopwatch to zero. Requires the state to be util::Chrono::State::paused. */
    void reset();
    /*! \brief Get the current number of elapsed seconds on the stopwatch. No state requirements. */
    double get_seconds() const;

    /*! \brief Logs the total number of elapsed seconds when the stopwatch was running. No state requirements. */
    void log() const override;

  private:
    /*! \brief The latest time when util::Chrono::start() was called */
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start_time;
    /*! \brief The sum of time elapsed between each pair of util::Chrono::start() & util::Chrono::pause() statements */
    std::chrono::nanoseconds m_duration;
    /*! \brief The state is kept to avoid calling util::Chrono::start() twice, etc.
     *
     * This is not strictly speaking necessary, but is helpful to make sure that
     * the way the stopwatch is used makes sense, and hence that it times the portion of code that it should time. */
    util::Chrono::State m_state;
};

} // namespace util
