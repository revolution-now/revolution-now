/****************************************************************
**command.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-27.
*
* Description: Handles the representation and application of
*              the commands that players can give to units.
*
*****************************************************************/
#include "command.hpp"

// Revolution Now
#include "co-combinator.hpp"
#include "co-wait.hpp"
#include "command-build.hpp"
#include "command-disband.hpp"
#include "command-dump.hpp"
#include "command-fortify.hpp"
#include "command-goto.hpp"
#include "command-move.hpp"
#include "command-plow.hpp"
#include "command-road.hpp"
#include "command-trade.hpp"
#include "unit-mgr.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/units.hpp"

// base
#include "base/lambda.hpp"

using namespace std;

namespace rn {

namespace {

unique_ptr<CommandHandler> handle_command(
    IEngine&, SS&, TS&, IAgent&, Player&, UnitId,
    command::wait const& ) {
  SHOULD_NOT_BE_HERE;
}

unique_ptr<CommandHandler> handle_command(
    IEngine&, SS&, TS&, IAgent&, Player&, UnitId,
    command::forfeight const& ) {
  SHOULD_NOT_BE_HERE;
}

} // namespace

unique_ptr<CommandHandler> command_handler(
    IEngine& engine, SS& ss, TS& ts, IAgent& agent,
    Player& player, UnitId id, command const& command ) {
  CHECK( !ss.units.unit_for( id ).mv_pts_exhausted() );
  return std::visit( LC( handle_command( engine, ss, ts, agent,
                                         player, id, _ ) ),
                     command.as_base() );
}

wait<CommandHandlerRunResult> CommandHandler::run() {
  CommandHandlerRunResult res{ .order_was_run       = false,
                               .units_to_prioritize = {} };
  if( bool confirmed = co_await confirm(); !confirmed )
    co_return res;

  if( unique_ptr<CommandHandler> delegate = switch_handler();
      delegate != nullptr )
    co_return co_await delegate->run();

  res.order_was_run = true;

  // Command can be carried out (which includes animation).
  co_await perform();

  res.units_to_prioritize = units_to_prioritize();
  res.insufficient_movement_points =
      had_insufficient_movement_points();
  co_return res;
}

} // namespace rn
