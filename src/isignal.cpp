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

using namespace ::rn::signal;

/****************************************************************
** ISignalHandler
*****************************************************************/
void ISignalHandler::handle( RefUnitAdded const& ) {}

void ISignalHandler::handle( RebelSentimentChanged const& ) {}

void ISignalHandler::handle( ColonyDestroyedByNatives const& ) {}

void ISignalHandler::handle(
    ColonyDestroyedByStarvation const& ) {}

void ISignalHandler::handle( ColonySignal const& ) {}

void ISignalHandler::handle( ColonySignalTransient const& ) {}

void ISignalHandler::handle( ImmigrantArrived const& ) {}

} // namespace rn
