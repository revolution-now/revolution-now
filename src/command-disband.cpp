/****************************************************************
**command-disband.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-16.
*
* Description: Carries out commands to disband a unit.
*
*****************************************************************/
#include "command-disband.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "map-square.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
#include "unit-ownership.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// Rds
#include "ui-enums.rds.hpp"

using namespace std;

namespace rn {

namespace {

// NOTE regarding disbanding ships at sea carrying units. As
// background, the OG does not allow disbanding ships at sea con-
// taining units, and this is very likely in order to avoid an
// ambiguity as to which units need to be destroyed as a result.
// Specifically, in the OG, units cannot technically be on ships;
// instead, units onboard ships at sea are still on the map but
// are "dragged around" by the ship as it moves in order to simu-
// late being onboard. That poses a problem when there are mul-
// tiple ships containing units on the same sea square: the game
// would not know in general which units were on which ships and
// hence would not know which units to destroy. In fact, the OG
// has a known "bug" where two ships on the same tile that both
// have units onboard can exchange units depending on which ship
// is the first to move off of the square. All of that said, even
// in the OG this restriction can be worked around by just
// clicking on the ship-at-sea, activating the units on it, and
// disbanding them prior to disbanding the ship, with the same
// effect. Technically, in the OG, this can't be done if one of
// the units on the ship has already moved in the turn (which
// will prevent it from being activated), but that is an arcane
// edge case that is probably not worth replicating. Thus, since
// the NG does have proper support for units on ships, and since
// even the OG has a workaround, we just always allow it since it
// shouldn't have any net effect on gameplay.
//
// There is one more wrinkle though: first note that this re-
// striction of the OG does not apply to ships in a colony port,
// because in that case the units don't have to be destroyed when
// disbanding the ship, since they are at the colony gate, thus
// there is no ambiguity. In order to replicate this behavior, we
// will automatically offboard any units in the cargo of ships in
// a colony port (or on a land square) before disbanding the
// ship, even though we technically don't have to.

/****************************************************************
** DisbandHandler
*****************************************************************/
struct DisbandHandler : public CommandHandler {
  DisbandHandler( SS& ss, TS& ts, UnitId unit_id )
    : ss_( ss ), ts_( ts ), unit_id_( unit_id ) {}

  wait<bool> confirm() override {
    auto q = fmt::format(
        "Really disband {}?",
        ss_.units.unit_for( unit_id_ ).desc().name );

    maybe<ui::e_confirm> const answer =
        co_await ts_.gui.optional_yes_no(
            { .msg = q, .yes_label = "Yes", .no_label = "No" } );
    co_return answer == ui::e_confirm::yes;
  }

  void offload_if_needed() const {
    Unit&       unit = ss_.units.unit_for( unit_id_ );
    Coord const tile =
        coord_for_unit_indirect_or_die( ss_.units, unit.id() );
    bool const is_on_land =
        is_land( ss_.terrain.square_at( tile ) );
    bool const contains_units = !unit.cargo().units().empty();

    if( unit.desc().ship && is_on_land && contains_units )
      // This is for compatibility with the OG.
      offboard_units_on_ship( ss_, ts_, unit );
  }

  wait<> perform() override {
    offload_if_needed();
    UnitOwnershipChanger( ss_, unit_id_ ).destroy();
    co_return;
  }

  SS&    ss_;
  TS&    ts_;
  UnitId unit_id_;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<CommandHandler> handle_command(
    SS& ss, TS& ts, Player&, UnitId id,
    command::disband const& ) {
  return make_unique<DisbandHandler>( ss, ts, id );
}

} // namespace rn
