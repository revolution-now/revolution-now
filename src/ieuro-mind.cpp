/****************************************************************
**ieuro-mind.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-31.
*
* Description: Interface for asking for orders and behaviors for
*              european (non-ref) units.
*
*****************************************************************/
#include "ieuro-mind.hpp"

using namespace std;

namespace rn {

/****************************************************************
** NoopEuroMind
*****************************************************************/
NoopEuroMind::NoopEuroMind( e_nation nation )
  : nation_( nation ) {}

// Implement IEuroMind.
e_nation NoopEuroMind::nation() const { return nation_; }

} // namespace rn
