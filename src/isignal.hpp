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

  virtual bool handle( signal::Foo const& foo ) = 0;

  virtual wait<int> handle( signal::Bar const& foo ) = 0;
};

/****************************************************************
** NoopSignalHandler
*****************************************************************/
struct NoopSignalHandler : ISignalHandler {
  bool handle( signal::Foo const& foo ) override;

  wait<int> handle( signal::Bar const& foo ) override;
};

} // namespace rn
