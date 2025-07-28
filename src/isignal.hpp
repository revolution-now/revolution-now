/****************************************************************
**isignal.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-28.
*
* Description: Signals to agents controlling european players.
*
*****************************************************************/
#pragma once

// rds
#include "isignal.rds.hpp"

// Revolution Now
#include "wait.hpp"

// Should be pure virtual to force us to override the method in
// the mock version which in turn forces us to expect the calls
// to it in unit tests (which would otherwise go unnoticed and
// untested).
#define DEFINE_SIGNAL( ret, sig ) \
  virtual ret handle( signal::sig const& ) = 0;

#define SIGNAL_RESULT( sig )                       \
  decltype( std::declval<ISignalHandler>().handle( \
      std::declval<signal::sig>() ) )

#define OVERRIDE_SIGNAL( sig ) \
  SIGNAL_RESULT( sig )         \
  handle( signal::sig const& ) override;

#define EMPTY_SIGNAL( sig ) \
  SIGNAL_RESULT( sig )      \
  SignalHandlerT::handle( signal::sig const& ) {}

namespace rn {

/****************************************************************
** Fwd Decls.
*****************************************************************/
struct ColonyNotification;

/****************************************************************
** Signals.
*****************************************************************/
namespace signal {

// TODO: need to add fwd decls to rds to move these out.
struct ColonySignal {
  ColonyNotification const* value = {};
};

struct ColonySignalTransient {
  std::string msg;
  ColonyNotification const* value = {};
};

} // namespace signal

/****************************************************************
** ISignalHandler
*****************************************************************/
struct ISignalHandler {
  virtual ~ISignalHandler() = default;

 public: // Required.
  DEFINE_SIGNAL( wait<maybe<int>>, ChooseImmigrant );
  DEFINE_SIGNAL( void, ColonyDestroyedByNatives );
  DEFINE_SIGNAL( void, ColonyDestroyedByStarvation );
  DEFINE_SIGNAL( void, ColonySignal );
  DEFINE_SIGNAL( void, ColonySignalTransient );
  DEFINE_SIGNAL( void, ForestClearedNearColony );
  DEFINE_SIGNAL( void, ImmigrantArrived );
  DEFINE_SIGNAL( void, NoSpotForShip );
  DEFINE_SIGNAL( void, PioneerExhaustedTools );
  DEFINE_SIGNAL( void, PriceChange );
  DEFINE_SIGNAL( void, RebelSentimentChanged );
  DEFINE_SIGNAL( void, RefUnitAdded );
  DEFINE_SIGNAL( void, ShipFinishedRepairs );
  DEFINE_SIGNAL( void, TaxRateWillChange );
  DEFINE_SIGNAL( void, TeaParty );
  DEFINE_SIGNAL( void, TreasureArrived );
  DEFINE_SIGNAL( void, TribeWipedOut );
};

} // namespace rn
