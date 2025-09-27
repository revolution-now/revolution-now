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

using namespace std;

namespace rn {

namespace {

using ::gfx::point;

struct GotoHandler : public CommandHandler {
  GotoHandler( SS& ss, UnitId const unit_id,
               command::go_to const& go_to )
    : unit_( ss.units.unit_for( unit_id ) ), goto_( go_to ) {}

  wait<bool> confirm() override {
    // The idea with goto is that we can always attempt it, which
    // will be done within the same turn immediately after this
    // on the next pass over the unit by the turn module, and if
    // it for some reason won't work then the orders will be
    // cleared and nothing will happen, and the unit will just
    // ask for orders normally.
    co_return true;
  }

  wait<> perform() override {
    // Note there is no charge of movement points for changing to
    // the goto state; those are only subtracted as the unit
    // moves.
    unit_.orders() =
        unit_orders::go_to{ .target = goto_.target };
    co_return;
  }

 private:
  Unit& unit_;
  command::go_to const goto_;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<CommandHandler> handle_command(
    IEngine&, SS& ss, TS&, IAgent&, Player&, UnitId id,
    command::go_to const& go_to ) {
  return make_unique<GotoHandler>( ss, id, go_to );
}

} // namespace rn
