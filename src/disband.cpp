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
#include "co-wait.hpp"
#include "gui.hpp"
#include "logger.hpp"
#include "map-square.hpp"
#include "plane-stack.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
#include "unit-ownership.hpp"
#include "unit-stack.hpp"
#include "views.hpp"
#include "window.hpp"

// config
#include "config/natives.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/native-unit.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/terrain.hpp"
#include "ss/tribe.rds.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::point;

bool can_disband_foreign_units( SSConst const& ss ) {
  return ss.settings.cheat_options.enabled;
}

bool can_disband_unit( SSConst const&      ss,
                       e_nation const&     player_nation,
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

auto& unit_type_name( SSConst const&      ss,
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

wait<EntitiesOnTile> disband_selection_dialog(
    SSConst const& ss, TS& ts,
    vector<GenericUnitId> const& units ) {
  using namespace ui;

  auto top_array = make_unique<VerticalArrayView>(
      VerticalArrayView::align::center );

  string const title = "Select unit(s) to disband.";

  // Add text.
  auto text_view = make_unique<TextView>( title );
  top_array->add_view( std::move( text_view ) );
  // Add some space between title and check boxes.
  top_array->add_view(
      make_unique<EmptyView>( Delta{ .w = 1, .h = 2 } ) );

  // Add check boxes.
  auto boxes_array = make_unique<VerticalArrayView>(
      VerticalArrayView::align::left );

  unordered_map<int, LabeledCheckBoxView const*> boxes;
  for( int idx = 0; idx < int( units.size() ); ++idx ) {
    auto icon_view = [&]() -> unique_ptr<View> {
      switch( ss.units.unit_kind( units[idx] ) ) {
        case e_unit_kind::euro: {
          Unit const& unit =
              ss.units.euro_unit_for( units[idx] );
          return make_unique<FakeUnitView>(
              unit.type(), unit.nation(), unit.orders() );
        }
        case e_unit_kind::native:
          NativeUnit const& native_unit =
              ss.units.native_unit_for( units[idx] );
          e_tribe const tribe_type =
              tribe_for_unit( ss, native_unit ).type;
          return make_unique<FakeNativeUnitView>(
              native_unit.type, tribe_type );
      }
    }();
    auto labeled_box = make_unique<LabeledCheckBoxView>(
        std::move( icon_view ) );
    labeled_box->add_view( make_unique<TextView>(
        unit_type_name( ss, units[idx] ) ) );
    labeled_box->recompute_child_positions();
    boxes[idx] = labeled_box.get();
    boxes_array->add_view( std::move( labeled_box ) );
  }
  boxes_array->recompute_child_positions();
  top_array->add_view( std::move( boxes_array ) );
  // Add some space between boxes and buttons.
  top_array->add_view(
      make_unique<EmptyView>( Delta{ .w = 1, .h = 4 } ) );

  // Add buttons.
  auto buttons_view          = make_unique<ui::OkCancelView2>();
  ui::OkCancelView2* buttons = buttons_view.get();
  top_array->add_view( std::move( buttons_view ) );

  // Finalize top-level array.
  top_array->recompute_child_positions();

  // Create window.
  WindowManager& wm = ts.planes.get().window.typed().manager();
  Window         window( wm );
  window.set_view( std::move( top_array ) );
  window.autopad_me();
  // Must be done after auto-padding.
  window.center_me();

  ui::e_ok_cancel const finished = co_await buttons->next();
  EntitiesOnTile        entities;
  if( finished == ui::e_ok_cancel::cancel ) co_return entities;
  entities.units.reserve( units.size() );
  for( int idx = 0; idx < ssize( units ); ++idx ) {
    if( boxes[idx]->on() ) {
      CHECK_LT( idx, ssize( units ) );
      entities.units.push_back( units[idx] );
    }
  }
  co_return entities;
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
  Unit&       unit = ss.units.unit_for( unit_id );
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
    SSConst const& ss, e_nation const player_nation,
    gfx::point const tile ) {
  vector<GenericUnitId> const units_sorted = [&] {
    auto const units =
        units_from_coord_recursive( ss.units, tile );
    vector<GenericUnitId> res( units.begin(), units.end() );
    sort_unit_stack( ss, res );
    return res;
  }();
  DisbandingPermissions perms;
  for( auto const generic_unit_id : units_sorted ) {
    auto& v =
        can_disband_unit( ss, player_nation, generic_unit_id )
            ? perms.disbandable.units
            : perms.non_disbandable.units;
    v.push_back( generic_unit_id );
  }
  return perms;
}

wait<EntitiesOnTile> disband_tile_ui_interaction(
    SSConst const& ss, TS& ts,
    DisbandingPermissions const& perms ) {
  if( !perms.disbandable.units.empty() &&
      !perms.non_disbandable.units.empty() )
    // We won't check-fail here even though this should never
    // happen in the normal game because it is possible that the
    // player might end up creating such a situation in cheat
    // mode or mods, and we're not yet sure if we want to prevent
    // it at that level.
    lg.warn(
        "there are both disbandable and non-disbandable units "
        "on a single tile." );

  EntitiesOnTile entities;

  if( perms.disbandable.units.empty() ) {
    // Nothing to do here.
    if( !perms.non_disbandable.units.empty() )
      co_await ts.gui.message_box(
          "Cannot disband units of another nation." );
    co_return entities;
  }

  if( perms.disbandable.units.size() == 1 ) {
    GenericUnitId const unit_id = perms.disbandable.units[0];
    auto const          q = fmt::format( "Really disband [{}]?",
                                         unit_type_name( ss, unit_id ) );

    maybe<ui::e_confirm> const answer =
        co_await ts.gui.optional_yes_no(
            { .msg            = q,
              .yes_label      = "Yes",
              .no_label       = "No",
              .no_comes_first = true } );
    if( answer == ui::e_confirm::yes )
      entities.units.push_back( unit_id );
    co_return entities;
  }

  entities = co_await disband_selection_dialog(
      ss, ts, perms.disbandable.units );
  co_return entities;
}

void execute_disband( SS& ss, TS& ts,
                      EntitiesOnTile const& entities ) {
  for( GenericUnitId const id : entities.units ) {
    switch( ss.as_const.units.unit_kind( id ) ) {
      case e_unit_kind::euro: {
        UnitId const unit_id = ss.units.check_euro_unit( id );
        offload_if_ship_on_land_with_units( ss, ts, unit_id );
        UnitOwnershipChanger( ss, unit_id ).destroy();
        break;
      }
      case e_unit_kind::native:
        NativeUnitOwnershipChanger(
            ss, ss.units.check_native_unit( id ) )
            .destroy();
        break;
    }
  }
}

} // namespace rn
