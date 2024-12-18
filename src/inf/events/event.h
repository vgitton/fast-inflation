#pragma once

#include "../../types.h"
#include "../../util/product_range.h"
#include "../../util/range.h"
#include <vector>

/*! \file */

/*! \defgroup event Events
    \brief Symmetry, iteration, storage, etc., over events.

    Events are represented as lists of outcomes.
    The context of which party gets which outcome is not explicilty stored in that data structure
    and depends on the context.
    This is less explicit but allows for a faster manipulation of events.
*/

/*! \brief The namespace for the inflation code */
namespace inf {

/*! \ingroup event
    \brief An outcome is stored in the smallest format available to save memory
    \details This would be problematic if we ever deal with more than 256 different outcomes
    per party, but this is anyway unlikely to be something that we can treat computationally here. */
typedef uint8_t Outcome;

/*! \ingroup event
    \brief A list of inf::Outcome. Conceptually, this plays the role of \f$\infevent \in \events{\set A}\f$ for some set \f$\set A\f$. */
typedef std::vector<inf::Outcome> Event;

/*! \ingroup event
    \brief An iterator over inf::Event, conceptually allowing to iterate over \f$\events{\set A}\f$ for some set \f$\set A\f$.
    \details Some classes return inf::EventRange instances. These instances should be used as in
    \code{.cpp}
    for(inf::Event const& e : inf::EventRange(3, 4))
    {
        // Do something with inf::Event e here
        // e = {0,0,0}
        // e = {0,0,1}
        // ...
        // e = {3,3,3}
    }
    \endcode
    */
typedef util::ProductRange<inf::Outcome> EventRange;
} // namespace inf
