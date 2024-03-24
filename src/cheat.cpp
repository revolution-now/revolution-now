/****************************************************************
**cheat.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-16.
*
* Description: Implements cheat mode.
*
*****************************************************************/
#include "cheat.hpp"

// Rds
#include "cheat-impl.rds.hpp"

// Revolution Now
#include "anim-builders.hpp"
#include "co-wait.hpp"
#include "colony-buildings.hpp"
#include "colony-evolve.hpp"
#include "fathers.hpp"
#include "fog-conv.hpp"
#include "game-options.hpp"
#include "igui.hpp"
#include "imap-updater.hpp"
#include "interrupts.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "market.hpp"
#include "minds.hpp"
#include "plane-stack.hpp"
#include "promotion.hpp"
#include "roles.hpp"
#include "settings.rds.hpp"
#include "tribe-mgr.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
#include "unit.hpp"
#include "views.hpp"
#include "visibility.hpp"
#include "window.hpp"

// config
#include "config/colony.rds.hpp"
#include "config/nation.rds.hpp"
#include "config/natives.rds.hpp"
#include "config/rn.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colony.rds.hpp"
#include "ss/land-view.rds.hpp"
#include "ss/natives.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"

// gfx
#include "gfx/iter.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/scope-exit.hpp"
#include "base/timer.hpp"
#include "base/to-str-ext-std.hpp"

// C++ standard library
#include <set>

using namespace std;

namespace rn {

#define CO_RETURN_IF_NO_CHEAT \
  if( !config_rn.cheat_functions_enabled ) co_return

#define RETURN_IF_NO_CHEAT \
  if( !config_rn.cheat_functions_enabled ) return

namespace {

bool can_remove_building( Colony const&     colony,
                          e_colony_building building ) {
  if( !colony.buildings[building] ) return true;
  e_colony_building_slot slot = slot_for_building( building );
  UNWRAP_CHECK( foremost, building_for_slot( colony, slot ) );
  if( foremost != building ) return true;
  maybe<e_indoor_job> job = indoor_job_for_slot( slot );
  if( !job.has_value() ) return true;
  return colony.indoor_jobs[*job].empty();
}

} // namespace

/****************************************************************
** In Land View
*****************************************************************/
wait<> cheat_reveal_map( SS& ss, TS& ts ) {
  CO_RETURN_IF_NO_CHEAT;
  // All enabled by default.
  refl::enum_map<e_cheat_reveal_map, bool> disabled;
  for( e_nation nation : refl::enum_values<e_nation> ) {
    if( !ss.players.players[nation].has_value() ) {
      string_view const name = refl::enum_value_name( nation );
      UNWRAP_CHECK(
          menu_item,
          refl::enum_from_string<e_cheat_reveal_map>( name ) );
      disabled[menu_item] = true;
    }
  }
  maybe<e_cheat_reveal_map> const selected =
      co_await ts.gui.optional_enum_choice<e_cheat_reveal_map>(
          disabled );
  if( !selected.has_value() ) co_return;

  MapRevealed revealed;

  switch( *selected ) {
    case e_cheat_reveal_map::english:
      revealed =
          MapRevealed::nation{ .nation = e_nation::english };
      break;
    case e_cheat_reveal_map::french:
      revealed =
          MapRevealed::nation{ .nation = e_nation::french };
      break;
    case e_cheat_reveal_map::spanish:
      revealed =
          MapRevealed::nation{ .nation = e_nation::spanish };
      break;
    case e_cheat_reveal_map::dutch:
      revealed =
          MapRevealed::nation{ .nation = e_nation::dutch };
      break;
    case e_cheat_reveal_map::entire_map:
      // These technically don't need to be disabled, but it is
      // just a QoL thing for the player (actually the OG does
      // this as well).
      disable_game_option(
          ss, ts, e_game_flag_option::show_indian_moves );
      disable_game_option(
          ss, ts, e_game_flag_option::show_foreign_moves );
      revealed = MapRevealed::entire{};
      break;
    case e_cheat_reveal_map::no_special_view:
      revealed = MapRevealed::no_special_view{};
      break;
  }

  // Need to set this before calling update_map_visibility.
  ss.land_view.map_revealed = revealed;
  update_map_visibility(
      ts, player_for_role( ss, e_player_role::viewer ) );
}

wait<> cheat_set_human_players( SS& ss, TS& ts ) {
  CO_RETURN_IF_NO_CHEAT;
  // All enabled by default.
  refl::enum_map<e_nation, CheckBoxInfo> info_map;
  for( e_nation nation : refl::enum_values<e_nation> ) {
    info_map[nation] = CheckBoxInfo{
        .name     = config_nation.nations[nation].display_name,
        .on       = false,
        .disabled = true };
    if( !ss.players.players[nation].has_value() ) {
      info_map[nation].disabled = true;
      info_map[nation].on       = false;
      continue;
    }
    info_map[nation].disabled = false;
    info_map[nation].on       = ss.players.humans[nation];
  }

  while( true ) {
    co_await ts.gui.enum_check_boxes<e_nation>(
        "Select Human Nations:", info_map );
    bool found_human = false;
    for( e_nation nation : refl::enum_values<e_nation> )
      if( info_map[nation].on ) found_human = true;
    if( found_human ) break;
    co_await ts.gui.message_box(
        "There must be at least one human player." );
  }

  // Set new human statuses.
  for( e_nation nation : refl::enum_values<e_nation> )
    ss.players.humans[nation] = info_map[nation].on;

  ts.euro_minds = create_euro_minds( ss, ts.gui );

  // We do this because we need to back out beyond the individual
  // nation's turn processor in order to handle this configura-
  // tion change properly. For example, if the current player is
  // a human and the above just changed it to an AI player then
  // we don't want to immediately resume having that player's
  // units ask for orders.
  throw top_of_turn_loop{};
}

void cheat_explore_entire_map( SS& ss, TS& ts ) {
  RETURN_IF_NO_CHEAT;
  Rect const world_rect = ss.terrain.world_rect_tiles();
  maybe<e_nation> const nation =
      player_for_role( ss, e_player_role::viewer );
  if( !nation.has_value() )
    // Entire map is already visible, no need to do anything.
    return;
  base::ScopedTimer timer( "explore entire map" );
  // In a sense, what we'd ideally be doing here is just calling
  // `make_squares_visible` with the map_updater on all tiles.
  // However, there are two problems with that. One, it would re-
  // move fog from all of the tiles on the map which is both un-
  // necessary and wasteful because on the very next turn most of
  // that fog would have to be regenerated. Second, even though
  // make_squares_visible is a batch API that will only render
  // each square once, it will do so on the annex buffers, which
  // will then cause the entire map to be re-rendered multiple
  // times throughout the process, slowing it down. The below is
  // as effecient as can be since it will leave the fog buffer
  // completely untouched and will redraw the map only once.
  auto& m = ss.mutable_terrain_use_with_care
                .mutable_player_terrain( *nation )
                .map;
  for( Coord const coord : gfx::rect_iterator( world_rect ) ) {
    // This will reveal the square to the player with fog if the
    // square was not already explored but with existing fog
    // status if it was already explored.
    auto frozen_square = [&]() -> maybe<FrozenSquare&> {
      SWITCH( m[coord] ) {
        CASE( unexplored ) {
          return m[coord]
              .emplace<PlayerSquare::explored>()
              .fog_status.emplace<FogStatus::fogged>()
              .contents;
        }
        CASE( explored ) {
          SWITCH( explored.fog_status ) {
            CASE( fogged ) { return fogged.contents; }
            CASE( clear ) { return nothing; }
          }
        }
      }
    }();
    if( frozen_square.has_value() )
      copy_real_square_to_frozen_square( ss, coord,
                                         *frozen_square );
  }
  ts.map_updater.redraw();
}

void cheat_toggle_reveal_full_map( SS& ss, TS& ts ) {
  RETURN_IF_NO_CHEAT;
  MapRevealed& revealed = ss.land_view.map_revealed;
  if( revealed.holds<MapRevealed::no_special_view>() ||
      revealed.holds<MapRevealed::nation>() )
    revealed = MapRevealed::entire{};
  else
    revealed = MapRevealed::no_special_view{};
  update_map_visibility(
      ts, player_for_role( ss, e_player_role::viewer ) );
}

wait<> cheat_edit_fathers( SS& ss, TS& ts, Player& player ) {
  using namespace ui;

  auto top_array = make_unique<VerticalArrayView>(
      VerticalArrayView::align::center );

  // Add text.
  auto text_view = make_unique<TextView>(
      "Select or unselect founding fathers:" );
  top_array->add_view( std::move( text_view ) );
  // Add some space between title and check boxes.
  top_array->add_view(
      make_unique<EmptyView>( Delta{ .w = 1, .h = 2 } ) );

  // Add vertical split, since there are too many fathers to put
  // them all vertically.
  auto vsplit_array = make_unique<HorizontalArrayView>(
      HorizontalArrayView::align::up );

  // Add check boxes.
  auto l_boxes_array = make_unique<VerticalArrayView>(
      VerticalArrayView::align::left );
  auto r_boxes_array = make_unique<VerticalArrayView>(
      VerticalArrayView::align::left );

  refl::enum_map<e_founding_father, LabeledCheckBoxView const*>
      boxes;
  for( e_founding_father father :
       refl::enum_values<e_founding_father> ) {
    auto labeled_box = make_unique<LabeledCheckBoxView>(
        string( founding_father_name( father ) ),
        player.fathers.has[father] );
    boxes[father] = labeled_box.get();
    if( static_cast<int>( father ) < 13 )
      l_boxes_array->add_view( std::move( labeled_box ) );
    else
      r_boxes_array->add_view( std::move( labeled_box ) );
  }
  l_boxes_array->recompute_child_positions();
  r_boxes_array->recompute_child_positions();

  vsplit_array->add_view( std::move( l_boxes_array ) );
  vsplit_array->add_view( std::move( r_boxes_array ) );
  vsplit_array->recompute_child_positions();
  top_array->add_view( std::move( vsplit_array ) );
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
  WindowManager& wm = ts.planes.window().manager();
  Window         window( wm );
  window.set_view( std::move( top_array ) );
  window.autopad_me();
  // Must be done after auto-padding.
  window.center_me();

  ui::e_ok_cancel const finished = co_await buttons->next();
  if( finished == ui::e_ok_cancel::cancel ) co_return;

  for( auto [father, box] : boxes ) {
    bool const had_previously  = player.fathers.has[father];
    bool const has_now         = box->on();
    player.fathers.has[father] = has_now;
    if( has_now && !had_previously )
      on_father_received( ss, ts, player, father );
  }
}

wait<> kill_natives( SS& ss, TS& ts ) {
  CO_RETURN_IF_NO_CHEAT;
  constexpr auto& tribes = refl::enum_values<e_tribe>;

  // Is there anything to do?
  bool const at_least_one = any_of(
      tribes.begin(), tribes.end(), [&]( e_tribe tribe ) {
        return ss.natives.tribe_exists( tribe );
      } );
  if( !at_least_one ) {
    co_await ts.gui.message_box(
        "All native tribes have been wiped out." );
    co_return;
  }

  // Create and open window.
  refl::enum_map<e_tribe, CheckBoxInfo> info_map;
  for( e_tribe const tribe : tribes )
    info_map[tribe] = {
        .name     = config_natives.tribes[tribe].name_singular,
        .on       = false,
        .disabled = !ss.natives.tribe_exists( tribe ),
    };
  // Note that if the user cancels this box then the values will
  // be unchanged, so effectively nothing will happen.
  co_await ts.gui.enum_check_boxes<e_tribe>(
      "Select Native Tribes to Kill:", info_map );

  // Collect results.
  set<e_tribe> const destroyed = [&] {
    set<e_tribe> res;
    for( e_tribe const tribe : tribes )
      if( !info_map[tribe].disabled && info_map[tribe].on )
        res.insert( tribe );
    return res;
  }();

  if( destroyed.empty() ) co_return;

  // Before we destroy the dwellings we need to record where they
  // were so that we can later update the maps.
  vector<Coord> const affected_coords = [&] {
    vector<Coord> res;
    res.reserve( ss.natives.dwellings_all().size() );
    for( e_tribe const tribe : destroyed ) {
      unordered_set<DwellingId> const& dwellings =
          ss.natives.dwellings_for_tribe( tribe );
      for( DwellingId const dwelling_id : dwellings )
        res.push_back( ss.natives.coord_for( dwelling_id ) );
    }
    return res;
  }();

  unique_ptr<IVisibility const> const viz =
      create_visibility_for(
          ss, player_for_role( ss, e_player_role::viewer ) );

  co_await ts.planes.land_view().animate(
      anim_seq_for_cheat_kill_natives( ss, *viz, destroyed ) );

  for( e_tribe const tribe : destroyed )
    destroy_tribe( ss, ts.map_updater, tribe );

  // At this point we need to update the fogged squares that con-
  // tained destroyed dwellings on the player maps to remove the
  // fog dwellings. The simplest way to do that using the usual
  // map updater API is to just go through all of the squares
  // that had fogged dwellings on them that were destroyed and
  // flip the fog on and then off again. This is not very ele-
  // gant, but it will suffice. Note that we don't have to do
  // this for dwellings that were totally hidden and we don't
  // have to do this for dwellings that were fully visible and
  // clear, since in the latter case those fog squares will even-
  // tually get updated if/when the square flips to fogged.
  for( e_nation const nation : refl::enum_values<e_nation> ) {
    if( !ss.players.players[nation].has_value() ) continue;
    vector<Coord> const affected_fogged = [&] {
      vector<Coord> res;
      res.reserve( affected_coords.size() );
      VisibilityForNation const viz( ss, nation );
      for( Coord const tile : affected_coords )
        if( viz.visible( tile ) == e_tile_visibility::fogged )
          res.push_back( tile );
      return res;
    }();
    ts.map_updater.make_squares_visible( nation,
                                         affected_fogged );
    ts.map_updater.make_squares_fogged( nation,
                                        affected_fogged );
    // This is actually not necessary since the tile will get re-
    // drawn anyway (either when we destroy the dwelling or when
    // we flip the fog squares above, depending on visibility
    // state) because the tile loses the road when the dwelling
    // gets destroyed, so the map updater understands that the
    // tile has to be redrawn. However, even if there were not a
    // road being lost, we would still in general need to redraw
    // the tile anyway since a prime resource might get revealed.
    // So, just out of principle so that we're not relying on the
    // presence of a road under the dwelling to render a prime
    // resource, we will anyway force a redraw of the tile.
    if( ts.map_updater.options().nation == nation )
      ts.map_updater.force_redraw_tiles( affected_fogged );
  }

  for( e_tribe const tribe : destroyed )
    co_await tribe_wiped_out_message( ts, tribe );
}

/****************************************************************
** In Harbor View
*****************************************************************/
void cheat_increase_tax_rate( Player& player ) {
  RETURN_IF_NO_CHEAT;
  int& rate = player.old_world.taxes.tax_rate;
  rate      = clamp( rate + 1, 0, 100 );
}

void cheat_decrease_tax_rate( Player& player ) {
  RETURN_IF_NO_CHEAT;
  int& rate = player.old_world.taxes.tax_rate;
  rate      = clamp( rate - 1, 0, 100 );
}

void cheat_increase_gold( Player& player ) {
  RETURN_IF_NO_CHEAT;
  int& gold = player.money;
  gold      = gold + 1000;
}

void cheat_decrease_gold( Player& player ) {
  RETURN_IF_NO_CHEAT;
  int& gold = player.money;
  gold      = std::max( gold - 1000, 0 );
}

wait<> cheat_evolve_market_prices( SS& ss, TS& ts,
                                   Player& player ) {
  CO_RETURN_IF_NO_CHEAT;
  evolve_group_model_volumes( ss );
  refl::enum_map<e_commodity, PriceChange> const changes =
      evolve_player_prices( static_cast<SSConst const&>( ss ),
                            player );
  for( e_commodity comm : refl::enum_values<e_commodity> )
    if( changes[comm].delta != 0 )
      co_await display_price_change_notification(
          ts, player, changes[comm] );
}

void cheat_toggle_boycott( Player& player, e_commodity type ) {
  RETURN_IF_NO_CHEAT;
  bool& boycott =
      player.old_world.market.commodities[type].boycott;
  boycott = !boycott;
}

/****************************************************************
** In Colony View
*****************************************************************/
wait<> cheat_colony_buildings( Colony& colony, IGui& gui ) {
  CO_RETURN_IF_NO_CHEAT;
  maybe<e_cheat_colony_buildings_option> mode =
      co_await gui.optional_enum_choice<
          e_cheat_colony_buildings_option>();
  if( !mode.has_value() ) co_return;
  switch( *mode ) {
    case e_cheat_colony_buildings_option::give_all_buildings:
      for( e_colony_building building :
           refl::enum_values<e_colony_building> )
        add_colony_building( colony, building );
      break;
    case e_cheat_colony_buildings_option::remove_all_buildings: {
      bool can_not_remove_all = false;
      for( e_colony_building building :
           refl::enum_values<e_colony_building> ) {
        if( can_remove_building( colony, building ) )
          colony.buildings[building] = false;
        else
          can_not_remove_all = true;
      }
      if( can_not_remove_all )
        co_await gui.message_box(
            "Unable to remove all buildings since some of them "
            "contain colonists." );
      break;
    }
    case e_cheat_colony_buildings_option::set_default_buildings:
      colony.buildings = config_colony.initial_colony_buildings;
      break;
    case e_cheat_colony_buildings_option::add_one_building: {
      maybe<e_colony_building> building =
          co_await gui.optional_enum_choice<e_colony_building>(
              /*sort=*/true );
      if( building.has_value() )
        add_colony_building( colony, *building );
      break;
    }
    case e_cheat_colony_buildings_option::remove_one_building: {
      maybe<e_colony_building> building =
          co_await gui.optional_enum_choice<e_colony_building>(
              /*sort=*/true );
      if( !building.has_value() ) co_return;
      if( !can_remove_building( colony, *building ) ) {
        co_await gui.message_box(
            "Cannot remove this building while it contains "
            "colonists working in it." );
        co_return;
      }
      colony.buildings[*building] = false;
      break;
    }
  }
}

void cheat_upgrade_unit_expertise( SS& ss, TS& ts, Unit& unit ) {
  RETURN_IF_NO_CHEAT;
  UnitType const original_type = unit.type_obj();
  SCOPE_EXIT {
    lg.debug( "{} --> {}", original_type, unit.type_obj() );
  };
  if( !is_unit_a_colonist( unit.type_obj() ) ) return;

  // First just use the normal game logic to attempt a promotion.
  // This will work in many cases but not all. It won't do a pro-
  // motion when the unit has no activity and it won't change an
  // expert's type to be a new expert, both things that we will
  // want to do with this cheat feature.
  if( try_promote_unit_for_current_activity( ss, ts, unit ) )
    return;

  maybe<e_unit_activity> const activity =
      current_activity_for_unit( ss.units, ss.colonies,
                                 unit.id() );

  if( activity.has_value() ) {
    if( unit_attr( unit.base_type() ).expertise == *activity )
      return;
    // This is for when we have a unit that is already an expert
    // at something other than the given activity, we want to
    // switch them to being an expert at the given activity.
    UnitType to_promote;
    if( unit.desc().is_derived )
      // Take the canonical version of the thing try to promote
      // it (this is needed if it's a derived type).
      to_promote = unit.type();
    else
      // Not derived, so just try promoting a free colonist.
      to_promote = e_unit_type::free_colonist;
    expect<UnitComposition> promoted =
        promoted_from_activity( to_promote, *activity );
    if( !promoted.has_value() ) return;
    CHECK( promoted.has_value() );
    change_unit_type( ss, ts, unit, *promoted );
    return;
  }

  // No activity. without an activity, the only people we know
  // how to upgrade are petty criminals and indentured servants.
  switch( unit.type() ) {
    case e_unit_type::petty_criminal:
      change_unit_type( ss, ts, unit,
                        e_unit_type::indentured_servant );
      return;
    case e_unit_type::indentured_servant:
      change_unit_type( ss, ts, unit,
                        e_unit_type::free_colonist );
      break;
    default:
      return;
  }
}

void cheat_downgrade_unit_expertise( SS& ss, TS& ts,
                                     Unit& unit ) {
  RETURN_IF_NO_CHEAT;
  UnitType const original_type = unit.type_obj();
  SCOPE_EXIT {
    lg.debug( "{} --> {}", original_type, unit.type_obj() );
  };
  if( !is_unit_a_colonist( unit.type_obj() ) ) return;

  if( !unit.desc().is_derived ) {
    e_unit_type new_type = {};
    switch( unit.type() ) {
      case e_unit_type::petty_criminal:
        return;
      case e_unit_type::indentured_servant:
        new_type = e_unit_type::petty_criminal;
        break;
      case e_unit_type::free_colonist:
        new_type = e_unit_type::indentured_servant;
        break;
      default:
        new_type = e_unit_type::free_colonist;
        break;
    }
    change_unit_type( ss, ts, unit, new_type );
    return;
  }

  // Derived type.
  switch( unit.base_type() ) {
    case e_unit_type::petty_criminal:
      return;
    case e_unit_type::indentured_servant: {
      UNWRAP_CHECK(
          ut, UnitType::create( unit.type(),
                                e_unit_type::petty_criminal ) );
      UNWRAP_CHECK( comp,
                    UnitComposition::create(
                        ut, unit.composition().inventory() ) );
      change_unit_type( ss, ts, unit, comp );
      return;
    }
    case e_unit_type::free_colonist: {
      UNWRAP_CHECK( ut, UnitType::create(
                            unit.type(),
                            e_unit_type::indentured_servant ) );
      UNWRAP_CHECK( comp,
                    UnitComposition::create(
                        ut, unit.composition().inventory() ) );
      change_unit_type( ss, ts, unit, comp );
      return;
    }
    default:
      maybe<UnitType> cleared =
          cleared_expertise( unit.type_obj() );
      if( !cleared.has_value() )
        // not sure what to do here.
        return;
      UNWRAP_CHECK(
          comp, UnitComposition::create(
                    *cleared, unit.composition().inventory() ) );
      change_unit_type( ss, ts, unit, comp );
      return;
  }
}

void cheat_create_new_colonist( SS& ss, TS& ts,
                                Player const& player,
                                Colony const& colony ) {
  RETURN_IF_NO_CHEAT;
  // Non-interactive works here because currently we're only
  // using this to create a unit inside the colony view.
  create_unit_on_map_non_interactive( ss, ts, player,
                                      e_unit_type::free_colonist,
                                      colony.location );
}

void cheat_increase_commodity( Colony&     colony,
                               e_commodity type ) {
  RETURN_IF_NO_CHEAT;
  int& quantity = colony.commodities[type];
  quantity += 50;
  quantity = quantity - ( quantity % 50 );
}

void cheat_decrease_commodity( Colony&     colony,
                               e_commodity type ) {
  RETURN_IF_NO_CHEAT;
  int& quantity = colony.commodities[type];
  if( quantity % 50 != 0 )
    quantity -= ( quantity % 50 );
  else
    quantity -= 50;
  quantity = std::max( quantity, 0 );
}

void cheat_advance_colony_one_turn( SS& ss, TS& ts,
                                    Player& player,
                                    Colony& colony ) {
  RETURN_IF_NO_CHEAT;
  lg.debug( "advancing colony {}. notifications:", colony.name );
  ColonyEvolution ev =
      evolve_colony_one_turn( ss, ts, player, colony );
  for( ColonyNotification const& notification :
       ev.notifications )
    lg.debug( "{}", notification );
  // NOTE: we will not starve the colony here since this is just
  // a cheat/debug feature. We'll just log that it was supposed
  // to happen.
  if( ev.colony_disappeared ) lg.debug( "colony has starved." );
}

wait<> cheat_create_unit_on_map( SS& ss, TS& ts, e_nation nation,
                                 Coord tile ) {
  CO_RETURN_IF_NO_CHEAT;
  static refl::enum_map<e_cheat_unit_creation_categories,
                        vector<e_unit_type>> const categories{
      { e_cheat_unit_creation_categories::basic_colonists,
        {
            e_unit_type::free_colonist,
            e_unit_type::indentured_servant,
            e_unit_type::petty_criminal,
            e_unit_type::native_convert,
        } },
      { e_cheat_unit_creation_categories::modified_colonists,
        {
            e_unit_type::pioneer,
            e_unit_type::hardy_pioneer,
            e_unit_type::missionary,
            e_unit_type::jesuit_missionary,
            e_unit_type::scout,
            e_unit_type::seasoned_scout,
        } },
      { e_cheat_unit_creation_categories::expert_colonists,
        {
            e_unit_type::expert_farmer,
            e_unit_type::expert_fisherman,
            e_unit_type::expert_sugar_planter,
            e_unit_type::expert_tobacco_planter,
            e_unit_type::expert_cotton_planter,
            e_unit_type::expert_fur_trapper,
            e_unit_type::expert_lumberjack,
            e_unit_type::expert_ore_miner,
            e_unit_type::expert_silver_miner,
            e_unit_type::master_carpenter,
            e_unit_type::master_distiller,
            e_unit_type::master_tobacconist,
            e_unit_type::master_weaver,
            e_unit_type::master_fur_trader,
            e_unit_type::master_blacksmith,
            e_unit_type::master_gunsmith,
            e_unit_type::elder_statesman,
            e_unit_type::firebrand_preacher,
            e_unit_type::hardy_colonist,
            e_unit_type::jesuit_colonist,
            e_unit_type::seasoned_colonist,
            e_unit_type::veteran_colonist,
        } },
      { e_cheat_unit_creation_categories::armies,
        {
            e_unit_type::soldier,
            e_unit_type::dragoon,
            e_unit_type::veteran_soldier,
            e_unit_type::veteran_dragoon,
            e_unit_type::continental_army,
            e_unit_type::continental_cavalry,
            e_unit_type::regular,
            e_unit_type::cavalry,
            e_unit_type::artillery,
            e_unit_type::damaged_artillery,
        } },
      { e_cheat_unit_creation_categories::ships,
        {
            e_unit_type::caravel,
            e_unit_type::merchantman,
            e_unit_type::galleon,
            e_unit_type::privateer,
            e_unit_type::frigate,
            e_unit_type::man_o_war,
        } },
      { e_cheat_unit_creation_categories::miscellaneous,
        {
            e_unit_type::wagon_train,
            e_unit_type::treasure,
        } },
  };
  maybe<e_cheat_unit_creation_categories> category =
      co_await ts.gui.optional_enum_choice<
          e_cheat_unit_creation_categories>();
  if( !category.has_value() ) co_return;
  maybe<e_unit_type> type =
      co_await ts.gui.partial_optional_enum_choice<e_unit_type>(
          categories[*category] );
  if( !type.has_value() ) co_return;
  UNWRAP_CHECK( player, ss.players.players[nation] );
  [[maybe_unused]] maybe<UnitId> unit_id =
      co_await create_unit_on_map( ss, ts, player, *type, tile );
}

} // namespace rn
