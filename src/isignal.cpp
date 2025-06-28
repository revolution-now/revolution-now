/****************************************************************
**isignal.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-28.
*
* Description: Signals to agents controlling european players.
*
*****************************************************************/
#include "isignal.hpp"

// Revolution Now
#include "co-wait.hpp"

using namespace std;

namespace rn {

/****************************************************************
** NoopSignalHandler
*****************************************************************/
bool NoopSignalHandler::handle( signal::Foo const& ) {
  return false;
}

wait<int> NoopSignalHandler::handle( signal::Bar const& ) {
  co_return 0;
}

} // namespace rn
