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
** ISignalHandler
*****************************************************************/
wait<> ISignalHandler::handle( signal::RefUnitAdded const& ) {
  co_return;
}

wait<> ISignalHandler::handle(
    signal::RebelSentimentChanged const& ) {
  co_return;
}

} // namespace rn
