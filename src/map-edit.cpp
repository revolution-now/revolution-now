/****************************************************************
**map-edit.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-04-03.
*
* Description: Map Editor.
*
*****************************************************************/
#include "map-edit.hpp"

// Revolution Now
#include "co-combinator.hpp"
#include "compositor.hpp"
#include "coord.hpp"
#include "game-state.hpp"
#include "gs-land-view.hpp"
#include "gs-terrain.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "map-square.hpp"
#include "map-updater.hpp"
#include "plane-ctrl.hpp"
#include "plane.hpp"
#include "plow.hpp"
#include "renderer.hpp" // FIXME: remove
#include "road.hpp"
#include "tiles.hpp"
#include "viewport.hpp"

// refl
#include "refl/enum-map.hpp"
#include "refl/to-str.hpp"

// base
#include "base/scope-exit.hpp"

using namespace std;

namespace rn {

namespace {

/****************************************************************
** Globals
*****************************************************************/
co::stream<input::event_t>    g_input;
maybe<editor::e_toolbar_item> g_selected_tool;

void reset_globals() {
  g_input         = {};
  g_selected_tool = {};
}

SmoothViewport& viewport() {
  return GameState::land_view().viewport;
}

/****************************************************************
** Toolbar
*****************************************************************/
struct ToolbarItem {
  e_tile                 tile = {};
  editor::e_toolbar_item item = {};
};

refl::enum_map<editor::e_toolbar_item, ToolbarItem>
    g_toolbar_items{
        { editor::e_toolbar_item::ocean,
          { .tile = e_tile::terrain_ocean } },
        { editor::e_toolbar_item::sea_lane,
          { .tile = e_tile::terrain_ocean_sea_lane } },
        { editor::e_toolbar_item::ground_arctic,
          { .tile = e_tile::terrain_arctic } },
        { editor::e_toolbar_item::ground_desert,
          { .tile = e_tile::terrain_desert } },
        { editor::e_toolbar_item::ground_grassland,
          { .tile = e_tile::terrain_grassland } },
        { editor::e_toolbar_item::ground_marsh,
          { .tile = e_tile::terrain_marsh } },
        { editor::e_toolbar_item::ground_plains,
          { .tile = e_tile::terrain_plains } },
        { editor::e_toolbar_item::ground_prairie,
          { .tile = e_tile::terrain_prairie } },
        { editor::e_toolbar_item::ground_savannah,
          { .tile = e_tile::terrain_savannah } },
        { editor::e_toolbar_item::ground_swamp,
          { .tile = e_tile::terrain_swamp } },
        { editor::e_toolbar_item::ground_tundra,
          { .tile = e_tile::terrain_tundra } },
        { editor::e_toolbar_item::mountain,
          { .tile = e_tile::terrain_mountain_island } },
        { editor::e_toolbar_item::hills,
          { .tile = e_tile::terrain_hills_island } },
        { editor::e_toolbar_item::forest,
          { .tile = e_tile::terrain_forest_island } },
        { editor::e_toolbar_item::irrigation,
          { .tile = e_tile::irrigation } },
        { editor::e_toolbar_item::road,
          { .tile = e_tile::road_island } },
    };

Rect toolbar_rect() {
  UNWRAP_CHECK( rect, compositor::section(
                          compositor::e_section::viewport ) );
  rect.h = g_tile_delta.h;
  return rect;
}

wait<> click_on_toolbar( Coord tile ) {
  UNWRAP_CHECK( item,
                refl::enum_from_integral<editor::e_toolbar_item>(
                    tile.x._ ) );
  g_selected_tool = item;
  co_return;
}

/****************************************************************
** Land Canvas
*****************************************************************/
enum class e_action { add, remove };

wait<> click_on_tile( IMapUpdater& map_updater, Coord tile,
                      e_action action ) {
  if( !g_selected_tool.has_value() ) co_return;
  TerrainState& terrain_state = GameState::terrain();
  MapSquare     new_square    = terrain_state.square_at( tile );
  switch( *g_selected_tool ) {
    case editor::e_toolbar_item::ocean:
      if( action == e_action::remove ) break;
      new_square = map_square_for_terrain( e_terrain::ocean );
      break;
    case editor::e_toolbar_item::sea_lane:
      if( action == e_action::remove ) break;
      new_square = map_square_for_terrain( e_terrain::ocean );
      new_square.sea_lane = true;
      break;
    case editor::e_toolbar_item::ground_arctic:
      if( action == e_action::remove ) break;
      new_square = map_square_for_terrain( e_terrain::arctic );
      break;
    case editor::e_toolbar_item::ground_desert:
      if( action == e_action::remove ) break;
      new_square = map_square_for_terrain( e_terrain::desert );
      break;
    case editor::e_toolbar_item::ground_grassland:
      if( action == e_action::remove ) break;
      new_square =
          map_square_for_terrain( e_terrain::grassland );
      break;
    case editor::e_toolbar_item::ground_marsh:
      if( action == e_action::remove ) break;
      new_square = map_square_for_terrain( e_terrain::marsh );
      break;
    case editor::e_toolbar_item::ground_plains:
      if( action == e_action::remove ) break;
      new_square = map_square_for_terrain( e_terrain::plains );
      break;
    case editor::e_toolbar_item::ground_prairie:
      if( action == e_action::remove ) break;
      new_square = map_square_for_terrain( e_terrain::prairie );
      break;
    case editor::e_toolbar_item::ground_savannah:
      if( action == e_action::remove ) break;
      new_square = map_square_for_terrain( e_terrain::savannah );
      break;
    case editor::e_toolbar_item::ground_swamp:
      if( action == e_action::remove ) break;
      new_square = map_square_for_terrain( e_terrain::swamp );
      break;
    case editor::e_toolbar_item::ground_tundra:
      if( action == e_action::remove ) break;
      new_square = map_square_for_terrain( e_terrain::tundra );
      break;
    case editor::e_toolbar_item::mountain:
      if( action == e_action::add )
        new_square.overlay = e_land_overlay::mountains;
      else if( new_square.overlay == e_land_overlay::mountains )
        new_square.overlay = nothing;
      break;
    case editor::e_toolbar_item::hills:
      if( action == e_action::add )
        new_square.overlay = e_land_overlay::hills;
      else if( new_square.overlay == e_land_overlay::hills )
        new_square.overlay = nothing;
      break;
    case editor::e_toolbar_item::forest:
      if( action == e_action::add )
        new_square.overlay = e_land_overlay::forest;
      else if( new_square.overlay == e_land_overlay::forest )
        new_square.overlay = nothing;
      break;
    case editor::e_toolbar_item::irrigation:
      new_square.irrigation = ( action == e_action::add );
      break;
    case editor::e_toolbar_item::road:
      new_square.road = ( action == e_action::add );
      break;
  }

  map_updater.modify_map_square(
      tile,
      [&]( MapSquare& to_edit ) { to_edit = new_square; } );
}

/****************************************************************
** Input Handling
*****************************************************************/
// Returns true if the user wants to exit the colony view.
wait<bool> handle_event( IMapUpdater&,
                         input::key_event_t const& event ) {
  if( event.change != input::e_key_change::down )
    co_return false;
  switch( event.keycode ) {
    case ::SDLK_z:
      if( viewport().get_zoom() < 1.0 )
        viewport().smooth_zoom_target( 1.0 );
      else if( viewport().get_zoom() < 1.5 )
        viewport().smooth_zoom_target( 2.0 );
      else
        viewport().smooth_zoom_target( 1.0 );
      break;
    case ::SDLK_ESCAPE: //
      co_return true;
    default: //
      break;
  }
  co_return false;
}

wait<bool> handle_event(
    IMapUpdater&                       map_updater,
    input::mouse_button_event_t const& event ) {
  bool left =
      event.buttons == input::e_mouse_button_event::left_down;
  bool right =
      event.buttons == input::e_mouse_button_event::right_down;
  if( !left && !right ) co_return false;
  Coord click_pos = event.pos;
  if( auto maybe_tile =
          viewport().screen_pixel_to_world_tile( click_pos ) ) {
    lg.debug( "clicked on tile: {}.", *maybe_tile );
    e_action action = left ? e_action::add : e_action::remove;
    co_await click_on_tile( map_updater, *maybe_tile, action );
    co_return false;
  }
  if( left && click_pos.is_inside( toolbar_rect() ) ) {
    co_await click_on_toolbar(
        Coord{} + ( click_pos - toolbar_rect().upper_left() ) /
                      g_tile_scale );
    co_return false;
  }
  co_return false;
}

wait<bool> handle_event(
    IMapUpdater&, input::mouse_wheel_event_t const& event ) {
  if( viewport().screen_coord_in_viewport( event.pos ) ) {
    if( event.wheel_delta < 0 )
      viewport().set_zoom_push( e_push_direction::negative,
                                nothing );
    if( event.wheel_delta > 0 )
      viewport().set_zoom_push( e_push_direction::positive,
                                event.pos );
    // A user zoom request halts any auto zooming that
    // may currently be happening.
    viewport().stop_auto_zoom();
    viewport().stop_auto_panning();
  }
  co_return false;
}

wait<bool> handle_event( IMapUpdater&, auto const& ) {
  co_return false;
}

// Remove all input events from the queue corresponding to normal
// user input, but save the ones that we always need to process,
// such as window resize events (which are needed to maintain
// proper drawing as the window is resized).
void clear_non_essential_events() {
  vector<input::event_t> saved;
  while( g_input.ready() ) {
    input::event_t e = g_input.next().get();
    switch( e.to_enum() ) {
      case input::e_input_event::win_event:
        saved.push_back( std::move( e ) );
        break;
      default: break;
    }
  }
  CHECK( !g_input.ready() );
  // Re-insert the ones we want to save.
  for( input::event_t& e : saved )
    g_input.send( std::move( e ) );
}

wait<> run_map_editor( IMapUpdater& map_updater ) {
  while( true ) {
    input::event_t event = co_await g_input.next();
    auto [exit, suspended] =
        co_await co::detect_suspend( std::visit(
            [&]( auto const& event ) {
              return handle_event( map_updater, event );
            },
            event ) );
    if( suspended ) clear_non_essential_events();
    if( exit ) co_return;
  }
}

/****************************************************************
** Rendering
*****************************************************************/
void render_toolbar( rr::Renderer& renderer ) {
  rr::Painter painter = renderer.painter();
  painter.draw_solid_rect(
      toolbar_rect().with_new_right_edge(
          0_x + renderer.logical_screen_size().w ),
      gfx::pixel::black() );
  Coord where = toolbar_rect().upper_left();
  for( editor::e_toolbar_item item :
       refl::enum_values<editor::e_toolbar_item> ) {
    render_sprite( painter, where, g_toolbar_items[item].tile );
    if( g_selected_tool == item )
      painter.draw_empty_rect(
          Rect::from( where, g_tile_delta ),
          rr::Painter::e_border_mode::inside,
          gfx::pixel::green() );
    where.x += g_tile_delta.w;
  }
  painter.draw_horizontal_line( toolbar_rect().lower_left(),
                                renderer.logical_screen_size().w,
                                gfx::pixel::black() );
}

void render_sidebar( rr::Renderer& renderer ) {
  UNWRAP_CHECK( rect, compositor::section(
                          compositor::e_section::panel ) );
  rr::Painter painter = renderer.painter();
  painter.draw_solid_rect(
      rect,
      gfx::pixel{ .r = 0x33, .g = 0x22, .b = 0x22, .a = 0xff } );
}

/****************************************************************
** Viewport
*****************************************************************/
Rect viewport_rect() {
  UNWRAP_CHECK( rect, compositor::section(
                          compositor::e_section::viewport ) );
  Delta toolbar_size = toolbar_rect().delta();
  rect.h -= toolbar_size.h;
  rect.y += toolbar_size.h;
  return rect;
}

void advance_viewport_state() {
  viewport().advance_state( viewport_rect() );

  viewport().fix_zoom_rounding();

  // TODO: should only do the following when the viewport has
  // input focus.
  auto const* __state = ::SDL_GetKeyboardState( nullptr );

  // Returns true if key is pressed.
  auto state = [__state]( ::SDL_Scancode code ) {
    return __state[code] != 0;
  };

  if( state( ::SDL_SCANCODE_LSHIFT ) ) {
    viewport().set_x_push(
        state( ::SDL_SCANCODE_A )   ? e_push_direction::negative
        : state( ::SDL_SCANCODE_D ) ? e_push_direction::positive
                                    : e_push_direction::none );
    // y motion
    viewport().set_y_push(
        state( ::SDL_SCANCODE_W )   ? e_push_direction::negative
        : state( ::SDL_SCANCODE_S ) ? e_push_direction::positive
                                    : e_push_direction::none );

    if( state( ::SDL_SCANCODE_A ) || state( ::SDL_SCANCODE_D ) ||
        state( ::SDL_SCANCODE_W ) || state( ::SDL_SCANCODE_S ) )
      viewport().stop_auto_panning();
  }
}

/****************************************************************
** Map Editor Plane
*****************************************************************/
struct MapEditorPlane : public Plane {
  MapEditorPlane() = default;

  bool covers_screen() const override { return true; }

  void initialize( IMapUpdater& map_updater ) override {
    viewport().set_max_viewable_size_tiles(
        map_updater.matrix().size() );
    map_updater.just_redraw_map();
    // This is done to initialize the viewport with info about
    // the viewport size that cannot be known while it is being
    // constructed.
    advance_viewport_state();
    map_updater_ = &map_updater;
  }

  void advance_state() override { advance_viewport_state(); }

  void draw( rr::Renderer& renderer ) const override {
    renderer.set_camera(
        viewport()
            .landscape_buffer_render_upper_left()
            .distance_from_origin(),
        viewport().get_zoom() );
    // Should do this after setting the camera.
    renderer.render_buffer(
        rr::e_render_target_buffer::landscape );
    render_sidebar( renderer );
    render_toolbar( renderer );
  }

  e_input_handled input( input::event_t const& event ) override {
    input::event_t event_translated = move_mouse_origin_by(
        event, canvas_.upper_left().distance_from_origin() );
    g_input.send( event_translated );
    if( event_translated.holds<input::win_event_t>() )
      // Generally we should return no here because this is an
      // event that we want all planes to see. FIXME: need to
      // find a better way to handle this automatically.
      return e_input_handled::no;
    return e_input_handled::yes;
  }

  /**************************************************************
  ** Dragging
  ***************************************************************/
  struct DragUpdate {
    Coord prev;
    Coord current;
  };
  // Here, `nothing` is used to indicate that it has ended. NOTE:
  // this needs to have update() called on it in the plane's
  // advance_state method.
  co::finite_stream<DragUpdate> drag_stream;
  // The waitable will be waiting on the drag_stream, so it must
  // come after so that it gets destroyed first.
  maybe<wait<>> drag_thread;
  bool          drag_finished = true;

  wait<> dragging( input::e_mouse_button button,
                   Coord /*coord*/ ) {
    SCOPE_EXIT( drag_finished = true );
    if( button == input::e_mouse_button::r ) {
      while( maybe<DragUpdate> d = co_await drag_stream.next() )
        viewport().pan_by_screen_coords( d->prev - d->current );
    } else {
      while( maybe<DragUpdate> d =
                 co_await drag_stream.next() ) {
        if( auto maybe_tile =
                viewport().screen_pixel_to_world_tile(
                    d->current ) ) {
          co_await click_on_tile( *map_updater_, *maybe_tile,
                                  e_action::add );
        }
      }
    }
  }

  Plane::e_accept_drag can_drag( input::e_mouse_button button,
                                 Coord origin ) override {
    if( !drag_finished ) return Plane::e_accept_drag::swallow;
    if( button == input::e_mouse_button::r &&
        viewport().screen_coord_in_viewport( origin ) ) {
      viewport().stop_auto_panning();
      drag_stream.reset();
      drag_finished = false;
      drag_thread   = dragging( button, origin );
      return Plane::e_accept_drag::yes;
    }
    if( button == input::e_mouse_button::l &&
        viewport().screen_pixel_to_world_tile( origin ) ) {
      viewport().stop_auto_panning();
      drag_stream.reset();
      drag_finished = false;
      drag_thread   = dragging( button, origin );
      return Plane::e_accept_drag::yes;
    }
    return Plane::e_accept_drag::no;
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

  Rect         canvas_;
  IMapUpdater* map_updater_ = nullptr;
};

MapEditorPlane g_map_editor_plane;

} // namespace

/****************************************************************
** Public API
*****************************************************************/
wait<> map_editor( IMapUpdater& map_updater ) {
  reset_globals();
  ScopedPlanePush pusher( e_plane_config::map_editor );
  lg.info( "entering map editor." );
  co_await run_map_editor( map_updater );
  lg.info( "leaving map editor." );
}

Plane* map_editor_plane() { return &g_map_editor_plane; }

} // namespace rn
