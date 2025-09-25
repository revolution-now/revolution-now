/****************************************************************
**command-goto.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-21.
*
* Description: Carries out the command to go to a location.
*
*****************************************************************/
#include "command-goto.hpp"

// Revolution Now
#include "co-wait.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/logger.hpp"

using namespace std;

namespace rn {

namespace {

struct GotoHandler : public CommandHandler {
  GotoHandler( SS& ss, IAgent& agent, Player& player,
               UnitId const unit_id,
               command::go_to const& go_to )
    : ss_( ss ),
      player_( player ),
      agent_( agent ),
      unit_( ss.units.unit_for( unit_id ) ),
      goto_( go_to ) {}

  wait<bool> confirm() override { co_return true; }

  wait<> perform() override {
    lg.info( "goto: {}", goto_ );
    // Note there is no charge of movement points for changing to
    // the goto state; those are only subtracted as the unit
    // moves.
    unit_.orders() =
        unit_orders::go_to{ .target = goto_.target };
    co_return;
  }

  SS& ss_;
  Player const& player_;
  IAgent& agent_;
  Unit& unit_;
  command::go_to const goto_;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<CommandHandler> handle_command(
    IEngine&, SS& ss, TS&, IAgent& agent, Player& player,
    UnitId id, command::go_to const& go_to ) {
  return make_unique<GotoHandler>( ss, agent, player, id,
                                   go_to );
}

} // namespace rn
