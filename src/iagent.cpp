/****************************************************************
**iagent.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-08-12.
*
* Description: Base interface for the I*Agent interfaces.
*
*****************************************************************/
#include "iagent.hpp"

// Revolution Now
#include "co-wait.hpp"

using namespace std;

namespace rn {

/****************************************************************
** NoopMind
*****************************************************************/
wait<> NoopMind::message_box( string const& ) { co_return; }

} // namespace rn
