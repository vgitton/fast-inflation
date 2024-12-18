#pragma once

#include "../../util/loggable.h"
#include "../events/event_tensor.h"
#include "orbitable.h"

/*! \file */

namespace inf {

/*! \ingroup orbits
    \brief An inf::EventTensor that is also an inf::Orbitable
    \details This class is used to reduce duplication of code between inf::DualVector and inf::TargetDistr.
    The idea is that both of these classes are essentially an inf::EventTensor (they are functions from a set of inf::Event to rational numbers).
    But both of these classes have a relevant symmetry group acting on the inf::Event, and the orbits of events are relevant.
    Thus, inf::TensorWithOrbits provides the methods common to inf::DualVector and to inf::TargetDistr (these are mostly logging capabilities).
*/
class TensorWithOrbits : public inf::Orbitable, public util::Loggable {
  public:
    // Constructor

    /*! \brief Constructor, intializes data to all 0's. The actual inf::EventTensor can be passed by child classes using
     * inf::TensorWithOrbits::init_event_tensor().
        \param n_parties The length of each inf::Event on which the underlying inf::EventTensor is defined
        \param base The number of outcomes per party */
    explicit TensorWithOrbits(Index n_parties, inf::Outcome base);
    //! \cond
    TensorWithOrbits(TensorWithOrbits const &other) = delete;
    TensorWithOrbits(TensorWithOrbits &&other) = delete;
    TensorWithOrbits &operator=(TensorWithOrbits const &other) = delete;
    TensorWithOrbits &operator=(TensorWithOrbits &&other) = delete;
    //! \endcond

    /*! \brief To get access to the underlying data */
    inf::EventTensor const &get_event_tensor() const;

    // Strings

    /*! \brief Extra-short log using inf::TensorWithOrbits::log_header(), inf::TensorWithOrbits::log_data_short() */
    void log_short() const;
    /*! \brief Short log using inf::TensorWithOrbits::log_header(), inf::TensorWithOrbits::log_data_short(),
        inf::Orbitable::log_sym_group() */
    void log() const override;
    /*! \brief Detailed log
        \details Calls inf::TensorWithOrbits::log_header(),
              inf::TensorWithOrbits::log_data_long(),
              inf::Orbitable::log_sym_group(),
              inf::Orbitable::log_orbits() (with `shorten = false`) */
    void log_long() const;
    /*! \brief Logs the data of the representatives of orbits */
    void log_data_short() const;
    /*! \brief Logs all data */
    void log_data_long() const;

    // Overrides from inf::Orbitable

    /*! \brief inf::EventRange supporting the inf::Orbitable and inf::EventTensor */
    virtual inf::EventRange get_event_range() const override;
    /*! \brief Specifies the number of events supporting the inf::Orbitable and inf::EventTensor,
        this matches with inf::TensorWithOrbits::get_event_range() */
    virtual Index get_n_events() const override;
    /*! \brief Specifies the number of parties (i.e., the length of the events supporting the inf::Orbitable),
     * this matches with inf::Orbitable::get_event_range() */
    virtual Index get_n_parties() const override;

    // Virtual

    /*! \brief Math name for inf::TensorWithOrbits::log_data_short() and inf::TensorWithOrbits::log_data_long() */
    virtual std::string get_math_name() const = 0;
    /*! \brief Header logged by the various logging functions */
    virtual void log_header() const = 0;

  protected:
    /*! \brief The underlying data.
        \details Even in the presence of symmetry, a distribution or dual_vector stores
        all of its coefficients for quick access without an intermediate lookup table.
        In other words, we choose to avoid un-symmetrized event -> symmetrized event -> coeff
        and instead go straing for un-symmetrized event -> coeff.
        This uses a tiny bit more memory, but is potentially quicker.
        This does make *setting* the coefficients slower, but that's not the bulk of the algorithm. */
    inf::EventTensor m_event_tensor;
    /*! \brief To set all the numerators at once, ignoring orbits
        \warning This method is symmetry-breaking! It should only be called within the constructor of the child class, and before initalizing the orbits
        of the underlying inf::Orbitable with inf::Orbitable::init_orbits(). This is soft-asserted.
        \param event_tensor Will be copied */
    void init_event_tensor(inf::EventTensor const &event_tensor);
};

} // namespace inf
