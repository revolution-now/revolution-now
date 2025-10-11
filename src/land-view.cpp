/****************************************************************
**land-view.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-29.
*
* Description: Handles the main game view of the land.
*
*****************************************************************/
#include "land-view.hpp"

// Rds
#include "land-view-impl.rds.hpp"

// Revolution Now
#include "anim-builders.hpp"
#include "camera.hpp"
#include "cheat.hpp"
#include "co-combinator.hpp"
#include "co-time.hpp"
#include "command.hpp"
#include "goto.hpp"
#include "hidden-terrain.hpp"
#include "iengine.hpp"
#include "imap-updater.hpp"
#include "imenu-handler.hpp"
#include "imenu-server.hpp"
#include "land-view-anim.hpp"
#include "land-view-render.hpp"
#include "map-view.hpp"
#include "omni.hpp"
#include "physics.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "roles.hpp"
#include "screen.hpp" // FIXME: remove
#include "time.hpp"
#include "ts.hpp"
#include "unit-id.hpp"
#include "unit-mgr.hpp"
#include "viewport.hpp"
#include "visibility.hpp"
#include "white-box.hpp"
#include "window.hpp"

// config
#include "config/land-view.rds.hpp"
#include "config/menu-items.rds.hpp"
#include "config/ui.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/land-view.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// gfx
#include "gfx/coord.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp" // IWYU pragma: keep

// base
#include "base/logger.hpp"
#include "base/scope-exit.hpp"

// luapp
#include "luapp/enum.hpp" // IWYU pragma: keep
#include "luapp/state.hpp"

// C++ standard library
#include <chrono>
#include <queue>

#define SCOPED_MODE_PUSH_AND_GET( mode, type )       \
  auto& mode = mode_.emplace( LandViewMode::type{} ) \
                   .get<LandViewMode::type>();       \
  SCOPE_EXIT { mode_.pop(); };

using namespace std;

namespace rn {

namespace {

using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

struct RawInput {
  RawInput( LandViewRawInput input_ )
    : input( std::move( input_ ) ), when( Clock_t::now() ) {}
  LandViewRawInput input;
  Time_t when = {};
};

struct PlayerInput {
  PlayerInput( LandViewPlayerInput input_, Time_t when_ )
    : input( std::move( input_ ) ), when( when_ ) {}
  LandViewPlayerInput input;
  Time_t when;
};

// Holds info about the previous unit that was asking for orders,
// since it can affect the UI behavior when asking for the cur-
// rent unit's orders (just some niceties that make it easier for
// the player to accurately control multiple units that are al-
// ternately asking for orders).
//
// This class also functions to guard against usage of unit IDs
// representing units that no longer exist, which is a bit hard
// to track otherwise.
struct LastUnitInput {
  [[maybe_unused]] explicit LastUnitInput( SSConst const& ss,
                                           UnitId const id )
    : ss_( ss ), unit_id_( id ) {}

  maybe<UnitId> unit_id() const {
    maybe<UnitId> res;
    if( ss_.units.exists( unit_id_ ) ) res = unit_id_;
    return res;
  }

 private:
  SSConst const& ss_;
  // Should not allow direct access to this because it could rep-
  // resent a unit that no longer exists.
  UnitId unit_id_ = {};

 public:
  bool need_input_buffer_shield = false;
  // Records the total number of windows that have been opened
  // thus far when the unit last asked for orders. That way, if
  // the same unit asks for orders again, we can check this to
  // see if any windows were opened during the course of the last
  // move and, if so, clear the input buffers.
  int window_count = 0;
};

} // namespace

struct LandViewPlane::Impl : public IPlane, public IMenuHandler {
  IEngine& engine_;
  SS& ss_;
  TS& ts_;
  unique_ptr<IVisibility const> viz_;
  ViewportController viewport_;
  Camera camera_;
  LandViewAnimator animator_;

  vector<IMenuServer::Deregistrar> dereg;

  co::stream<RawInput> raw_input_stream_;
  queue<PlayerInput> translated_input_stream_;
  stack<LandViewMode> mode_;

  maybe<co::stream<point>> white_box_stream_;

  maybe<LastUnitInput> last_unit_input_;

  // For convenience.
  maybe<UnitId> last_unit_input_id() const {
    if( !last_unit_input_.has_value() ) return nothing;
    return last_unit_input_->unit_id();
  }

  maybe<InputOverrunIndicator> input_overrun_indicator_;

  bool g_needs_scroll_to_unit_on_input = true;

  ViewportController const& viewport() const {
    return viewport_;
  }

  ViewportController& viewport() { return viewport_; }

  void register_menu_items( IMenuServer& menu_server ) {
    // Register menu handlers.
    dereg.push_back( menu_server.register_handler(
        e_menu_item::cheat_create_unit_on_map, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::cheat_reveal_map, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::zoom_in, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::zoom_out, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::restore_zoom, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::find_blinking_unit, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::sentry, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::fortify, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::disband, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::wait, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::build_colony, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::return_to_europe, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::dump, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::plow, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::road, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::activate, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::hidden_terrain, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::view_mode, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::move, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::go_to, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::return_to_europe, *this ) );
    dereg.push_back( menu_server.register_handler(
        e_menu_item::no_orders, *this ) );
  }

  Impl( IEngine& engine, SS& ss, TS& ts, maybe<e_player> player )
    : engine_( engine ),
      ss_( ss ),
      ts_( ts ),
      viz_( create_visibility_for( ss, nothing ) ),
      viewport_( engine_, ss.terrain, ss.land_view.viewport,
                 landview_renderable_rect() ),
      camera_( engine_.user_config(), ss.land_view.viewport ),
      animator_( engine_.sfx(), ss, viewport_, viz_ ) {
    set_visibility( player );
    CHECK( viz_ != nullptr );
    register_menu_items( ts.planes.get().menu );
    // Initialize general global data.
    mode_.push( LandViewMode::none{} );
    last_unit_input_ = nothing;
    raw_input_stream_.reset();
    translated_input_stream_        = {};
    g_needs_scroll_to_unit_on_input = true;
  }

  maybe<point> white_box_tile_if_visible() const {
    bool visible = false;
    SWITCH( mode_.top() ) {
      CASE( none ) { break; }
      CASE( end_of_turn ) {
        visible = true;
        break;
      }
      CASE( goto_mode ) { break; }
      CASE( hidden_terrain ) {
        visible = true;
        break;
      }
      CASE( unit_input ) { break; }
      CASE( view_mode ) {
        visible = true;
        break;
      }
    }
    if( visible ) return white_box_tile( ss_ );
    return nothing;
  }

  maybe<point> find_tile_to_center_on() const {
    // First try white box.
    if( auto const p = white_box_tile_if_visible();
        p.has_value() )
      return *p;

    // Next try blinking units.
    SWITCH( mode_.top() ) {
      CASE( none ) { break; }
      CASE( end_of_turn ) { break; }
      CASE( goto_mode ) { return goto_mode.curr_tile; }
      CASE( hidden_terrain ) { break; }
      CASE( unit_input ) {
        return coord_for_unit_indirect_or_die(
            ss_.units, unit_input.unit_id );
      }
      CASE( view_mode ) { break; }
    }

    return nothing;
  }

  /****************************************************************
  ** Tile Clicking
  *****************************************************************/
  // If there is a single unit on the square with orders then the
  // unit's orders will be cleared and the unit will be placed at
  // the back of the queue to potentially move this turn.
  //
  // If there is a single unit on the square with no orders then
  // the unit will be prioritized (moved to the front of the
  // queue).
  //
  // If there are multiple units on the square then it will pop
  // open a window to allow the user to select and/or activate
  // them, with the results for each unit behaving in a similar
  // way to the single-unit case described above.
  wait<vector<LandViewPlayerInput>> click_on_world_tile(
      Coord coord ) {
    vector<LandViewPlayerInput> res;

    auto const scroll_map = [&] {
      // Nothing to click on, so just scroll the map to center on
      // the clicked tile.
      viewport().center_on_tile( coord );
    };

    if( white_box_stream_.has_value() )
      white_box_stream_->send( coord );

    if( mode_.top().holds<LandViewMode::hidden_terrain>() ) {
      scroll_map();
      co_return res;
    }

    auto add = [&res]<typename T>( T t ) -> T& {
      res.push_back( std::move( t ) );
      return res.back().get<T>();
    };

    // First check for colonies.
    if( auto const colony_id =
            can_open_colony_on_tile( *viz_, coord );
        colony_id.has_value() ) {
      auto& colony = add( LandViewPlayerInput::colony{} );
      colony.id    = *colony_id;
      co_return res;
    }

    // Now check for units.
    bool const mode_allows_activate =
        mode_.top().holds<LandViewMode::unit_input>() ||
        mode_.top().holds<LandViewMode::view_mode>() ||
        mode_.top().holds<LandViewMode::end_of_turn>();
    auto const units =
        can_activate_units_on_tile( ss_, *viz_, coord );
    if( !units.empty() && mode_allows_activate ) {
      // Decide which units are selected and for what actions.
      vector<UnitSelection> selections;
      if( units.size() == 1 ) {
        auto id = *units.begin();
        UnitSelection selection{
          id, e_unit_selection::clear_orders };
        if( !ss_.units.unit_for( id ).has_orders() )
          selection.what = e_unit_selection::activate;
        selections = vector{ selection };
      } else {
        selections = co_await unit_selection_box(
            engine_.textometer(), ss_, ts_.planes.get().window,
            units,
            UnitActivationOptions{ .allow_prioritizing_multiple =
                                       true } );
      }

      vector<UnitId> prioritize;
      for( auto const& selection : selections ) {
        switch( selection.what ) {
          case e_unit_selection::clear_orders:
            ss_.units.unit_for( selection.id ).clear_orders();
            break;
          case e_unit_selection::activate:
            // Activation implies also to clear orders if they're
            // not already cleared. We do this here because, even
            // if the prioritization is later denied (because the
            // unit has already moved this turn) the clearing of
            // the orders should still be upheld, because that
            // can always be done, hence they are done sepa-
            // rately.
            ss_.units.unit_for( selection.id ).clear_orders();
            prioritize.push_back( selection.id );
            break;
        }
      }
      // These need to all be grouped into a vector so that the
      // first prioritized unit doesn't start asking for orders
      // before the rest are prioritized.
      if( !prioritize.empty() )
        add( LandViewPlayerInput::prioritize{} ).units =
            std::move( prioritize );

      co_return res;
    }

    scroll_map();
    co_return res;
  }

  wait<vector<LandViewPlayerInput>> right_click_on_world_tile(
      gfx::point const tile ) {
    vector<LandViewPlayerInput> res;

    if( white_box_stream_.has_value() )
      white_box_stream_->send( tile );

    auto add = [&res]<typename T>( T t ) -> T& {
      res.push_back( std::move( t ) );
      return res.back().get<T>();
    };

    if( mode_.top().holds<LandViewMode::unit_input>() ) {
      add( LandViewPlayerInput::view_mode{
        .options = ViewModeOptions{ .initial_tile = tile } } );
      co_return res;
    }

    co_return res;
  }

  wait<> context_menu( point const where,
                       point const /*tile*/ ) {
    viewport().stop_auto_zoom();
    viewport().stop_auto_panning();
    rect const r = landview_renderable_rect();
    // This will ensure that the menu opens in a direction such
    // that it doesn't get hidden beyond the bounds of the view-
    // port, if the mouse is too near to the edge.
    e_diagonal_direction const orientation = [&] {
      return where.x < r.center().x
                 ? where.y < r.center().y
                       ? e_diagonal_direction::nw
                       : e_diagonal_direction::sw
             : where.y < r.center().y ? e_diagonal_direction::ne
                                      : e_diagonal_direction::se;
    }();
    MenuAllowedPositions const positions{
      .positions_allowed = {
        { .where = where, .orientation = orientation } } };
    IMenuServer& menu_server = ts_.planes.get().menu.typed();
    auto const selected_item = co_await menu_server.open_menu(
        e_menu::land_view, positions );
    if( !selected_item.has_value() ) co_return;
    bool const handled =
        menu_server.click_item( *selected_item );
    (void)handled;
  }

  /****************************************************************
  ** Tile Entering
  *****************************************************************/
  // This happens typically when the white box is visible and the
  // enter or return keys are hit on a square.
  wait<vector<LandViewPlayerInput>> enter_on_world_tile(
      gfx::point const tile ) {
    vector<LandViewPlayerInput> res;

    auto add = [&res]<typename T>( T t ) -> T& {
      res.push_back( std::move( t ) );
      return res.back().get<T>();
    };

    // First check for colonies.
    if( auto const colony_id =
            can_open_colony_on_tile( *viz_, tile );
        colony_id.has_value() ) {
      auto& colony = add( LandViewPlayerInput::colony{} );
      colony.id    = *colony_id;
      co_return res;
    }

    co_return res;
  }

  wait<maybe<LandViewPlayerInput>> activate_tile(
      point const tile ) {
    maybe<LandViewPlayerInput> res;
    auto const units =
        can_activate_units_on_tile( ss_, *viz_, tile );
    if( units.empty() ) co_return res;

    // Decide which units are selected and for what actions.
    vector<UnitSelection> selections;
    if( units.size() == 1 ) {
      auto id = *units.begin();
      UnitSelection selection{ id,
                               e_unit_selection::clear_orders };
      if( !ss_.units.unit_for( id ).has_orders() )
        selection.what = e_unit_selection::activate;
      selections = vector{ selection };
    } else {
      selections = co_await unit_selection_box(
          engine_.textometer(), ss_, ts_.planes.get().window,
          units,
          UnitActivationOptions{ .allow_prioritizing_multiple =
                                     false } );
    }

    for( auto const& selection : selections ) {
      switch( selection.what ) {
        case e_unit_selection::clear_orders:
          ss_.units.unit_for( selection.id ).clear_orders();
          break;
        case e_unit_selection::activate:
          // Activation implies also to clear orders if they're
          // not already cleared. We do this here because, even
          // if the prioritization is later denied (because the
          // unit has already moved this turn) the clearing of
          // the orders should still be upheld, because that
          // can always be done, hence they are done sepa-
          // rately.
          ss_.units.unit_for( selection.id ).clear_orders();
          // We specified in the options that the unit selection
          // box should only allow one unit to be prioritized,
          // thus there is no ambiguity in result here.
          res = LandViewPlayerInput::activate{
            .unit = selection.id };
          break;
      }
    }

    // NOTE: there will only be an item added to the result if
    // the player prioritized any units. But either way, some
    // units may have had their orders cleared.

    co_return res;
  }

  wait<maybe<point>> goto_mode( point const start ) {
    // Set land view mode.
    SCOPED_MODE_PUSH_AND_GET( mode, goto_mode );
    mode.start_tile = start;
    mode.curr_tile  = start;

    // Set goto mouse cursor.
    OmniPlane& omni = ts_.planes.get().omni.typed();
    e_mouse_cursor const old_cursor = omni.get_mouse_cursor();
    omni.set_mouse_cursor( e_mouse_cursor::go_to );
    SCOPE_EXIT { omni.set_mouse_cursor( old_cursor ); };

    // Take updates and wait for drag thread to finish.
    for( ;; ) {
      RawInput const raw = co_await raw_input_stream_.next();
      SWITCH( raw.input ) {
        CASE( goto_drag_update ) {
          mode.curr_tile = goto_drag_update.tile;
          break;
        }
        CASE( goto_drag_cancel ) { co_return nothing; }
        CASE( goto_drag_finish ) { co_return mode.curr_tile; }
        CASE( escape ) { co_return nothing; }
        default:
          break;
      }
    }
    co_return nothing;
  }

  // NOTE: this is not for the goto selection where the mouse
  // drags to the target; this is mainly for keyboard driven in-
  // put, though it supports the mouse as well. Point is, it is
  // NOT a mouse drag.
  wait<maybe<point>> select_goto_tile( Unit const& unit ) {
    point const start =
        coord_for_unit_indirect_or_die( ss_.units, unit.id() );

    // Set land view mode.
    SCOPED_MODE_PUSH_AND_GET( mode, goto_mode );
    mode.start_tile = start;
    mode.curr_tile  = start;

    // Set goto mouse cursor.
    OmniPlane& omni = ts_.planes.get().omni.typed();
    e_mouse_cursor const old_cursor = omni.get_mouse_cursor();
    omni.set_mouse_cursor( e_mouse_cursor::go_to );
    SCOPE_EXIT { omni.set_mouse_cursor( old_cursor ); };

    rect const map_rect = viewport().world_rect_tiles();

    // Center up front, and then only center below on certain
    // events. In particular, we don't want to center each time
    // the mouse moves, otherwise the screen scrolls too fast.
    co_await animator_.ensure_visible( mode.curr_tile );

    // Take updates and wait for a tile to be selected either
    // with the keyboard or mouse click. Likely it'll be the key-
    // board here since this is mostly for keyboard driven goto
    // input.
    for( ;; ) {
      RawInput const raw = co_await raw_input_stream_.next();
      SWITCH( raw.input ) {
        CASE( tile_enter ) { co_return mode.curr_tile; }
        CASE( tile_click ) {
          // Before this click, the curr_tile will have been
          // moved to be where the mouse tile is.
          co_return mode.curr_tile;
        }
        CASE( cmd ) {
          SWITCH( cmd.what ) {
            CASE( move ) {
              mode.curr_tile = mode.curr_tile.moved( move.d );
              // Here it is ok to scroll because this is in re-
              // sponse to the keyboard, so the scrolling won't
              // get out of hand as it would for when the target
              // is moved by the mouse.
              co_await animator_.ensure_visible(
                  mode.curr_tile );
              break;
            }
            CASE( forfeight ) {
              // This is the space bar, which we will interpret
              // as "accept tile and go".
              co_return mode.curr_tile;
            }
            default:
              break;
          }
          break;
        }
        CASE( center ) {
          co_await center_on_tile( mode.curr_tile );
          break;
        }
        CASE( escape ) { co_return nothing; }
        CASE( tile_right_click ) { co_return nothing; }
        CASE( mouse_click_outside_of_map ) {
          co_return mode.curr_tile;
        }
        CASE( mouse_over ) {
          // NOTE: this tile could be off the map, but it will be
          // clamped appropriately below.
          mode.curr_tile = mouse_over.hypothetical_tile;
          break;
        }
        default:
          break;
      }
      // The the player can see beyond the left/right edges of
      // the map (due to zoom level) then allow one extra tile on
      // the left/right so that the player can indicate that they
      // want to go to the harbor. We need to check this in each
      // loop because the player may change the zoom level as
      // they are selecting tiles.
      rect const bounds =
          viewport().is_fully_visible_x()
              ? map_rect //
                    .with_new_left_edge( map_rect.left() - 1 )
                    .with_new_right_edge( map_rect.right() + 1 )
                    .with_dec_size()
              : map_rect.with_dec_size();
      mode.curr_tile = mode.curr_tile.clamped( bounds );
    }
    co_return nothing;
  }

  /****************************************************************
  ** Input Processor
  *****************************************************************/
  // Fetches one raw input and translates it, adding a new ele-
  // ment into the "translated" stream. For each translated event
  // created, preserve the time that the corresponding raw input
  // event was received.
  wait<> single_raw_input_translator() {
    using RI = LandViewRawInput;
    using PI = LandViewPlayerInput;

    RawInput raw_input = co_await raw_input_stream_.next();

    switch( raw_input.input.to_enum() ) {
      using e = RI::e;
      case e::goto_drag_start: {
        auto& o = raw_input.input.get<RI::goto_drag_start>();
        auto const end_tile = co_await goto_mode( o.tile );
        if( !end_tile.has_value() ) break;
        UNWRAP_CHECK_T(
            UnitId const unit_id,
            mode_.top().inner_if<LandViewMode::unit_input>() );
        Unit const& unit = ss_.units.unit_for( unit_id );
        auto const input = PI::give_command{
          .cmd = command::go_to{
            .target = create_goto_map_target(
                ss_, unit.player_type(), *end_tile ) } };
        // NOTE: we use the current time here because the drag
        // have lasted a while and so if we use raw_input.when
        // then the event may get discarded as being too old.
        translated_input_stream_.push(
            PlayerInput( input, Clock_t::now() ) );
        break;
      }
      case e::goto_drag_update:
      case e::goto_drag_finish:
        // These can be sent after a drag is cancelled while the
        // drag thread is finishing up.
        break;
      case e::goto_drag_cancel:
        // This happens because these messages are sent even
        // after a normal drag operation finishes, it is harm-
        // less.
        break;
      case e::reveal_map: {
        if( !cheat_mode_enabled( ss_ ) ) break;
        co_await cheat_reveal_map( ss_, ts_ );
        break;
      }
      case e::toggle_map_reveal: {
        if( !cheat_mode_enabled( ss_ ) ) break;
        cheat_toggle_reveal_full_map( ss_, ts_ );
        break;
      }
      case e::cheat_create_unit: {
        if( !cheat_mode_enabled( ss_ ) ) break;
        maybe<e_player> const player =
            player_for_role( ss_, e_player_role::active );
        if( !player.has_value() ) break;
        auto const tile = cheat_target_square( ss_, ts_ );
        if( !tile.has_value() ) break;
        co_await cheat_create_unit_on_map( ss_, ts_, *player,
                                           *tile );
        break;
      }
      case e::escape: {
        translated_input_stream_.push(
            PlayerInput( PI::exit{}, raw_input.when ) );
        break;
      }
      case e::next_turn: {
        if( !mode_.top().holds<LandViewMode::end_of_turn>() )
          // We're not supposed to send these events when we're
          // not in EOT mode. However, because of the fact that
          // input events can be buffered, we will be defensive
          // here and check if one of these snuck into the input
          // stream while we were in EOT mode but we now no
          // longer are.
          break;
        translated_input_stream_.push(
            PlayerInput( PI::next_turn{}, raw_input.when ) );
        break;
      }
      case e::cmd: {
        translated_input_stream_.push( PlayerInput(
            PI::give_command{
              .cmd = raw_input.input.get<RI::cmd>().what },
            raw_input.when ) );
        break;
      }
      case e::european_status: {
        translated_input_stream_.push( PlayerInput(
            PI::european_status{}, raw_input.when ) );
        break;
      }
      case e::hidden_terrain: {
        translated_input_stream_.push( PlayerInput(
            PI::hidden_terrain{}, raw_input.when ) );
        break;
      }
      case e::view_mode: {
        auto& o = raw_input.input.get<RI::view_mode>();
        translated_input_stream_.push(
            PlayerInput( PI::view_mode{ .options = o.options },
                         raw_input.when ) );
        break;
      }
      case e::activate: {
        auto& o = raw_input.input.get<RI::activate>();
        maybe<PI> const input = co_await activate_tile( o.tile );
        // Since we may have just popped open a box to ask the
        // user to select units, just use the "now" time so
        // that these events don't get disgarded. Also, mouse
        // clicks are not likely to get buffered for too long
        // anyway.
        if( input.has_value() )
          translated_input_stream_.push(
              PlayerInput( *input, Clock_t::now() ) );
        break; //
      }
      case e::move_mode: {
        translated_input_stream_.push(
            PlayerInput( PI::move_mode{}, Clock_t::now() ) );
        break; //
      }
      case e::tile_click: {
        auto& o = raw_input.input.get<RI::tile_click>();
        if( o.mods.shf_down ) {
          // cheat mode.
          if( !cheat_mode_enabled( ss_ ) ) break;
          maybe<e_player> const player =
              player_for_role( ss_, e_player_role::active );
          if( !player.has_value() ) break;
          co_await cheat_create_unit_on_map( ss_, ts_, *player,
                                             o.coord );
          break;
        }
        vector<PI> inputs =
            co_await click_on_world_tile( o.coord );
        // Since we may have just popped open a box to ask the
        // user to select units, just use the "now" time so
        // that these events don't get disgarded. Also, mouse
        // clicks are not likely to get buffered for too long
        // anyway.
        for( auto const& input : inputs )
          translated_input_stream_.push(
              PlayerInput( input, Clock_t::now() ) );
        break;
      }
      case e::tile_right_click: {
        auto& o = raw_input.input.get<RI::tile_right_click>();
        vector<PI> inputs =
            co_await right_click_on_world_tile( o.coord );
        for( auto const& input : inputs )
          translated_input_stream_.push(
              PlayerInput( input, raw_input.when ) );
        break;
      }
      case e::tile_enter: {
        auto& o = raw_input.input.get<RI::tile_enter>();
        vector<PI> inputs =
            co_await enter_on_world_tile( o.tile );
        for( auto const& input : inputs )
          translated_input_stream_.push(
              PlayerInput( input, raw_input.when ) );
        break;
      }
      case e::center: {
        // For this one, we just perform the action right here.
        auto const tile = find_tile_to_center_on();
        if( tile.has_value() ) co_await center_on_tile( *tile );
        break;
      }
      case e::context_menu: {
        auto& o = raw_input.input.get<RI::context_menu>();
        co_await context_menu( o.where, o.tile );
        break;
      }
      case e::goto_tile: {
        auto const unit_input =
            mode_.top().get_if<LandViewMode::unit_input>();
        if( !unit_input.has_value() ) break;
        Unit const& unit =
            ss_.units.unit_for( unit_input->unit_id );
        auto const tile = co_await select_goto_tile( unit );
        if( !tile.has_value() ) break;
        // Use current time since there was some user interac-
        // tion.
        translated_input_stream_.push( PlayerInput(
            PI::give_command{
              .cmd =
                  command::go_to{
                    .target = create_goto_map_target(
                        ss_, unit.player_type(), *tile ) } },
            Clock_t::now() ) );
        break;
      }
      case e::goto_port: {
        auto const unit_input =
            mode_.top().get_if<LandViewMode::unit_input>();
        if( !unit_input.has_value() ) break;
        Unit const& unit =
            ss_.units.unit_for( unit_input->unit_id );
        point const tile = coord_for_unit_indirect_or_die(
            ss_.units, unit.id() );
        GotoPort const goto_port = find_goto_port(
            ss_, ts_.connectivity, unit.player_type(),
            unit.type(), tile );
        UNWRAP_CHECK_T(
            Player const& player,
            ss_.players.players[unit.player_type()] );
        auto const goto_target = co_await ask_goto_port(
            ss_, ts_.gui, player, goto_port, unit.type() );
        if( !goto_target.has_value() ) break;
        // Use current time since a box popped open.
        translated_input_stream_.push( PlayerInput(
            PI::give_command{
              .cmd = command::go_to{ .target = *goto_target } },
            Clock_t::now() ) );
        break;
      }
      case e::goto_harbor: {
        translated_input_stream_.push( PlayerInput(
            PI::give_command{
              .cmd =
                  command::go_to{ .target =
                                      goto_target::harbor{} } },
            raw_input.when ) );
        break;
      }
      case e::mouse_over: {
        // TBD.
        break;
      }
      case e::zoom_changed: {
        // TBD.
        break;
      }
      case e::mouse_click_outside_of_map: {
        // TBD.
        break;
      }
    }
  }

  wait<LandViewPlayerInput> next_player_input_object() {
    while( true ) {
      while( translated_input_stream_.empty() )
        co_await single_raw_input_translator();
      CHECK( !translated_input_stream_.empty() );
      PlayerInput res = translated_input_stream_.front();
      translated_input_stream_.pop();
      // Ignore any input events that are too old.
      if( Clock_t::now() - res.when < chrono::seconds{ 2 } )
        co_return std::move( res.input );
    }
  }

  void scroll_when_mouse_pos_at_edge() {
    point const mouse_pos = input::current_mouse_position();
    rect const outter     = landview_renderable_rect();
    size const n_trim =
        ( outter.size.to_double() *
          config_land_view.scrolling.edge_thickness_percent )
            .truncated();
    if( n_trim == size{} )
      // This allows us to configure edge_thickness_percent to be
      // 0.0 to effectively disable this scrolling.
      return;
    auto const trim = [&]( rect const r ) {
      return r.moved( n_trim )
          .with_dec_size( n_trim )
          .with_dec_size( n_trim );
    };

    rect const inner = trim( outter );
    if( mouse_pos.is_inside( inner ) ) return;

    // The idea here is that if at least one of the mouse dimen-
    // sions is on the outter most border then we will do the
    // panning, but we'll use inner2 which has even larger mar-
    // gins. This provides for a more natural experience because
    // it effectively provides larger regions at the corners
    // where both x and y will scroll which makes it easier for
    // the user to scroll diagonally. Without this, the scrolling
    // tends to flip between pure horizontal or pure vertical
    // (unless the mouse is really in the corner) which makes it
    // feel a bit weird. We didn't want to solve this by lowering
    // the above trimming percent because then that would make
    // all scrolling too sensitive.
    rect const inner2 = trim( trim( inner ) );
    // Use e.g. > and not >= so that when the edge thickness is
    // configured to be zero then we don't scroll.
    bool const push_x_positive = mouse_pos.x > inner2.right();
    bool const push_x_negative = mouse_pos.x < inner2.left();
    bool const push_y_positive = mouse_pos.y > inner2.bottom();
    bool const push_y_negative = mouse_pos.y < inner2.top();

    using enum e_push_direction;
    if( push_x_positive ) viewport().set_x_push( positive );
    if( push_y_positive ) viewport().set_y_push( positive );
    if( push_x_negative ) viewport().set_x_push( negative );
    if( push_y_negative ) viewport().set_y_push( negative );
  }

  /****************************************************************
  ** Land View IPlane
  *****************************************************************/
  rect landview_renderable_rect() const {
    rect r = main_window_logical_rect( engine_.video(),
                                       engine_.window(),
                                       engine_.resolutions() );
    return r.with_new_top_edge( config_ui.menus.menu_bar_height )
        .with_new_right_edge( r.right() -
                              config_ui.panel.width );
  }

  void advance_viewport_state() {
    viewport().advance_state();

    // TODO: should only do the following when the viewport has
    // input focus.
    auto const* __state = ::SDL_GetKeyboardState( nullptr );

    // Returns true if key is pressed.
    auto state = [__state]( ::SDL_Scancode code ) {
      return __state[code] != 0;
    };

    if( state( ::SDL_SCANCODE_LCTRL ) ) {
      viewport().set_x_push( state( ::SDL_SCANCODE_A )
                                 ? e_push_direction::negative
                             : state( ::SDL_SCANCODE_D )
                                 ? e_push_direction::positive
                                 : e_push_direction::none );
      // y motion
      viewport().set_y_push( state( ::SDL_SCANCODE_W )
                                 ? e_push_direction::negative
                             : state( ::SDL_SCANCODE_S )
                                 ? e_push_direction::positive
                                 : e_push_direction::none );

      if( state( ::SDL_SCANCODE_A ) ||
          state( ::SDL_SCANCODE_D ) ||
          state( ::SDL_SCANCODE_W ) ||
          state( ::SDL_SCANCODE_S ) )
        viewport().stop_auto_panning();
    }

    // Let the mouse push when it is near the edge of the visible
    // map to help scroll to the goto target.
    if( mode_.top().holds<LandViewMode::goto_mode>() )
      scroll_when_mouse_pos_at_edge();
  }

  maybe<command> try_orders_from_lua( int keycode,
                                      bool ctrl_down,
                                      bool shf_down ) {
    lua::state& st = ts_.lua;

    lua::any lua_orders = st["land_view"]["key_to_orders"](
        keycode, ctrl_down, shf_down );
    if( !lua_orders ) return nothing;

    // FIXME: the conversion from Lua to C++ below needs to be
    // au- tomated once we have sumtype and struct reflection.
    // When this is done then ideally we will just be able to do
    // this:
    //
    //   command command = lua_orders.as<command>();
    //
    // And it should do the correct conversion and error
    // checking.
    lua::table tbl = lua_orders.as<lua::table>();
    command command;
    if( false )
      ;
    else if( tbl["wait"] )
      command = command::wait{};
    else if( tbl["forfeight"] )
      command = command::forfeight{};
    else if( tbl["build"] )
      command = command::build{};
    else if( tbl["fortify"] )
      command = command::fortify{};
    else if( tbl["sentry"] )
      command = command::sentry{};
    else if( tbl["disband"] )
      ; // handled in c++
    else if( tbl["road"] )
      command = command::road{};
    else if( tbl["plow"] )
      command = command::plow{};
    else if( tbl["move"] ) {
      e_direction d = tbl["move"]["d"].as<e_direction>();
      command       = command::move{ .d = d };
    } else {
      FATAL(
          "invalid command::move object received from "
          "lua." );
    }
    return command;
  }

  void advance_state() override { advance_viewport_state(); }

  void draw( rr::Renderer& renderer ) const override {
    LandViewRenderer const lv_renderer(
        ss_, renderer, animator_, viz_, last_unit_input_id(),
        landview_renderable_rect(), input_overrun_indicator_,
        viewport(), ts_.map_updater() );

    lv_renderer.render_non_entities();
    lv_renderer.render_entities();

    if( auto const goto_mode =
            mode_.top().get_if<LandViewMode::goto_mode>();
        goto_mode.has_value() ) {
      point const start = goto_mode->start_tile;
      point const end   = goto_mode->curr_tile;
      lv_renderer.render_goto( start, end );
    }
  }

  maybe<function<void()>> menu_click_handler(
      e_menu_item item ) {
    // These are factors by which the zoom will be scaled when
    // zooming in/out with the menus.
    double constexpr zoom_in_factor  = 2.0;
    double constexpr zoom_out_factor = 1.0 / zoom_in_factor;
    // This is so that a zoom-in followed by a zoom-out will re-
    // store to previous state.
    static_assert( zoom_in_factor * zoom_out_factor == 1.0 );
    switch( item ) {
      case e_menu_item::cheat_create_unit_on_map: {
        if( !mode_.top().holds<LandViewMode::unit_input>() &&
            !mode_.top().holds<LandViewMode::view_mode>() &&
            !mode_.top().holds<LandViewMode::end_of_turn>() )
          break;
        auto handler = [this] {
          raw_input_stream_.send( RawInput(
              LandViewRawInput::cheat_create_unit{} ) );
        };
        return handler;
      }
      case e_menu_item::cheat_reveal_map: {
        if( !mode_.top().holds<LandViewMode::unit_input>() &&
            !mode_.top().holds<LandViewMode::view_mode>() &&
            !mode_.top().holds<LandViewMode::end_of_turn>() )
          break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::reveal_map{} ) );
        };
        return handler;
      }
      case e_menu_item::view_mode: {
        if( !mode_.top().holds<LandViewMode::unit_input>() )
          break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::view_mode{} ) );
        };
        return handler;
      }
      case e_menu_item::activate: {
        if( !mode_.top().holds<LandViewMode::end_of_turn>() &&
            !mode_.top().holds<LandViewMode::view_mode>() )
          break;
        // This menu item is only expected to be clicked when
        // the white box tile is visible.
        CHECK( white_box_stream_.has_value() );
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::activate{
                .tile = white_box_tile( ss_ ) } ) );
        };
        return handler;
      }
      case e_menu_item::move: {
        if( !mode_.top().holds<LandViewMode::end_of_turn>() &&
            !mode_.top().holds<LandViewMode::view_mode>() )
          break;
        // This menu item is only expected to be clicked when the
        // white box tile is visible.
        CHECK( white_box_stream_.has_value() );
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::move_mode{} ) );
        };
        return handler;
      }
      case e_menu_item::zoom_in: {
        auto handler = [this] {
          // A user zoom request halts any auto zooming that may
          // currently be happening.
          viewport().stop_auto_zoom();
          viewport().stop_auto_panning();
          viewport().smooth_zoom_target( viewport().get_zoom() *
                                         zoom_in_factor );
        };
        return handler;
      }
      case e_menu_item::zoom_out: {
        auto handler = [this] {
          // A user zoom request halts any auto zooming that may
          // currently be happening.
          viewport().stop_auto_zoom();
          viewport().stop_auto_panning();
          viewport().smooth_zoom_target( viewport().get_zoom() *
                                         zoom_out_factor );
        };
        return handler;
      }
      case e_menu_item::restore_zoom: {
        if( viewport().get_zoom() == 1.0 ) break;
        auto handler = [this] {
          viewport().smooth_zoom_target( 1.0 );
        };
        return handler;
      }
      case e_menu_item::find_blinking_unit: {
        if( !mode_.top().holds<LandViewMode::unit_input>() )
          break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::center{} ) );
        };
        return handler;
      }
      case e_menu_item::sentry: {
        if( !mode_.top().holds<LandViewMode::unit_input>() )
          break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::cmd{
                .what = command::sentry{} } ) );
        };
        return handler;
      }
      case e_menu_item::fortify: {
        if( !mode_.top().holds<LandViewMode::unit_input>() )
          break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::cmd{
                .what = command::fortify{} } ) );
        };
        return handler;
      }
      case e_menu_item::disband: {
        if( mode_.top().holds<LandViewMode::unit_input>() ) {
          return [this] {
            raw_input_stream_.send(
                RawInput( LandViewRawInput::cmd{
                  .what = command::disband{} } ) );
          };
        }
        if( mode_.top().holds<LandViewMode::view_mode>() ) {
          point const tile = white_box_tile( ss_ );
          if( ss_.units.from_coord( Coord::from_gfx( tile ) )
                  .empty() )
            break;
          return [tile, this] {
            raw_input_stream_.send(
                RawInput( LandViewRawInput::cmd{
                  .what = command::disband{ .tile = tile } } ) );
          };
        }
        break;
      }
      case e_menu_item::wait: {
        if( mode_.top().holds<LandViewMode::unit_input>() ) {
          return [this] {
            raw_input_stream_.send(
                RawInput( LandViewRawInput::cmd{
                  .what = command::wait{} } ) );
          };
        }
        break;
      }
      case e_menu_item::build_colony: {
        if( mode_.top().holds<LandViewMode::unit_input>() ) {
          return [this] {
            raw_input_stream_.send(
                RawInput( LandViewRawInput::cmd{
                  .what = command::build{} } ) );
          };
        }
        break;
      }
      case e_menu_item::return_to_europe: {
        if( auto const unit_input =
                mode_.top()
                    .get_if<LandViewMode::unit_input>() ) {
          Unit const& unit =
              ss_.units.unit_for( unit_input->unit_id );
          if( !unit.desc().ship ) break;
          // NOTE: this will cause the unit to path-find and/or
          // search for the nearest sea lane launch point (which
          // could be the edge of the map). This is different
          // from what this menu item does in the OG: in the OG
          // it is only available when the unit is already at a
          // sea lane launch point and it just causes the unit to
          // move onto the adjacent tile (or off the map, as the
          // case may be) to go to the harbor. There doesn't seem
          // to be anything to gain by keeping that behavior as
          // opposed to the more generate behavior of just initi-
          // ating a sea-lane path find.
          return [this] {
            raw_input_stream_.send(
                RawInput( LandViewRawInput::goto_harbor{} ) );
          };
        }
        break;
      }
      case e_menu_item::dump: {
        if( !mode_.top().holds<LandViewMode::unit_input>() )
          break;
        // Only for things that can carry cargo (ships and wagon
        // trains).
        if( ss_.units
                .unit_for( mode_.top()
                               .get<LandViewMode::unit_input>()
                               .unit_id )
                .desc()
                .cargo_slots == 0 )
          break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::cmd{
                .what = command::dump{} } ) );
        };
        return handler;
      }
      case e_menu_item::plow: {
        if( !mode_.top().holds<LandViewMode::unit_input>() )
          break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::cmd{
                .what = command::plow{} } ) );
        };
        return handler;
      }
      case e_menu_item::road: {
        if( !mode_.top().holds<LandViewMode::unit_input>() )
          break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::cmd{
                .what = command::road{} } ) );
        };
        return handler;
      }
      case e_menu_item::hidden_terrain: {
        if( !mode_.top().holds<LandViewMode::unit_input>() &&
            !mode_.top().holds<LandViewMode::view_mode>() &&
            !mode_.top().holds<LandViewMode::end_of_turn>() )
          break;
        auto handler = [this] {
          raw_input_stream_.send(
              RawInput( LandViewRawInput::hidden_terrain{} ) );
        };
        return handler;
      }
      case e_menu_item::go_to: {
        if( mode_.top().holds<LandViewMode::unit_input>() )
          return [this] {
            raw_input_stream_.send(
                RawInput( LandViewRawInput::goto_port{} ) );
          };
        break;
      }
      case e_menu_item::no_orders: {
        if( mode_.top().holds<LandViewMode::unit_input>() )
          return [this] {
            raw_input_stream_.send(
                RawInput( LandViewRawInput::cmd{
                  .what = command::forfeight{} } ) );
          };
        break;
      }
      default:
        break;
    }
    return nothing;
  }

  bool will_handle_menu_click( e_menu_item item ) override {
    return menu_click_handler( item ).has_value();
  }

  void handle_menu_click( e_menu_item item ) override {
    auto const func = menu_click_handler( item );
    CHECK( func.has_value() );
    ( *func )();
  }

  e_input_handled input( input::event_t const& event ) override {
    auto handled = e_input_handled::no;
    switch( event.to_enum() ) {
      case input::e_input_event::unknown_event: //
        break;
      case input::e_input_event::quit_event: //
        break;
      case input::e_input_event::win_event: //
        break;
      case input::e_input_event::key_event: {
        auto& key_event = event.get<input::key_event_t>();
        if( key_event.change != input::e_key_change::down )
          break;
        handled          = e_input_handled::yes;
        auto is_move_key = [&] {
          return key_event.direction.has_value();
        };

        // NOTE: we need to keep collecting inputs even when the
        // land view state is "none" so that e.g. the user can
        // buffer motion commands for a unit while it is sliding.

        if( mode_.top().holds<LandViewMode::hidden_terrain>() ) {
          // Here we want to basically allow the user to navigate
          // around with the white box, but any other key should
          // exit hidden terrain mode, regardless of what it
          // would otherwise normally do.
          if( !input::is_mod_key( key_event ) &&
              !is_move_key() && key_event.keycode != ::SDLK_c ) {
            // This will tell hidden-terrain mode to exit.
            raw_input_stream_.send(
                RawInput( LandViewRawInput::hidden_terrain{} ) );
            break;
          }
        }
        // First allow the Lua hook to handle the key press if it
        // wants.
        maybe<command> lua_orders = try_orders_from_lua(
            key_event.keycode, key_event.mod.ctrl_down,
            key_event.mod.shf_down );
        if( lua_orders ) {
          // lg.debug( "received key from lua: {}", lua_orders );
          raw_input_stream_.send( RawInput(
              LandViewRawInput::cmd{ .what = *lua_orders } ) );
          break;
        }
        switch( key_event.keycode ) {
          case ::SDLK_x: {
            // FIXME: these are only needed because of the fact
            // that the viewport doesn't really work properly in
            // that sometimes it hangs onto panning targets. We
            // will be able to remove this after we migrate fully
            // to the Camera.
            viewport().stop_auto_panning();
            viewport().stop_auto_zoom();

            camera_.zoom_out();
            viewport().fix_invariants();
            raw_input_stream_.send(
                RawInput( LandViewRawInput::zoom_changed{} ) );
            break;
          }
          case ::SDLK_z: {
            {
              // FIXME: these are only needed because of the fact
              // that the viewport doesn't really work properly
              // in that sometimes it hangs onto panning targets.
              // We will be able to remove this after we migrate
              // fully to the Camera.
              viewport().stop_auto_panning();
              viewport().stop_auto_zoom();

              auto const tile = find_tile_to_center_on();
              // The idea here is that if we are zoomed out and
              // we can see the special tile then it is ok to
              // center on it as we zoom, since that is what the
              // player probably wants. But if the tile is not
              // visible then hitting zoom would pan to a totally
              // different part of the map, which would look
              // strange. Need to do this before zooming in oth-
              // erwise we may lose sight of the tile.
              bool const tile_visible =
                  tile->is_inside( viewport_.covered_tiles() );
              ZoomChanged const changed = camera_.zoom_in();
              viewport().fix_invariants();
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::zoom_changed{} ) );
              if( tile_visible && changed.bucket_changed ) {
                CHECK( tile.has_value() );
                camera_.center_on_tile( *tile );
                viewport().fix_invariants();
              }
            }
            break;
          }
          case ::SDLK_F1:
            if( key_event.mod.shf_down ) {
              if( !cheat_mode_enabled( ss_ ) ) break;
              // Cheat mode.
              if( !mode_.top()
                       .holds<LandViewMode::unit_input>() &&
                  !mode_.top()
                       .holds<LandViewMode::end_of_turn>() &&
                  !mode_.top().holds<LandViewMode::view_mode>() )
                break;
              raw_input_stream_.send( RawInput(
                  LandViewRawInput::cheat_create_unit{} ) );
            }
            break;
          case ::SDLK_g:
            if( !mode_.top().holds<LandViewMode::unit_input>() )
              break;
            if( key_event.mod.shf_down )
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::goto_tile{} ) );
            else
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::goto_port{} ) );
            break;
          case ::SDLK_v:
            if( key_event.mod.shf_down ) break;
            if( !mode_.top().holds<LandViewMode::unit_input>() )
              break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::view_mode{} ) );
            break;
          case ::SDLK_w:
            if( key_event.mod.shf_down ) break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::cmd{
                  .what = command::wait{} } ) );
            break;
          case ::SDLK_s:
            if( key_event.mod.shf_down ) break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::cmd{
                  .what = command::sentry{} } ) );
            break;
          case ::SDLK_a:
            if( key_event.mod.shf_down ) break;
            if( !mode_.top()
                     .holds<LandViewMode::end_of_turn>() &&
                !mode_.top().holds<LandViewMode::view_mode>() )
              break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::activate{
                  .tile = white_box_tile( ss_ ) } ) );
            break;
          case ::SDLK_m:
            if( key_event.mod.shf_down ) break;
            if( !mode_.top()
                     .holds<LandViewMode::end_of_turn>() &&
                !mode_.top().holds<LandViewMode::view_mode>() )
              break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::move_mode{} ) );
            break;
          case ::SDLK_f:
            if( key_event.mod.shf_down ) break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::cmd{
                  .what = command::fortify{} } ) );
            break;
          case ::SDLK_o:
            // Capital O.
            if( !key_event.mod.shf_down ) break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::cmd{
                  .what = command::dump{} } ) );
            break;
          case ::SDLK_b:
            if( key_event.mod.shf_down ) break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::cmd{
                  .what = command::build{} } ) );
            break;
          case ::SDLK_c:
            if( key_event.mod.shf_down ) break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::center{} ) );
            break;
          case ::SDLK_d:
            if( !key_event.mod.shf_down ) {
              // No shift.
              if( mode_.top().holds<LandViewMode::unit_input>() )
                raw_input_stream_.send(
                    RawInput( LandViewRawInput::cmd{
                      .what = command::disband{} } ) );
            } else {
              // shift key down.
              if( mode_.top().holds<LandViewMode::view_mode>() ||
                  mode_.top()
                      .holds<LandViewMode::end_of_turn>() )
                raw_input_stream_.send(
                    RawInput( LandViewRawInput::cmd{
                      .what = command::disband{
                        .tile = white_box_tile( ss_ ) } } ) );
            }
            break;
          case ::SDLK_h:
            if( !key_event.mod.shf_down ) break;
            if( mode_.top()
                    .holds<LandViewMode::hidden_terrain>() )
              // This can happen when pressing shift-h multiple
              // times before the animator gets to the point
              // where it starts taking keys.
              break;
            raw_input_stream_.send(
                RawInput( LandViewRawInput::hidden_terrain{} ) );
            break;
          case ::SDLK_r:
            // Cheat function -- reveal entire map.
            if( !key_event.mod.shf_down ) break;
            raw_input_stream_.send( RawInput(
                LandViewRawInput::toggle_map_reveal{} ) );
            break;
          case ::SDLK_e:
            raw_input_stream_.send( RawInput(
                LandViewRawInput::european_status{} ) );
            break;
          case ::SDLK_ESCAPE:
            raw_input_stream_.send(
                RawInput( LandViewRawInput::escape{} ) );
            break;
          case ::SDLK_KP_ENTER:
          case ::SDLK_RETURN:
            if( mode_.top().holds<LandViewMode::goto_mode>() ) {
              // The goto mode state keeps track of the desired
              // tile that is selected.
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::tile_enter{} ) );
              break;
            }
            if( mode_.top().holds<LandViewMode::view_mode>() ||
                mode_.top()
                    .holds<LandViewMode::end_of_turn>() ) {
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::tile_enter{
                    .tile = white_box_tile( ss_ ),
                    .mods = key_event.mod } ) );
              break;
            }
            break;
          case ::SDLK_SPACE:
          case ::SDLK_KP_5:
            if( mode_.top().holds<LandViewMode::unit_input>() ) {
              if( key_event.mod.shf_down ) break;
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::cmd{
                    .what = command::forfeight{} } ) );
            } else if( mode_.top()
                           .holds<
                               LandViewMode::end_of_turn>() ) {
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::next_turn{} ) );
            } else if( mode_.top()
                           .holds<LandViewMode::goto_mode>() ) {
              // This will be interpreted as "accept tile" and
              // will start the goto.
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::cmd{
                    .what = command::forfeight{} } ) );
            } else if( mode_.top()
                           .holds<LandViewMode::view_mode>() ) {
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::move_mode{} ) );
            }
            break;
          default:
            if( key_event.mod.shf_down ) break;
            handled = e_input_handled::no;
            if( key_event.direction ) {
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::cmd{
                    .what = command::move{
                      *key_event.direction } } ) );
              handled = e_input_handled::yes;
            }
            break;
        }
        break;
      }
      case input::e_input_event::mouse_move_event: {
        auto& val = event.get<input::mouse_move_event_t>();
        auto const tile =
            viewport().screen_pixel_to_world_tile( val.pos );
        auto const hypo_tile =
            viewport().screen_pixel_to_hypothetical_world_tile(
                val.pos );
        raw_input_stream_.send(
            RawInput( LandViewRawInput::mouse_over{
              .where             = val.pos,
              .tile              = tile,
              .hypothetical_tile = hypo_tile } ) );
        handled = e_input_handled::yes;
        break;
      }
      case input::e_input_event::mouse_wheel_event: {
        auto& val = event.get<input::mouse_wheel_event_t>();
        // If the mouse is in the viewport and its a wheel event
        // then we are in business.
        if( val.pos.is_inside( landview_renderable_rect() ) ) {
          if( val.wheel_delta < 0 ) {
            viewport().set_zoom_push( e_push_direction::negative,
                                      nothing );
            raw_input_stream_.send(
                RawInput( LandViewRawInput::zoom_changed{} ) );
          }
          if( val.wheel_delta > 0 ) {
            viewport().set_zoom_push( e_push_direction::positive,
                                      val.pos );
            raw_input_stream_.send(
                RawInput( LandViewRawInput::zoom_changed{} ) );
          }
          // A user zoom request halts any auto zooming that may
          // currently be happening.
          viewport().stop_auto_zoom();
          viewport().stop_auto_panning();
          handled = e_input_handled::yes;
        }
        break;
      }
      case input::e_input_event::mouse_button_event: {
        auto& val = event.get<input::mouse_button_event_t>();
        point const hypo_tile =
            viewport().screen_pixel_to_hypothetical_world_tile(
                val.pos );
        if( !hypo_tile.is_inside(
                viewport().world_rect_tiles() ) ) {
          raw_input_stream_.send( RawInput(
              LandViewRawInput::mouse_click_outside_of_map{
                .buttons           = val.buttons,
                .hypothetical_tile = hypo_tile } ) );
          handled = e_input_handled::yes;
          break;
        }
        UNWRAP_BREAK(
            tile,
            viewport().screen_pixel_to_world_tile( val.pos ) );
        handled = e_input_handled::yes;
        lg.debug( "clicked on tile: {}.", tile );
        // Need to only handle "up" events here because if we
        // handled "down" events then that would interfere with
        // dragging.
        switch( val.buttons ) {
          // FIXME: Try to find a way to make this left_down be-
          // cause it is more responsive. When it was tried it
          // messed some things up, for example when clicking on
          // a unit that already moved a message box would pop
          // up, then the release of the mouse button (left_up)
          // would close the window. Not sure if it will be fea-
          // sible to do this, but should try.
          case input::e_mouse_button_event::left_up: {
            raw_input_stream_.send(
                RawInput( LandViewRawInput::tile_click{
                  .coord = tile, .mods = val.mod } ) );
            break;
          }
          case input::e_mouse_button_event::right_up: {
            if( val.mod.shf_down ) {
              raw_input_stream_.send(
                  RawInput( LandViewRawInput::context_menu{
                    .where = val.pos, .tile = tile } ) );
              break;
            }
            raw_input_stream_.send(
                RawInput( LandViewRawInput::tile_right_click{
                  .coord = tile, .mods = val.mod } ) );
            break;
          }
          default:
            break;
        }
        break;
      }
      default: //
        break;
    }
    return handled;
  }

  /**************************************************************
  ** Viewport Dragging
  ***************************************************************/
  wait<> viewport_dragging( point const /*origin*/ ) {
    viewport().stop_auto_panning();
    while( auto const d = co_await drag_stream.next() )
      viewport().pan_by_screen_coords( d->prev - d->current );
  }

  /**************************************************************
  ** Goto Dragging
  ***************************************************************/
  [[nodiscard]] bool is_goto_drag( point const mouse ) {
    auto const& unit_input =
        mode_.top().get_if<LandViewMode::unit_input>();
    if( !unit_input.has_value() ) return false;
    auto const mouse_tile =
        viewport().screen_pixel_to_world_tile( mouse );
    if( !mouse_tile.has_value() ) return false;
    UNWRAP_CHECK_T( auto const unit_tile,
                    coord_for_unit_indirect(
                        ss_.units, unit_input->unit_id ) );
    return *mouse_tile == unit_tile;
  }

  wait<> goto_dragging( point const origin ) {
    using I = LandViewRawInput;

    auto const send = [&]( auto const& o ) {
      raw_input_stream_.send( I{ o } );
    };

    // This guarantees that we always send at least one message
    // to other dragging coro that we're finished, otherwise that
    // other one could get stuck waiting, which would be cata-
    // strophic because it is in the main coro. This will also
    // get sent when the drag finishes naturally, but there
    // should be no harm in that.
    SCOPE_EXIT { send( I::goto_drag_cancel{} ); };

    UNWRAP_CHECK(
        tile, viewport().screen_pixel_to_world_tile( origin ) );
    send( I::goto_drag_start{ .tile = tile } );

    while( auto const d = co_await drag_stream.next() ) {
      // This will allow the mouse to be on the map or in the
      // area around it (if it is zoomed out enough) but not e.g.
      // on the panel or menu. This is because we want the user
      // to be able to drag off the map to signal going to the
      // harbor, but only if the map surroundings are visible,
      // i.e. we don't want them doing that when the mouse is
      // over the panel.
      if( !d->current.is_inside( landview_renderable_rect() ) )
        continue;
      // Allow dragging off the edge of the map.
      point const hypo_tile =
          viewport().screen_pixel_to_hypothetical_world_tile(
              d->current );
      // Add one column of edge to left and right sides to allow
      // the player to signal that they want to sail the high
      // seas.
      rect const allowed = viewport()
                               .world_rect_tiles()
                               .to_gfx()
                               .moved_right()
                               .with_new_left_edge( -1 )
                               .with_dec_size();
      point const tile = hypo_tile.clamped( allowed );
      send( I::goto_drag_update{ .tile = tile } );
    }

    send( I::goto_drag_finish{} );
  }

  /**************************************************************
  ** Dragging Driver
  ***************************************************************/
  struct DragUpdate {
    Coord prev;
    Coord current;
  };
  // Here, `nothing` is used to indicate that it has ended
  // without having been cancelled.
  co::finite_stream<DragUpdate> drag_stream;
  // The waitable will be waiting on the drag_stream, so it must
  // come after so that it gets destroyed first.
  maybe<wait<>> drag_thread;

  using DragHandler =
      wait<> ( Impl::* const )( point const origin );

  wait<> drag_thread_wrapper(
      DragHandler const handler,
      input::e_mouse_button const /*button*/,
      point const origin ) {
    drag_stream.reset();
    SCOPE_EXIT { drag_stream.reset(); };
    co_await ( this->*handler )( origin );
    // Wait for drag to finish if it hasn't already.
    while( co_await drag_stream.next() );
  }

  IPlane::e_accept_drag can_drag( input::e_mouse_button button,
                                  Coord origin ) override {
    drag_thread.reset();
    if( button == input::e_mouse_button::r &&
        viewport().screen_coord_in_viewport( origin ) ) {
      drag_thread = drag_thread_wrapper(
          &Impl::viewport_dragging, button, origin );
      return IPlane::e_accept_drag::yes;
    }
    if( button == input::e_mouse_button::l &&
        viewport().screen_coord_in_viewport( origin ) ) {
      if( is_goto_drag( origin ) ) {
        drag_thread = drag_thread_wrapper( &Impl::goto_dragging,
                                           button, origin );
        return IPlane::e_accept_drag::yes;
      }
    }
    // Since this is a bottom plane, there is no one else who
    // could handle drag events, so just give them to us as mo-
    // tion events so that we can still generally respond to the
    // mouse movements.
    return IPlane::e_accept_drag::motion;
  }

  void on_drag( input::mod_keys const& /*unused*/,
                input::e_mouse_button /*unused*/,
                Coord /*unused*/, Coord prev,
                Coord current ) override {
    drag_stream.send(
        DragUpdate{ .prev = prev, .current = current } );
  }

  void on_drag_finished( input::mod_keys const& /*mod*/,
                         input::e_mouse_button /*button*/,
                         Coord /*origin*/,
                         Coord /*end*/ ) override {
    drag_stream.finish();
    // At this point we assume that the callback will finish on
    // its own after doing any post-drag stuff it needs to do.
  }

  void reset_input_buffers() {
    // lg.debug( "clearing land-view input buffers." );
    raw_input_stream_.reset();
    translated_input_stream_ = {};
  }

  void start_new_turn() {
    // An example of why this is needed is because when a unit is
    // moving (say, it is the only active unit) and the screen
    // scrolls away from it to show a colony update, then when
    // that update message closes and the next turn starts and
    // focuses again on the unit, it would not scroll back to
    // that unit.
    g_needs_scroll_to_unit_on_input = true;
  }

  wait<> center_on_tile( point const tile ) {
    co_await viewport().center_on_tile_smooth(
        Coord::from_gfx( tile ) );
  }

  void set_visibility( maybe<e_player> player ) {
    viz_ = create_visibility_for( ss_, player );
  }

  void zoom_out_full() {
    viewport().set_zoom( viewport().optimal_min_zoom() );
  }

  maybe<UnitId> unit_blinking() const {
    return mode_.top().get_if<LandViewMode::unit_input>().member(
        &LandViewMode::unit_input::unit_id );
  }

  maybe<point> white_box() const {
    return white_box_tile_if_visible();
  }

  wait<> eat_cross_unit_buffered_input_events( UnitId id ) {
    if( !config_land_view.input_overrun_detection.enabled )
      co_return;
    // It is necessary to clear the input buffers here because
    // otherwise we could potentially read an old buffered even
    // that was entered longer than kWait ago, which will cause
    // the below to exit the loop prematurely and not eat any-
    // thing.
    reset_input_buffers();
    auto const kWait =
        config_land_view.input_overrun_detection.wait_time;
    SCOPE_EXIT { input_overrun_indicator_ = nothing; };
    int const kMaxInputsToWithold =
        config_land_view.input_overrun_detection
            .max_inputs_to_withold;
    for( int i = 0; i < kMaxInputsToWithold; ++i ) {
      auto const res = co_await co::first(
          raw_input_stream_.next(), wait_for_duration( kWait ) );
      if( res.index() == 1 ) // timeout
        break;
      UNWRAP_CHECK( raw_input, res.get_if<RawInput>() );
      auto command =
          raw_input.input.get_if<LandViewRawInput::cmd>();
      if( !command.has_value() ) {
        raw_input_stream_.reset();
        raw_input_stream_.send( raw_input );
        co_return;
      }
      if( Clock_t::now() - raw_input.when >= kWait ) break;
      // The player may still be trying to move the old unit, so
      // eat this command and display an indicator to signal that
      // inputs are getting eaten.
      input_overrun_indicator_ = InputOverrunIndicator{
        .unit_id = id, .start_time = raw_input.when };
    }
    reset_input_buffers();
  }

  bool is_unit_visible_on_map( GenericUnitId id ) const {
    if( animator_.unit_animations().contains( id ) ) return true;
    maybe<Coord> const tile =
        coord_for_unit_multi_ownership( ss_, id );
    if( !tile.has_value() ) return false;
    if( ss_.colonies.maybe_from_coord( *tile ).has_value() )
      // Note that if the unit is in a colony square and is de-
      // fending an attack (and hence visible) then the anima-
      // tions check above should have caught it.
      return false;
    if( ss_.units.unit_kind( id ) == e_unit_kind::euro ) {
      if( is_unit_onboard( ss_.units,
                           ss_.units.check_euro_unit( id ) ) )
        // Note that if the unit is onboard but is asking for or-
        // ders then the animations check above should have
        // caught it.
        return false;
    }
    vector<GenericUnitId> const sorted =
        land_view_unit_stack( ss_, *tile, last_unit_input_id() );
    CHECK( !sorted.empty() );
    return ( sorted[0] == id );
  }

  // Handles interaction while the animation is happening. Con-
  // sume further inputs but eat all of them except for the ones
  // we want, returning only when we should interrupt.
  wait<> hidden_terrain_interact_during_animation() {
    CHECK( mode_.top().holds<LandViewMode::hidden_terrain>() );
    for( ;; ) {
      RawInput const raw = co_await raw_input_stream_.next();
      SWITCH( raw.input ) {
        CASE( hidden_terrain ) { co_return; }
        CASE( tile_click ) {
          co_await click_on_world_tile( tile_click.coord );
          break;
        }
        default:
          break;
      }
    }
  };

  // Handles interaction while waiting in hidden_terrain mode.
  wait<LandViewPlayerInput> hidden_terrain_white_box_loop(
      point const initial_tile ) {
    wait<> const _ = white_box_thread( initial_tile );
    while( true ) {
      auto const input = co_await white_box_input();
      if( input->get_if<LandViewPlayerInput::hidden_terrain>() )
        co_return *input;
    }
  }

  wait<> show_hidden_terrain() {
    // Clear input buffers after changing state to the new state
    // and after switching back to the old state.
    SCOPE_EXIT { reset_input_buffers(); };
    SCOPED_MODE_PUSH_AND_GET( _, hidden_terrain );
    reset_input_buffers();
    // TODO: maybe we could find some way of animating the re-
    // moval of the fog instead of just having it disappear.
    auto const defogger =
        ts_.map_updater().push_options_and_redraw(
            []( MapUpdaterOptions& options ) {
              options.render_fog_of_war = false;
            } );

    HiddenTerrainAnimationSequence const seq =
        anim_seq_for_hidden_terrain( ss_, *viz_, ts_.rand );

    co_await co::first(
        animator_.animate_sequence( seq.hide ),
        hidden_terrain_interact_during_animation() );

    auto const tile = find_a_good_white_box_location(
        ss_, viewport_.covered_tiles() );
    co_await co::first(
        animator_.animate_sequence(
            seq.hold, AnimSeqOptions{ .hold = true } ),
        hidden_terrain_white_box_loop( tile ) );

    co_await co::first(
        animator_.animate_sequence( seq.show ),
        hidden_terrain_interact_during_animation() );
  }

  wait<> white_box_thread( point const initial ) {
    CHECK( !white_box_stream_.has_value() );
    auto& stream = white_box_stream_.emplace();
    SCOPE_EXIT { white_box_stream_.reset(); };
    set_white_box_tile( ss_, initial );
    while( true ) {
      point const tile     = white_box_tile( ss_ );
      wait<> const blinker = animator_.animate_white_box();
      wait<> const panner  = animator_.ensure_visible( tile );
      set_white_box_tile( ss_, co_await stream.next() );
    }
  }

  wait<maybe<LandViewPlayerInput>> white_box_input() {
    LandViewPlayerInput const input =
        co_await next_player_input_object();

    auto const direction =
        input.inner_if<LandViewPlayerInput::give_command>()
            .inner_if<command::move>();

    if( !direction.has_value() ) co_return input;

    white_box_stream_->send(
        white_box_tile( ss_ )
            .moved( *direction )
            .clamped(
                viz_->rect_tiles().to_gfx().with_dec_size() ) );

    co_return nothing;
  }

  wait<LandViewPlayerInput> white_box_input_loop(
      point const initial_tile ) {
    wait<> const _ = white_box_thread( initial_tile );
    // This loop typically returns on the first iteration in
    // order to deliver the command to the caller, but it may
    // loop if the player is just doing things that can be han-
    // dled in this module, such as moving the white box tile or
    // doing things that only affect the land view display.
    while( true ) {
      auto const input = co_await white_box_input();
      if( input.has_value() ) co_return *input;
    }
  }

  wait<LandViewPlayerInput> show_view_mode(
      ViewModeOptions const options ) {
    // Clear input buffers after changing state to the new state
    // and after switching back to the old state.
    SCOPE_EXIT { reset_input_buffers(); };
    SCOPED_MODE_PUSH_AND_GET( _, view_mode );
    reset_input_buffers();
    point const initial_tile =
        options.initial_tile.has_value()
            ? *options.initial_tile
            : find_a_good_white_box_location(
                  ss_, viewport_.covered_tiles() );
    // Now that we've extracted potential info from this, reset
    // it since the fact that we're changing modes means that we
    // don't want the continuity behavior when we exit this mode
    // and return to asking order for the unit which would other-
    // wise be enabled by letting this remain set.
    last_unit_input_ = nothing;
    co_return co_await white_box_input_loop( initial_tile );
  }

  wait<LandViewPlayerInput> get_next_input( UnitId id ) {
    // If the last unit that asked for orders no longer exists
    // then clear all state associated with it.
    if( !last_unit_input_id() ) last_unit_input_ = nothing;

    // We only pan to the unit here because if we did that out-
    // side of this if statement then the viewport would pan to
    // the blinking unit after the player e.g. clicks on another
    // unit to activate it.
    if( last_unit_input_id() != id )
      g_needs_scroll_to_unit_on_input = true;

    // Input buffers. If there was no previous unit asking for
    // orders (or if that unit no longer exists) then definitely
    // clear the buffers.
    if( !last_unit_input_.has_value() ) reset_input_buffers();

    // The idea of this is that if we're starting on a new unit
    // or if we're still on the same unit but a window popped up
    // since it last asked for orders (which could have happened
    // as a result of its move or other processing during the
    // turn) then we want to clear the input buffers. Otherwise,
    // it could create a strange situation where the player ob-
    // serves buffered input events processed after a window
    // closes.
    if( last_unit_input_.has_value() &&
        ts_.gui.total_windows_created() >
            last_unit_input_->window_count ) {
      // This means that a window popped up since the last time
      // that a unit was asking for orders. In that case we prob-
      // ably don't need the input buffer shield because the pre-
      // vious unit's movement has been interrupted by a window.
      // And for the same reason we should clear buffered input
      // events.
      last_unit_input_->window_count =
          ts_.gui.total_windows_created();
      last_unit_input_->need_input_buffer_shield = false;
      reset_input_buffers();
    }

    // This might be true either because we started a new turn,
    // or because of the above assignment.
    if( g_needs_scroll_to_unit_on_input )
      co_await animator_.ensure_visible_unit( id );
    g_needs_scroll_to_unit_on_input = false;

    // Need to set this before taking any user input, otherwise
    // we will be in the "none" state, which represents the
    // end-of-turn, in which case e.g. a space bar gets sent as a
    // "next turn" event as opposed to a "forfeight" event (which
    // can happen because sometimes we take input while eating
    // buffered input events below).
    SCOPED_MODE_PUSH_AND_GET( unit_input, unit_input );
    unit_input.unit_id = id;

    // When we start on a new unit clear the input queue so that
    // commands that were accidentally buffered while controlling
    // the previous unit don't affect this new one, which would
    // almost certainly not be desirable. This function does that
    // clearing in an interactive way in order to signal to the
    // user what is happening.
    bool const eat_buffered =
        last_unit_input_id().has_value() &&
        // If still moving same unit then no need to shield.
        *last_unit_input_id() != id &&
        last_unit_input_->need_input_buffer_shield &&
        // E.g. if the previous unit got on a ship then the
        // player doesn't expect them to continue moving, so no
        // need to shield this unit.
        is_unit_on_map( ss_.units, *last_unit_input_id() );
    if( eat_buffered ) {
      // We typically wait for a bit while eating input events;
      // during that time, make sure that the unit who is about
      // to ask for orders is rendered on the front.
      AnimationSequence const seq = anim_seq_unit_to_front( id );
      AnimSeqOptions const opts{ .hold = true };
      // We need the "hold" version because we need this to keep
      // going until the buffer eating is complete, at which
      // point it will be cancelled.
      wait<> anim = animator_.animate_sequence( seq, opts );
      co_await eat_cross_unit_buffered_input_events( id );
    }

    // Most of the time we start the blinking with the unit in-
    // visible, that way the start of the animation creates an
    // immediate visual change that draws the player's eye to the
    // unit. However, in some cases, the unit is not initially
    // visible even before the animation starts; this could be
    // e.g. because it is on a colony square, it is the cargo of
    // a ship, or it is in the middle of a stack of units. In
    // those cases, we want the blinking animation to be ini-
    // tially visible so that it creates a similar visual change
    // to draw the player's eye. Note that if we did eat buffered
    // input events above then the unit will have been made vis-
    // ible for a brief time regardless, so in that case we know
    // we that we should start with the unit invisible. Note that
    // this needs to be computed before we change the last unit
    // input, because the last unit input is involved in deter-
    // mining whether or not the unit is currently visible (that
    // is because the algorithm that orders unit stacks on a tile
    // will treat the last unit input specially, putting it on
    // top for various UX reasons).
    bool const visible_initially =
        !is_unit_visible_on_map( id ) && !eat_buffered;
    lg.trace( "visible={}, eat={}, init={}",
              is_unit_visible_on_map( id ), eat_buffered,
              visible_initially );

    last_unit_input_.emplace( ss_, /*unit_id=*/id );
    last_unit_input_->need_input_buffer_shield = true;
    last_unit_input_->window_count =
        ts_.gui.total_windows_created();

    // The reason we are setting it here is so that if the player
    // enters view mode while the unit is blinking it will select
    // that square when it runs though the logic to find a suit-
    // able square for the white box (assuming that the unit is
    // visible). NOTE: We use this method directly to set the
    // white box position (which is not normally how we set it
    // when it is visible; to do that we feed the position into
    // the stream so that the coro can update the animation
    // state) because we are setting it when it is not visible.
    set_white_box_tile(
        ss_, coord_for_unit_indirect_or_die( ss_.units, id ) );

    // Run the blinker while waiting for user input. The question
    // is, do we want the blinking to start "on" or "off"? The
    // idea is that we want to start it off in the opposite vi-
    // sual state that the unit currently is in.
    LandViewPlayerInput input = co_await co::background(
        next_player_input_object(),
        animator_.animate_blink( id, visible_initially ) );

    // Disable input buffer shielding when the unit issues a
    // non-move command, because in that case it is unlikely that
    // the player will assume that the subsequent command they
    // issue would go to the same unit. E.g., if the unit issues
    // a command to fortify this unit or to open a colony, then
    // they know that the next command they issue would go to the
    // new unit, so we don't need to shield it.
    maybe<command::move const&> move_cmd =
        input.get_if<LandViewPlayerInput::give_command>()
            .member( &LandViewPlayerInput::give_command::cmd )
            .get_if<command::move>();
    if( last_unit_input_.has_value() && !move_cmd.has_value() )
      last_unit_input_->need_input_buffer_shield = false;

    co_return input;
  }

  wait<LandViewPlayerInput> eot_get_next_input() {
    last_unit_input_ = nothing;
    SCOPED_MODE_PUSH_AND_GET( _, end_of_turn );
    point const initial_tile = find_a_good_white_box_location(
        ss_, viewport_.covered_tiles() );
    co_return co_await white_box_input_loop( initial_tile );
  }

  void on_logical_resolution_selected(
      gfx::e_resolution ) override {
    viewport_.update_logical_rect_cache(
        landview_renderable_rect() );
  }
};

/****************************************************************
** LandViewPlane
*****************************************************************/
IPlane& LandViewPlane::impl() { return *impl_; }

LandViewPlane::~LandViewPlane() = default;

LandViewPlane::LandViewPlane( IEngine& engine, SS& ss, TS& ts,
                              maybe<e_player> visibility )
  : impl_( new Impl( engine, ss, ts, visibility ) ) {}

wait<> LandViewPlane::ensure_visible( Coord const& coord ) {
  return impl_->animator_.ensure_visible( coord.to_gfx() );
}

wait<> LandViewPlane::center_on_tile( point const tile ) {
  return impl_->center_on_tile( tile );
}

void LandViewPlane::set_visibility( maybe<e_player> player ) {
  return impl_->set_visibility( player );
}

wait<> LandViewPlane::ensure_visible_unit( GenericUnitId id ) {
  return impl_->animator_.ensure_visible_unit( id );
}

wait<> LandViewPlane::show_hidden_terrain() {
  return impl_->show_hidden_terrain();
}

wait<LandViewPlayerInput> LandViewPlane::show_view_mode(
    ViewModeOptions const options ) {
  return impl_->show_view_mode( options );
}

wait<LandViewPlayerInput> LandViewPlane::get_next_input(
    UnitId id ) {
  return impl_->get_next_input( id );
}

wait<LandViewPlayerInput> LandViewPlane::eot_get_next_input() {
  return impl_->eot_get_next_input();
}

void LandViewPlane::start_new_turn() {
  return impl_->start_new_turn();
}

void LandViewPlane::zoom_out_full() {
  return impl_->zoom_out_full();
}

maybe<UnitId> LandViewPlane::unit_blinking() const {
  return impl_->unit_blinking();
}

maybe<point> LandViewPlane::white_box() const {
  return impl_->white_box();
}

wait<> LandViewPlane::animate_always(
    AnimationSequence const& seq ) {
  co_await impl_->animator_.animate_sequence(
      seq, AnimSeqOptions{ .check_visibility = false,
                           .hold             = false } );
}

wait<> LandViewPlane::animate_always_and_hold(
    AnimationSequence const& seq ) {
  co_await impl_->animator_.animate_sequence(
      seq, AnimSeqOptions{ .check_visibility = false,
                           .hold             = true } );
}

wait<> LandViewPlane::animate_if_visible(
    AnimationSequence const& seq ) {
  // Visibility should be checked by default within the
  // animate_sequence method.
  co_await impl_->animator_.animate_sequence(
      seq, { .check_visibility = true, .hold = false } );
}

wait<> LandViewPlane::animate_if_visible_and_hold(
    AnimationSequence const& seq ) {
  // Visibility should be checked by default within the
  // animate_sequence method.
  co_await impl_->animator_.animate_sequence(
      seq, { .check_visibility = true, .hold = true } );
}

ViewportController& LandViewPlane::viewport() const {
  return impl_->viewport_;
}

} // namespace rn
