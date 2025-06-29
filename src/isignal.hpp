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
** ISignalHandler
*****************************************************************/
struct ISignalHandler {
  virtual ~ISignalHandler() = default;

 public: // Required.
  virtual bool handle( signal::Foo const& ctx ) = 0;

  virtual wait<int> handle( signal::Bar const& ctx ) = 0;

 public: // Optional.
  virtual wait<> handle( signal::RefUnitAdded const& );

  virtual wait<> handle( signal::RebelSentimentChanged const& );
};

} // namespace rn
