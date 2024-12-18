#pragma once

namespace util {

/*! \ingroup printing
 *  \brief Defines printable classes
    \details This class is used to indicate classes that are compatible with the `util::logger << instance` syntax.
    This is defined in a separate file to avoid including the whole logger.h file. */
class Loggable {
  public:
    /*! \brief This prints the object to the globally accessible `util::logger` and allows the `util::logger << loggable` syntax */
    virtual void log() const = 0;
};

} // namespace util
