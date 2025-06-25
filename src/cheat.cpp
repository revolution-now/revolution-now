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
#include "declare.hpp"
#include "fathers.hpp"
#include "fog-conv.hpp"
#include "game-options.hpp"
#include "icolony-evolve.rds.hpp"
#include "iengine.hpp"
#include "igui.hpp"
#include "imap-updater.hpp"
#include "imenu-server.hpp"
#include "interrupts.hpp"
#include "intervention.hpp"
#include "land-view.hpp"
#include "market.hpp"
#include "minds.hpp"
#include "plane-stack.hpp"
#include "player.rds.hpp"
#include "promotion.hpp"
#include "query-enum.hpp"
#include "rebel-sentiment.hpp"
#include "roles.hpp"
#include "settings.rds.hpp"
#include "succession.hpp"
#include "tribe-mgr.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
#include "views.hpp"
#include "visibility.hpp"
#include "white-box.hpp"
#include "window.hpp"

// config
#include "config/cheat.rds.hpp"
#include "config/colony.rds.hpp"
#include "config/nation.rds.hpp"
#include "config/natives.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colony.rds.hpp"
#include "ss/land-view.rds.hpp"
#include "ss/nation.hpp"
#include "ss/natives.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/unit.hpp"

// luapp
#include "luapp/register.hpp"

// gfx
#include "gfx/iter.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/logger.hpp"
#include "base/scope-exit.hpp"
#include "base/timer.hpp"
#include "base/to-str-ext-std.hpp"

// C++ standard library
#include <set>

using namespace std;

namespace rn {

namespace {

using ::gfx::point;
using ::refl::enum_derives_from;
using ::refl::enum_from_string;
using ::refl::enum_map;
using ::refl::enum_values;

bool can_remove_building( Colony const& colony,
                          e_colony_building building ) {
  if( !colony.buildings[building] ) return true;
  e_colony_building_slot slot = slot_for_building( building );
  UNWRAP_CHECK( foremost, building_for_slot( colony, slot ) );
  if( foremost != building ) return true;
  maybe<e_indoor_job> job = indoor_job_for_slot( slot );
  if( !job.has_value() ) return true;
  return colony.indoor_jobs[*job].empty();
}

void reveal_map_qol( SS& ss, TS& ts ) {
  // These technically don't need to be disabled, but it is just
  // a QoL thing for the player (actually the OG does this as
  // well).
  disable_game_option( ss, ts,
                       e_game_menu_option::show_indian_moves );
  disable_game_option( ss, ts,
                       e_game_menu_option::show_foreign_moves );
}

// This just decides if cheat functions should be enabled by de-
// fault when a new game starts. The actual per-game setting is
// stored in the game settings.
bool enable_cheat_mode_by_default() {
  return config_cheat.enable_cheat_mode_by_default;
}

} // namespace

/****************************************************************
** General.
*****************************************************************/
void enable_cheat_mode( SS& ss, TS& ts ) {
  if( !config_cheat.can_enable_cheat_mode ) return;
  ts.planes.get().menu.typed().enable_cheat_menu( true );
  ss.settings.cheat_options.enabled = true;
}

bool cheat_mode_enabled( SSConst const& ss ) {
  return ss.settings.cheat_options.enabled;
}

wait<> monitor_magic_key_sequence( co::stream<char>& chars ) {
  auto const& seq = config_cheat.magic_sequence;
  if( seq.empty() )
    // If there is no sequence to monitor then we will never
    // enter cheat mode and thus never return.
    co_await co::halt();
  while( true ) {
    for( char const next : seq )
      if( co_await chars.next() != next ) //
        goto start_over;
    co_return;
  start_over:;
  }
}

/****************************************************************
** In Land View
*****************************************************************/
maybe<point> cheat_target_square( SSConst const& ss, TS& ts ) {
  auto const& lv = ts.planes.get().get_bottom<ILandViewPlane>();

  if( auto const unit_id = lv.unit_blinking();
      unit_id.has_value() )
    return coord_for_unit_indirect_or_die( ss.units, *unit_id );

  if( auto const tile = lv.white_box(); tile.has_value() )
    return *tile;

  return nothing;
}

wait<> cheat_reveal_map( SS& ss, TS& ts ) {
  // This must be true otherwise the below will crash as it will
  // fail to map values from one enum to another.
  static_assert(
      enum_derives_from<e_player, e_cheat_reveal_map>() );
  // All enabled by default.
  refl::enum_map<e_cheat_reveal_map, bool> disabled;
  for( e_player player : enum_values<e_player> ) {
    if( !ss.players.players[player].has_value() ) {
      string_view const name = refl::enum_value_name( player );
      UNWRAP_CHECK(
          menu_item,
          enum_from_string<e_cheat_reveal_map>( name ) );
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
          MapRevealed::player{ .type = e_player::english };
      break;
    case e_cheat_reveal_map::french:
      revealed = MapRevealed::player{ .type = e_player::french };
      break;
    case e_cheat_reveal_map::spanish:
      revealed =
          MapRevealed::player{ .type = e_player::spanish };
      break;
    case e_cheat_reveal_map::dutch:
      revealed = MapRevealed::player{ .type = e_player::dutch };
      break;
    case e_cheat_reveal_map::ref_english:
      revealed =
          MapRevealed::player{ .type = e_player::ref_english };
      break;
    case e_cheat_reveal_map::ref_french:
      revealed =
          MapRevealed::player{ .type = e_player::ref_french };
      break;
    case e_cheat_reveal_map::ref_spanish:
      revealed =
          MapRevealed::player{ .type = e_player::ref_spanish };
      break;
    case e_cheat_reveal_map::ref_dutch:
      revealed =
          MapRevealed::player{ .type = e_player::ref_dutch };
      break;
    case e_cheat_reveal_map::entire_map:
      reveal_map_qol( ss, ts );
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

wait<> cheat_set_human_players( IEngine& engine, SS& ss,
                                TS& ts ) {
  struct Mapping {
    int idx = {};
    string label;
  };
  static vector<pair<e_player_control, Mapping>> const kSpec{
    { e_player_control::human,
      Mapping{ .idx = 0, .label = "Human" } },
    { e_player_control::ai, Mapping{ .idx = 1, .label = "AI" } },
    { e_player_control::withdrawn,
      Mapping{ .idx = 2, .label = "Withdrawn" } },
  };
  auto const& textometer = engine.textometer();
  // Use unique_ptr here because the radio button group must be
  // immobile which enum_map does not support.
  enum_map<e_player, unique_ptr<ui::RadioButtonGroup>> groups;

  // Right alignment here is necessary because the words in the
  // left column are variable length whereas the checkboxs in the
  // right column are all the same.
  auto top = make_unique<ui::VerticalArrayView>(
      ui::VerticalArrayView::align::right );

  auto const add_player = [&]( Player const& player ) {
    CHECK( !groups[player.type] );
    groups[player.type] = make_unique<ui::RadioButtonGroup>();
    ui::RadioButtonGroup& group = *groups[player.type];
    auto player_boxes = make_unique<ui::HorizontalArrayView>(
        ui::HorizontalArrayView::align::middle );
    for( auto const& [control, mapping] : kSpec ) {
      auto button_view =
          make_unique<ui::TextLabeledRadioButtonView>(
              group, textometer, mapping.label );
      auto* p_button_view = button_view.get();
      player_boxes->add_view( std::move( button_view ) );
      group.add( *p_button_view );
    }
    player_boxes->recompute_child_positions();
    for( auto const& [control, mapping] : kSpec )
      if( control == player.control ) //
        group.set( mapping.idx );
    auto labeled_player_boxes =
        make_unique<ui::HorizontalArrayView>(
            ui::HorizontalArrayView::align::middle );
    string const qualifier =
        is_ref( player.type )
            ? format( " ({})",
                      config_nation
                          .players[colonist_player_for(
                              nation_for( player.type ) )]
                          .possessive_pre_declaration )
            : "";
    string label_txt = format( "{}{}: ",
                               config_nation.players[player.type]
                                   .display_name_pre_declaration,
                               qualifier );
    auto label       = make_unique<ui::TextView>(
        textometer, std::move( label_txt ) );
    labeled_player_boxes->add_view( std::move( label ) );
    labeled_player_boxes->add_view( std::move( player_boxes ) );
    labeled_player_boxes->recompute_child_positions();
    top->add_view( std::move( labeled_player_boxes ) );
    top->add_view(
        make_unique<ui::EmptyView>( Delta{ .w = 1, .h = 4 } ) );
  };

  for( auto const& [type, player] : ss.players.players ) {
    if( !player.has_value() ) continue;
    add_player( *player );
  }
  top->recompute_child_positions();

  using ResultT = enum_map<e_player, e_player_control>;

  auto const get_selected = [&] {
    ResultT selected;
    for( auto const& [type, player] : ss.players.players ) {
      if( !player.has_value() ) continue;
      auto const& group = groups[type];
      CHECK( group != nullptr );
      UNWRAP_CHECK_T( int const idx, group->get_selected() );
      CHECK_LT( idx, ssize( kSpec ) );
      auto const& [control, mapping] = kSpec[idx];
      selected[type]                 = control;
    }
    return selected;
  };

  auto const has_at_least_one_human = [&]( ResultT const& res ) {
    for( auto const& [type, player] : ss.players.players ) {
      if( !player.has_value() ) continue;
      if( res[type] == e_player_control::human ) return true;
    }
    return false;
  };

  auto const loop = [&]() -> wait<ui::e_ok_cancel> {
    while( true ) {
      ui::e_ok_cancel const res = co_await ts.gui.ok_cancel_box(
          "Select Player Control", *top );
      if( res == ui::e_ok_cancel::cancel ) co_return res;
      auto const selected = get_selected();
      if( has_at_least_one_human( selected ) ) break;
      co_await ts.gui.message_box(
          "There must be at least one human player." );
    }
    co_return ui::e_ok_cancel::ok;
  };

  auto const ok = co_await loop();
  if( ok == ui::e_ok_cancel::cancel ) co_return;
  auto const selected = get_selected();

  bool change_made = false;
  for( auto& [type, player] : ss.players.players ) {
    if( !player.has_value() ) continue;
    e_player_control const new_value = selected[type];
    e_player_control& value          = player->control;
    if( new_value == value ) continue;
    change_made = true;
    value       = new_value;
  }

  if( !change_made ) co_return;

  ts.euro_minds() = create_euro_minds( ss, ts.gui );

  // We do this because we need to back out beyond the individual
  // nation's turn processor in order to handle this configura-
  // tion change properly. For example, if the current player is
  // a human and the above just changed it to an AI player then
  // we don't want to immediately resume having that player's
  // units ask for orders.
  throw top_of_turn_loop{};
}

void cheat_explore_entire_map( SS& ss, TS& ts ) {
  Rect const world_rect = ss.terrain.world_rect_tiles();
  maybe<e_player> const player =
      player_for_role( ss, e_player_role::viewer );
  if( !player.has_value() )
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
                .mutable_player_terrain( *player )
                .map;
  for( point const p : gfx::rect_iterator( world_rect ) ) {
    Coord const coord = Coord::from_gfx( p ); // FIXME
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
  ts.map_updater().redraw();
}

void cheat_toggle_reveal_full_map( SS& ss, TS& ts ) {
  MapRevealed& revealed = ss.land_view.map_revealed;
  if( revealed.holds<MapRevealed::no_special_view>() ||
      revealed.holds<MapRevealed::player>() ) {
    reveal_map_qol( ss, ts );
    revealed = MapRevealed::entire{};
  } else {
    revealed = MapRevealed::no_special_view{};
  }
  update_map_visibility(
      ts, player_for_role( ss, e_player_role::viewer ) );
}

wait<> cheat_edit_fathers( IEngine& engine, SS& ss, TS& ts,
                           Player& player ) {
  using namespace ui;

  auto const& textometer = engine.textometer();

  auto top_array = make_unique<VerticalArrayView>(
      VerticalArrayView::align::center );

  // Add text.
  auto text_view = make_unique<TextView>(
      textometer, "Select or unselect founding fathers:" );
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
       enum_values<e_founding_father> ) {
    auto labeled_box = make_unique<TextLabeledCheckBoxView>(
        textometer, string( founding_father_name( father ) ),
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
  auto buttons_view =
      make_unique<ui::OkCancelView2>( textometer );
  ui::OkCancelView2* buttons = buttons_view.get();
  top_array->add_view( std::move( buttons_view ) );

  // Finalize top-level array.
  top_array->recompute_child_positions();

  // Create window.
  WindowManager& wm = ts.planes.get().window.typed().manager();
  Window window( wm );
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
  constexpr auto& tribes = enum_values<e_tribe>;

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

  co_await ts.planes.get().get_bottom<ILandViewPlane>().animate(
      anim_seq_for_cheat_kill_natives( ss, *viz, destroyed ) );

  for( e_tribe const tribe : destroyed )
    destroy_tribe( ss, ts.map_updater(), tribe );

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
  for( e_player const player : enum_values<e_player> ) {
    if( !ss.players.players[player].has_value() ) continue;
    vector<Coord> const affected_fogged = [&] {
      vector<Coord> res;
      res.reserve( affected_coords.size() );
      VisibilityForPlayer const viz( ss, player );
      for( Coord const tile : affected_coords )
        if( viz.visible( tile ) == e_tile_visibility::fogged )
          res.push_back( tile );
      return res;
    }();
    ts.map_updater().make_squares_visible( player,
                                           affected_fogged );
    ts.map_updater().make_squares_fogged( player,
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
    if( ts.map_updater().options().player == player )
      ts.map_updater().force_redraw_tiles( affected_fogged );
  }

  for( e_tribe const tribe : destroyed )
    co_await tribe_wiped_out_message( ts, tribe );
}

// In the OG there are four stages to the revolution status:
//
//   1. Rebel sentiment < 50%.
//   2. Rebel sentiment hits 50%; War of Spanish Succession per-
//      formed if not yet performed.
//   3. Independence declared / war begins.
//   4. Foreign intervention force deployed.
//   5. Independence won.
//
wait<> cheat_advance_revolution_status( SS& ss, TS& ts,
                                        Player& player ) {
  if( player.control != e_player_control::human ) {
    co_await ts.gui.message_box(
        "Cannot perform this operation for player {} because "
        "they are not human-controlled.",
        player.type );
    co_return;
  }

  // Bump rebel sentiment to 50% and do the war of succession if
  // needed.
  int const required_sentiment =
      required_rebel_sentiment_for_declaration( ss );
  if( player.revolution.status < e_revolution_status::declared &&
      player.revolution.rebel_sentiment < required_sentiment ) {
    RebelSentimentChangeReport const change_report{
      .prev = player.revolution.rebel_sentiment,
      .nova = required_sentiment };
    player.revolution.rebel_sentiment = required_sentiment;
    co_await show_rebel_sentiment_change_report(
        player, ts.euro_minds()[player.type], change_report );
    if( should_do_war_of_succession( as_const( ss ),
                                     as_const( player ) ) ) {
      WarOfSuccessionNations const nations =
          select_players_for_war_of_succession( ss.as_const );
      WarOfSuccessionPlan const plan =
          war_of_succession_plan( ss.as_const, nations );
      do_war_of_succession( ss, ts, player, plan );
      co_await do_war_of_succession_ui_seq( ts, plan );
    }
    co_return;
  }

  // Rebel sentiment already sufficiently high and/or War of suc-
  // cession already done. Check if independence has been de-
  // clared.
  if( player.revolution.status <
      e_revolution_status::declared ) {
    co_await declare_independence_ui_sequence_pre(
        ss.as_const, ts, as_const( player ) );
    DeclarationResult const decl_res =
        declare_independence( ss, ts, player );
    co_await declare_independence_ui_sequence_post(
        ss.as_const, ts, as_const( player ), decl_res );
    CHECK_GE( player.revolution.status,
              e_revolution_status::declared );
    co_return;
  }

  if( !player.revolution.intervention_force_deployed ) {
    int const bells_needed =
        bells_required_for_intervention( ss.settings );
    if( player.bells < bells_needed )
      player.bells = bells_needed;
    if( should_trigger_intervention( ss.as_const,
                                     as_const( player ) ) ) {
      trigger_intervention( player );
      e_nation const intervention_nation =
          select_nation_for_intervention( player.nation );
      co_await intervention_forces_triggered_ui_seq(
          ss, ts.gui, player.nation, intervention_nation );
    }
    co_return;
  }

  if( player.revolution.status < e_revolution_status::won ) {
    // In this case, "end of the REF's next turn" means at the
    // end of the REF's turn in this same turn cycle.
    co_await ts.gui.message_box(
        "The War of Independence will be won at the end of the "
        "REF's next turn." );
    player.revolution.ref_will_forfeit = true;
    co_return;
  }

  co_await ts.gui.message_box(
      "The War of Independence has already been won." );
}

/****************************************************************
** In Harbor View
*****************************************************************/
void cheat_increase_tax_rate( Player& player ) {
  int& rate = player.old_world.taxes.tax_rate;
  rate      = clamp( rate + 1, 0, 100 );
}

void cheat_decrease_tax_rate( Player& player ) {
  int& rate = player.old_world.taxes.tax_rate;
  rate      = clamp( rate - 1, 0, 100 );
}

void cheat_increase_gold( Player& player ) {
  int& gold = player.money;
  gold      = gold + 1000;
}

void cheat_decrease_gold( Player& player ) {
  int& gold = player.money;
  gold      = std::max( gold - 1000, 0 );
}

wait<> cheat_evolve_market_prices( SS& ss, TS& ts,
                                   Player& player ) {
  evolve_group_model_volumes( ss );
  refl::enum_map<e_commodity, PriceChange> const changes =
      evolve_player_prices( static_cast<SSConst const&>( ss ),
                            player );
  for( e_commodity comm : enum_values<e_commodity> )
    if( changes[comm].delta != 0 )
      co_await display_price_change_notification(
          ts, player, changes[comm] );
}

void cheat_toggle_boycott( Player& player, e_commodity type ) {
  bool& boycott =
      player.old_world.market.commodities[type].boycott;
  boycott = !boycott;
}

/****************************************************************
** In Colony View
*****************************************************************/
wait<> cheat_colony_buildings( Colony& colony, IGui& gui ) {
  maybe<e_cheat_colony_buildings_option> mode =
      co_await gui.optional_enum_choice<
          e_cheat_colony_buildings_option>();
  if( !mode.has_value() ) co_return;
  switch( *mode ) {
    case e_cheat_colony_buildings_option::give_all_buildings:
      for( e_colony_building building :
           enum_values<e_colony_building> )
        add_colony_building( colony, building );
      break;
    case e_cheat_colony_buildings_option::remove_all_buildings: {
      bool can_not_remove_all = false;
      for( e_colony_building building :
           enum_values<e_colony_building> ) {
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
  // Non-interactive works here because currently we're only
  // using this to create a unit inside the colony view.
  create_unit_on_map_non_interactive( ss, ts, player,
                                      e_unit_type::free_colonist,
                                      colony.location );
}

void cheat_increase_commodity( Colony& colony,
                               e_commodity type ) {
  int& quantity = colony.commodities[type];
  quantity += 50;
  quantity = quantity - ( quantity % 50 );
}

void cheat_decrease_commodity( Colony& colony,
                               e_commodity type ) {
  int& quantity = colony.commodities[type];
  if( quantity % 50 != 0 )
    quantity -= ( quantity % 50 );
  else
    quantity -= 50;
  quantity = std::max( quantity, 0 );
}

void cheat_advance_colony_one_turn(
    IColonyEvolver const& colony_evolver, Colony& colony ) {
  lg.debug( "advancing colony {}. notifications:", colony.name );
  ColonyEvolution ev =
      colony_evolver.evolve_colony_one_turn( colony );
  for( ColonyNotification const& notification :
       ev.notifications )
    lg.debug( "{}", notification );
  // NOTE: we will not starve the colony here since this is just
  // a cheat/debug feature. We'll just log that it was supposed
  // to happen.
  if( ev.colony_disappeared ) lg.debug( "colony has starved." );
}

wait<> cheat_create_unit_on_map( SS& ss, TS& ts,
                                 e_player const player_type,
                                 point const tile ) {
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
        e_unit_type::expert_teacher,
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
  UNWRAP_CHECK( player, ss.players.players[player_type] );
  // TODO:
  //   * If we are trying to add a land unit onto an ocean tile
  //     then we should prevent it unless there is a ship on that
  //     tile that can hold the unit, in which case we should add
  //     the unit as cargo onto the ship.
  [[maybe_unused]] maybe<UnitId> unit_id =
      co_await create_unit_on_map( ss, ts, player, *type,
                                   Coord::from_gfx( tile ) );
}

/****************************************************************
** Lua
*****************************************************************/
namespace {

LUA_AUTO_FN( enable_cheat_mode_by_default );

}

} // namespace rn
