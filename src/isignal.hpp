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
  virtual bool handle( signal::Foo const& ctx ) = 0;

  virtual wait<int> handle( signal::Bar const& ctx ) = 0;

 public: // Optional.

  virtual void handle( signal::RefUnitAdded const& );

  virtual void handle( signal::RebelSentimentChanged const& );

  virtual void handle( signal::ColonyDestroyedByNatives const& );

  virtual void handle(
      signal::ColonyDestroyedByStarvation const& );

  virtual void handle( signal::ColonySignal const& );

  virtual void handle( signal::ColonySignalTransient const& );

  virtual void handle( signal::ImmigrantArrived const& );

  virtual wait<maybe<int>> handle(
      signal::ChooseImmigrant const& ) = 0;

  virtual wait<> handle( signal::PanTile const& ) = 0;

  virtual void handle( signal::NoSpotForShip const& );

  virtual void handle( signal::TreasureArrived const& );
};

} // namespace rn
