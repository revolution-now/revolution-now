/****************************************************************
**disband.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-09.
*
* Description: Things related to disbanding units.
*
*****************************************************************/
#include "disband.hpp"

// Revolution Now
#include "cheat.hpp"
#include "co-wait.hpp"
#include "colony-mgr.hpp"
#include "gui.hpp"
#include "imap-updater.hpp"
#include "map-square.hpp"
#include "revolution-status.hpp"
#include "tribe-mgr.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
#include "unit-ownership.hpp"
#include "unit-stack.hpp"
#include "views.hpp"
#include "visibility.hpp"

// config
#include "config/natives.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/native-unit.rds.hpp"
#include "ss/natives.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/terrain.hpp"
#include "ss/tribe.rds.hpp"
#include "ss/units.hpp"

// base
#include "base/string.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::point;

bool can_disband_foreign_units( SSConst const& ss ) {
  return cheat_mode_enabled( ss );
}

bool can_disband_unit( SSConst const& ss,
                       e_nation const& player_nation,
                       GenericUnitId const generic_unit_id ) {
  switch( ss.units.unit_kind( generic_unit_id ) ) {
    case e_unit_kind::euro: {
      Unit const& unit =
          ss.units.euro_unit_for( generic_unit_id );
      if( unit.nation() != player_nation )
        return can_disband_foreign_units( ss );
      return true;
    }
    case e_unit_kind::native:
      return can_disband_foreign_units( ss );
  }
}

// This applies to either friendly or foreign, since neither is
// possible outside of cheat mode.
bool can_disband_colonies( SSConst const& ss ) {
  return cheat_mode_enabled( ss );
}

bool can_disband_dwellings( SSConst const& ss ) {
  return cheat_mode_enabled( ss );
}

string tribe_name( e_tribe const tribe_type ) {
  return config_natives.tribes[tribe_type].name_singular;
}

string tribe_dwelling_type_name( e_tribe const tribe_type ) {
  auto const level = config_natives.tribes[tribe_type].level;
  return config_natives.dwelling_types[level].name_singular;
}

vector<UnitId> ships_on_tile( SSConst const& ss,
                              point const tile ) {
  vector<UnitId> ships;
  for( GenericUnitId const generic_id :
       ss.units.from_coord( tile ) )
    if( auto const unit =
            ss.units.maybe_euro_unit_for( generic_id );
        unit.has_value() )
      if( unit->desc().ship ) ships.push_back( unit->id() );
  return ships;
}

auto& unit_type_name( SSConst const& ss,
                      GenericUnitId const unit_id ) {
  switch( ss.units.unit_kind( unit_id ) ) {
    case e_unit_kind::euro:
      return unit_attr(
                 ss.units.euro_unit_for( unit_id ).type() )
          .name;
    case e_unit_kind::native:
      return unit_attr(
                 ss.units.native_unit_for( unit_id ).type )
          .name;
  }
}

wait<maybe<EntitiesOnTile>> disband_selection_dialog(
    SSConst const& ss, IGui& gui, Player const& player,
    IVisibility const& viz, EntitiesOnTile const& entities ) {
  using namespace ui;

  string const title = "Select unit(s) to disband:";

  // Add check boxes.
  auto boxes_array = make_unique<VerticalArrayView>(
      VerticalArrayView::align::left );

  unordered_map<int, LabeledCheckBoxView const*> boxes;
  auto add_checkbox = [&, checkbox_idx = 0](
                          string const& label,
                          unique_ptr<View> view ) mutable {
    auto labeled_box =
        make_unique<LabeledCheckBoxView>( std::move( view ) );
    labeled_box->add_view( make_unique<TextView>( label ) );
    labeled_box->recompute_child_positions();
    boxes[checkbox_idx++] = labeled_box.get();
    boxes_array->add_view( std::move( labeled_box ) );
  };
  for( GenericUnitId const unit_id : entities.units ) {
    string owner_name;
    auto icon_view = [&]() -> unique_ptr<View> {
      switch( ss.units.unit_kind( unit_id ) ) {
        case e_unit_kind::euro: {
          Unit const& unit = ss.units.euro_unit_for( unit_id );
          // Note we use viz.nation here instead of player na-
          // tion, because it really matters more what view the
          // player is assuming here.
          if( viz.nation() != unit.nation() )
            owner_name = nation_possessive( player );
          return make_unique<FakeUnitView>(
              unit.type(), unit.nation(), unit.orders() );
        }
        case e_unit_kind::native:
          NativeUnit const& native_unit =
              ss.units.native_unit_for( unit_id );
          e_tribe const tribe_type =
              tribe_for_unit( ss, native_unit ).type;
          owner_name = tribe_name( tribe_type );
          return make_unique<FakeNativeUnitView>(
              native_unit.type, tribe_type );
      }
    }();
    if( !owner_name.empty() ) owner_name += ' ';
    add_checkbox( fmt::format( "{}{}", owner_name,
                               unit_type_name( ss, unit_id ) ),
                  std::move( icon_view ) );
  }
  if( auto const& colony = entities.colony;
      colony.has_value() ) {
    auto icon_view =
        make_unique<RenderedColonyView>( ss, *colony );
    add_checkbox( colony->name, std::move( icon_view ) );
  }
  if( auto const& dwelling = entities.dwelling;
      dwelling.has_value() ) {
    auto icon_view =
        make_unique<RenderedDwellingView>( ss, *dwelling );
    e_tribe const tribe_type =
        tribe_type_for_dwelling( ss, *dwelling );
    string const label = fmt::format(
        "{} {}", tribe_name( tribe_type ),
        base::capitalize_initials(
            tribe_dwelling_type_name( tribe_type ) ) );
    add_checkbox( label, std::move( icon_view ) );
  }
  boxes_array->recompute_child_positions();

  auto on_ok = [&] {
    EntitiesOnTile selected;
    selected.units.reserve( entities.units.size() );
    int checkbox_idx = 0;
    // Must go in order of the checkboxes here so that the idx
    // matches what we think it represents.
    for( auto const id : entities.units )
      if( boxes[checkbox_idx++]->on() ) //
        selected.units.push_back( id );
    if( entities.colony.has_value() )
      if( boxes[checkbox_idx++]->on() ) //
        selected.colony = entities.colony;
    if( entities.dwelling.has_value() )
      if( boxes[checkbox_idx++]->on() ) //
        selected.dwelling = entities.dwelling;
    return selected;
  };

  co_return co_await gui
      .interactive_ok_cancel_box<EntitiesOnTile>(
          title, std::move( boxes_array ), on_ok );
  // !! At this point the view and all pointers into it above
  // have been destroyed.
}

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
// will prevent it from being activated), but that is an edge
// case that is probably not worth replicating. Thus, since the
// NG does have proper support for units on ships, and since even
// the OG has a workaround, we just always allow it since it
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
void offload_if_ship_on_land_with_units( SS& ss, TS& ts,
                                         UnitId const unit_id ) {
  Unit& unit = ss.units.unit_for( unit_id );
  point const tile =
      coord_for_unit_indirect_or_die( ss.units, unit.id() );
  bool const is_on_land =
      is_land( ss.terrain.square_at( tile ) );
  bool const contains_units = !unit.cargo().units().empty();
  if( unit.desc().ship && is_on_land && contains_units )
    // This is for compatibility with the OG.
    offboard_units_on_ship( ss, ts, unit );
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
DisbandingPermissions disbandable_entities_on_tile(
    SSConst const& ss, IVisibility const& viz,
    gfx::point const tile ) {
  DisbandingPermissions perms;

  // Units. Note that if the current viewer does not have a clear
  // square then no units will be visible, so thus we should not
  // ask the user if they want to disband units on that tile as
  // that would seem strange to the user.
  if( viz.visible( tile ) == e_tile_visibility::clear ) {
    vector<GenericUnitId> const units_sorted = [&] {
      auto const units =
          units_from_coord_recursive( ss.units, tile );
      vector<GenericUnitId> res( units.begin(), units.end() );
      sort_unit_stack( ss, res );
      return res;
    }();
    for( auto const generic_unit_id : units_sorted ) {
      bool const disbandable =
          !viz.nation().has_value() ||
          can_disband_unit( ss, *viz.nation(), generic_unit_id );
      if( disbandable )
        perms.disbandable.units.push_back( generic_unit_id );
    }
  }

  // Colonies. It appears that colonies (even without a stockade)
  // cannot be disbanded outside of cheat mode. If a colony does
  // not have a stockade then the player can always eliminate the
  // colony, but the OG still doesn't allow disbanding it outside
  // of cheat mode. This is likely just to prevent disbanding it
  // by mistake, which would be costly, so we'll keep that behav-
  // ior. In cheat mode we will allow disbanding it as in the OG.
  if( auto const colony = viz.colony_at( tile );
      colony.has_value() ) {
    if( can_disband_colonies( ss ) )
      perms.disbandable.colony = colony;
  }

  // Dwellings. Cannot be disbanded outside of cheat mode.
  if( auto const dwelling = viz.dwelling_at( tile );
      dwelling.has_value() ) {
    if( can_disband_dwellings( ss ) )
      perms.disbandable.dwelling = dwelling;
  }

  return perms;
}

wait<EntitiesOnTile> disband_tile_ui_interaction(
    SSConst const& ss, TS& ts, Player const& player,
    IVisibility const& viz,
    DisbandingPermissions const& perms ) {
  EntitiesOnTile entities;

  if( perms.disbandable.units.empty() &&
      !perms.disbandable.colony.has_value() &&
      !perms.disbandable.dwelling.has_value() )
    // Nothing to do here.
    co_return entities;

  auto ask_one =
      [&]( string_view const q ) -> wait<maybe<ui::e_confirm>> {
    co_return co_await ts.gui.optional_yes_no(
        { .msg            = string( q ),
          .yes_label      = "Yes",
          .no_label       = "No",
          .no_comes_first = true } );
  };

  auto show_ships_in_port_error = [&]() -> wait<> {
    co_await ts.gui.message_box(
        "We cannot disband a colony that has a ship in its "
        "port unless the ship is simultaneously disbanded or "
        "first moved out of the colony." );
  };

  if( perms.disbandable.units.size() == 1 &&
      !perms.disbandable.colony.has_value() &&
      !perms.disbandable.dwelling.has_value() ) {
    GenericUnitId const unit_id = perms.disbandable.units[0];
    auto const q = fmt::format( "Really disband [{}]?",
                                unit_type_name( ss, unit_id ) );

    maybe<ui::e_confirm> const answer = co_await ask_one( q );
    if( answer == ui::e_confirm::yes )
      entities.units.push_back( unit_id );
    co_return entities;
  }

  if( perms.disbandable.units.empty() &&
      perms.disbandable.colony.has_value() &&
      !perms.disbandable.dwelling.has_value() ) {
    Colony const& colony = *perms.disbandable.colony;
    // We cannot allow disbanding a colony that has a ship in
    // port unless we are also disbanding the ship, otherwise the
    // ship will be left on land, which would break game invari-
    // ants (for example, what would happen if it then got at-
    // tacked by a land unit?). In practice we won't get to this
    // part of the code because if there are (friendly) ships in
    // port then they would be in the list of disbandable units,
    // which would prevent us from getting into this block. The
    // other check for this scenario further below is something
    // that could actually happen. That said, we'll check it any-
    // way, just in case someone calls this function with strange
    // arguments or the game is in a weird state as a result of
    // cheat mode (which is very likely on, because the player
    // cannot disband colonies outside of cheat mode).
    bool const has_ships_in_port =
        !ships_on_tile( ss, colony.location ).empty();
    // In this part of the code there are units simultaneously
    // being disbanded, so we are not disbanding the ship, thus
    // we cannot allow the player to disband the colony.
    if( has_ships_in_port ) {
      co_await show_ships_in_port_error();
      co_return entities;
    }
    auto const q =
        fmt::format( "Really disband [{}]?", colony.name );
    maybe<ui::e_confirm> const answer = co_await ask_one( q );
    if( answer == ui::e_confirm::yes ) entities.colony = colony;
    co_return entities;
  }

  if( perms.disbandable.units.empty() &&
      !perms.disbandable.colony.has_value() &&
      perms.disbandable.dwelling.has_value() ) {
    Dwelling const& dwelling = *perms.disbandable.dwelling;
    e_tribe const tribe_type =
        tribe_type_for_dwelling( ss, dwelling );
    auto const q = fmt::format(
        "Really disband [{}] {}?", tribe_name( tribe_type ),
        base::capitalize_initials(
            tribe_dwelling_type_name( tribe_type ) ) );
    maybe<ui::e_confirm> const answer = co_await ask_one( q );
    if( answer == ui::e_confirm::yes )
      entities.dwelling = dwelling;
    co_return entities;
  }

  // There are multiple entities, thus we must pop open a dialog
  // box and list them.
  entities = ( co_await disband_selection_dialog(
                   ss, ts.gui, player, viz, perms.disbandable ) )
                 .value_or( EntitiesOnTile{} );

  // Make sure that if we're disbanding a colony that it either
  // has no ships in port or those ships are also being disbanded
  // simultaneously. The idea is that we want to prevent ships
  // from being left on land.
  if( entities.colony.has_value() ) {
    vector<UnitId> const ships_in_port =
        ships_on_tile( ss, entities.colony->location );
    unordered_set<GenericUnitId> const units_being_disbanded(
        entities.units.begin(), entities.units.end() );
    for( UnitId const ship : ships_in_port ) {
      if( !units_being_disbanded.contains( ship ) ) {
        co_await show_ships_in_port_error();
        entities = {};
        co_return entities;
      }
    }
  }

  co_return entities;
}

wait<> execute_disband( SS& ss, TS& ts, IVisibility const& viz,
                        gfx::point const tile,
                        EntitiesOnTile const& entities ) {
  // NOTE: we need to do units first in case we are disbanding
  // both a colony and a ship that it has in port. If we don't
  // first get rid of the ship then code below will check-fail
  // when we disband the colony.
  for( GenericUnitId const generic_id : entities.units ) {
    if( !ss.units.exists( generic_id ) )
      // We might be simultaneously disbanding a ship and the
      // units it contains, and so if we happen to disband the
      // ship first, then its cargo units (in this list) will
      // have already been destroyed.
      continue;
    switch( ss.as_const.units.unit_kind( generic_id ) ) {
      case e_unit_kind::euro: {
        UnitId const unit_id =
            ss.units.check_euro_unit( generic_id );
        offload_if_ship_on_land_with_units( ss, ts, unit_id );
        UnitOwnershipChanger( ss, unit_id ).destroy();
        break;
      }
      case e_unit_kind::native:
        NativeUnitOwnershipChanger(
            ss, ss.units.check_native_unit( generic_id ) )
            .destroy();
        break;
    }
  }

  IMapUpdater& map_updater = ts.map_updater();

  if( entities.colony.has_value() ) {
    // The colony object that we've obtained will be the player's
    // object, which always has an ID of zero, since it may rep-
    // resent a fogged colony. Thus we need to check if there is
    // a real one on this tile and if so destroy it.
    if( auto const real_colony_id =
            ss.colonies.maybe_from_coord( tile );
        real_colony_id.has_value() ) {
      ColonyDestructionOutcome const outcome = destroy_colony(
          ss, ts, ss.colonies.colony_for( *real_colony_id ) );
      int const ships_in_port = [&] {
        int n = 0;
        for( auto const& [type, count] :
             outcome.ships_that_were_in_port )
          n += count;
        return n;
      }();
      CHECK( ships_in_port == 0,
             "there are not supposed to be ships remaining in a "
             "colony port when it gets disbanded, but there "
             "were {}.",
             ships_in_port );
    }
  }

  // If we need to destroy the tribe below then we want to use
  // the interactive method so that it can post the usual message
  // (the OG does this when disbanding the last dwelling of a
  // tribe as well). However, in the case that the player is dis-
  // banding a dwelling under fog, we want to run the code fur-
  // ther below that makes that tile clear first so that the
  // dwelling appears to disappear before the message pops up
  // about the tribe having been destroyed, otherwise it won't
  // make visual sense to the player. So make a note if a tribe
  // needs to be destroyed but run it further below.
  maybe<e_tribe> destroy_tribe;

  if( entities.dwelling.has_value() ) {
    // The dwelling object that we've obtained will be the play-
    // er's object, which always has an ID of zero, since it may
    // represent a fogged dwelling. Thus we need to check if
    // there is a real one on this tile and if so destroy it.
    if( auto const real_id =
            ss.natives.maybe_dwelling_from_coord( tile );
        real_id.has_value() ) {
      e_tribe const tribe_type =
          ss.natives.tribe_type_for( *real_id );
      destroy_dwelling( ss, map_updater, *real_id );
      if( ss.natives.dwellings_for_tribe( tribe_type ).empty() )
        destroy_tribe = tribe_type;
    }
  }

  if( entities.colony.has_value() ||
      entities.dwelling.has_value() )
    // Just in case the colony or dwelling is under fog, this
    // will cause the player square to get updated (and defogged)
    // otherwise the player's (frozen) square will continue to
    // get rendered and the dwelling will continue to appear. If
    // we are disbanding a phantom dwelling, this will simply ex-
    // pose to the player the fact that it is no longer there.
    if( auto const nation = viz.nation(); nation.has_value() )
      map_updater.make_squares_visible( *nation, { tile } );

  if( destroy_tribe.has_value() )
    co_await destroy_tribe_interactive( ss, ts, *destroy_tribe );
}

} // namespace rn
