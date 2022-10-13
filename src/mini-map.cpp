/****************************************************************
**mini-map.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-11.
*
* Description: Draws the mini-map on the panel.
*
*****************************************************************/
#include "mini-map.hpp"

// Revolution Now
#include "imap-updater.hpp"
#include "nation.hpp"
#include "ts.hpp"
#include "visibility.hpp"

// config
#include "config/nation.rds.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/terrain.hpp"

// render
#include "render/renderer.hpp"

// base
#include "base/scope-exit.hpp"

using namespace std;

namespace rn {

namespace {

double const kSpeed = 3;

int constexpr kPixelsPerPoint = 2;

gfx::pixel parse_color( string_view hex ) {
  UNWRAP_CHECK_MSG( res, gfx::pixel::parse_from_hex( hex ),
                    "failed to parse hex color '{}'.", hex );
  return res;
}

gfx::pixel const kHiddenColor =
    parse_color( "2c3468" ).shaded( 3 );
gfx::pixel const kOceanColor = parse_color( "404b78" );

gfx::pixel const kMountainsColor = parse_color( "abafb0" );
gfx::pixel const kHillsColor     = parse_color( "6b5f5c" );

gfx::pixel const kArcticColor    = parse_color( "f5f4f4" );
gfx::pixel const kDesertColor    = parse_color( "d3bf99" );
gfx::pixel const kGrasslandColor = parse_color( "3c6d35" );
gfx::pixel const kMarshColor     = parse_color( "46836f" );
gfx::pixel const kPlainsColor    = parse_color( "928547" );
gfx::pixel const kPrairieColor   = parse_color( "a19e47" );
gfx::pixel const kSavannahColor  = parse_color( "548746" );
gfx::pixel const kSwampColor     = parse_color( "257380" );
gfx::pixel const kTundraColor    = parse_color( "cdad94" );

gfx::pixel color_for_square( MapSquare const& square ) {
  if( square.surface == e_surface::water ) return kOceanColor;
  if( square.overlay.has_value() ) {
    switch( *square.overlay ) {
      case e_land_overlay::mountains: return kMountainsColor;
      case e_land_overlay::hills: return kHillsColor;
      case e_land_overlay::forest:
        // For forested tiles just render the underyling tile.
        // That is what the OG does and it looks better, other-
        // wise the entire map is basically covered in forest and
        // doesn't really provide useful terrain info. This won't
        // reveal anything that the player can't already see be-
        // cause the player can see the underlying terrain by
        // using the "Show Hidden Terrain" feature, which the OG
        // has as well.
        break;
    }
  }
  switch( square.ground ) {
    case e_ground_terrain::arctic: return kArcticColor;
    case e_ground_terrain::desert: return kDesertColor;
    case e_ground_terrain::grassland: return kGrasslandColor;
    case e_ground_terrain::marsh: return kMarshColor;
    case e_ground_terrain::plains: return kPlainsColor;
    case e_ground_terrain::prairie: return kPrairieColor;
    case e_ground_terrain::savannah: return kSavannahColor;
    case e_ground_terrain::swamp: return kSwampColor;
    case e_ground_terrain::tundra: return kTundraColor;
  }
}

} // namespace

/****************************************************************
** MiniMap
*****************************************************************/
void MiniMap::fix_invariants() {
  // These two won't change as we make changes.
  gfx::drect const world =
      gfx::to_double( ss_.terrain.world_rect_tiles().to_gfx() );
  MiniMapState& minimap = ss_.land_view.minimap;

  // This one needs to be recomputed each time we change the
  // mini-map origin and then again look at the same coordinate.
  // We don't have to recompute it between examinations or x and
  // y dimensions though.
  gfx::drect visible = tiles_visible_on_minimap();
  if( visible.left() < world.left() )
    minimap.origin.x = world.origin.x;
  if( visible.top() < world.top() )
    minimap.origin.y = world.origin.y;

  visible = tiles_visible_on_minimap();
  if( visible.right() > world.right() )
    minimap.origin.x -= ( visible.right() - world.right() );
  if( visible.bottom() > world.bottom() )
    minimap.origin.y -= ( visible.bottom() - world.bottom() );
}

MiniMap::MiniMap( SS& ss, Delta available_size ) : ss_( ss ) {
  SCOPE_EXIT( fix_invariants() );
  // Compute pixels_occupied_.
  Delta const world_size = ss_.terrain.world_size_tiles();
  Rect const  available  = Rect::from( Coord{}, available_size );
  Delta const pixels_needed = world_size * kPixelsPerPoint;
  Rect const  clamped_pixels =
      Rect::from( available.upper_left(), pixels_needed )
          .clamp( available );
  pixels_occupied_ = clamped_pixels.delta();
}

void MiniMap::zoom_in() {
  SCOPE_EXIT( fix_invariants() );
  SmoothViewport& viewport = ss_.land_view.viewport;
  viewport.stop_auto_zoom();
  viewport.stop_auto_panning();
  viewport.set_zoom_push( e_push_direction::positive, nothing );
}

void MiniMap::zoom_out() {
  SCOPE_EXIT( fix_invariants() );
  SmoothViewport& viewport = ss_.land_view.viewport;
  viewport.stop_auto_zoom();
  viewport.stop_auto_panning();
  viewport.set_zoom_push( e_push_direction::negative, nothing );
}

void MiniMap::center_box_on_tile( gfx::point where ) {
  SCOPE_EXIT( fix_invariants() );
  SmoothViewport&         viewport = ss_.land_view.viewport;
  maybe<gfx::point> const tile     = tile_under_cursor( where );
  if( !tile.has_value() ) return;
  viewport.center_on_tile( Coord::from_gfx( *tile ) );
}

void MiniMap::center_map_on_tile( gfx::point where ) {
  SCOPE_EXIT( fix_invariants() );
  ss_.land_view.minimap.origin =
      gfx::to_double( where ) -
      tiles_visible_on_minimap().size / 2;
}

void MiniMap::drag_map( Delta mouse_delta ) {
  SCOPE_EXIT( fix_invariants() );
  SmoothViewport& viewport = ss_.land_view.viewport;
  viewport.stop_auto_zoom();
  viewport.stop_auto_panning();
  gfx::drect const world =
      gfx::to_double( ss_.terrain.world_rect_tiles().to_gfx() );

  ss_.land_view.minimap.origin -=
      gfx::to_double( mouse_delta.to_gfx() ) / kPixelsPerPoint;

  // Needs to be computed after changing the origin.
  gfx::drect const visible = tiles_visible_on_minimap();

  // Needs to be recomputed after each viewport change.
  gfx::drect white_box = fractional_tiles_inside_white_box();

  // This is what makes the viewport (white box) change position
  // when we are dragging the mini-map and it pushes the white
  // box up against the edge while there is still more mini-map
  // to scroll. If we didn't do this, then the mini-map would
  // stop panning when the white box's edges hit the edges of the
  // mini-map.

  auto left = [&] {
    if( white_box.left() < visible.left() &&
        visible.right() <= world.right() &&
        white_box.right() <= visible.right() ) {
      ss_.land_view.viewport.pan_by_world_coords( gfx::dsize{
          .w = -( white_box.left() - visible.left() ) * 32 } );
      white_box = fractional_tiles_inside_white_box();
    }
  };

  auto right = [&] {
    if( white_box.right() > visible.right() &&
        visible.left() >= 0 &&
        white_box.left() >= visible.left() ) {
      ss_.land_view.viewport.pan_by_world_coords( gfx::dsize{
          .w = -( white_box.right() - visible.right() ) * 32 } );
      white_box = fractional_tiles_inside_white_box();
    }
  };

  auto top = [&] {
    if( white_box.top() < visible.top() &&
        visible.bottom() <= world.bottom() &&
        white_box.bottom() <= visible.bottom() ) {
      ss_.land_view.viewport.pan_by_world_coords( gfx::dsize{
          .h = -( white_box.top() - visible.top() ) * 32 } );
      white_box = fractional_tiles_inside_white_box();
    }
  };

  auto bottom = [&] {
    if( white_box.bottom() > visible.bottom() &&
        visible.top() >= 0 &&
        white_box.top() >= visible.top() ) {
      ss_.land_view.viewport.pan_by_world_coords( gfx::dsize{
          .h = -( white_box.bottom() - visible.bottom() ) *
               32 } );
      white_box = fractional_tiles_inside_white_box();
    }
  };

  if( mouse_delta.w < 0 ) {
    left();
    right();
  }
  if( mouse_delta.w > 0 ) {
    right();
    left();
  }

  if( mouse_delta.h < 0 ) {
    top();
    bottom();
  }
  if( mouse_delta.h > 0 ) {
    bottom();
    top();
  }
}

void MiniMap::drag_box( Delta mouse_delta ) {
  SCOPE_EXIT( fix_invariants() );
  SmoothViewport& viewport = ss_.land_view.viewport;
  viewport.stop_auto_zoom();
  viewport.stop_auto_panning();
  static_assert( kPixelsPerPoint / 2 > 0 );
  viewport.pan_by_world_coords( mouse_delta * 32 /
                                kPixelsPerPoint );
}

gfx::rect MiniMap::white_box_pixels() const {
  gfx::drect white_rect = fractional_tiles_inside_white_box();
  white_rect            = white_rect.point_becomes_origin(
      upper_left_visible_tile() );
  white_rect = white_rect * kPixelsPerPoint;
  return white_rect.truncated();
}

maybe<gfx::point> MiniMap::tile_under_cursor(
    gfx::point p ) const {
  if( !Coord::from_gfx( p ).is_inside( Rect::from_gfx( gfx::rect{
          .origin = {}, .size = pixels_occupied_ } ) ) )
    return nothing;
  gfx::point const tile =
      p / kPixelsPerPoint + upper_left_visible_tile()
                                .truncated()
                                .distance_from_origin();
  return tile;
}

gfx::drect MiniMap::tiles_visible_on_minimap() const {
  // First compute the minimap area in terms of pixels, since is
  // what really constrains and determines the size of it.
  gfx::dsize const actual_pixels =
      gfx::to_double( pixels_occupied_ );

  gfx::dsize actual_tiles = actual_pixels / kPixelsPerPoint;
  gfx::drect const visible{
      .origin = ss_.land_view.minimap.origin,
      .size   = actual_tiles };
  return visible;
}

bool MiniMap::tile_visible_on_minimap( gfx::point tile ) const {
  gfx::rect const visible_rect =
      tiles_visible_on_minimap().truncated();
  return tile.is_inside( visible_rect );
}

gfx::dpoint MiniMap::upper_left_visible_tile() const {
  return ss_.land_view.minimap.origin;
}

gfx::drect MiniMap::fractional_tiles_inside_white_box() const {
  return ss_.land_view.viewport.covered_pixels() / 32.0;
}

void MiniMap::update() {
  SCOPE_EXIT( fix_invariants() );
  MiniMapState&         minimap  = ss_.land_view.minimap;
  SmoothViewport const& viewport = ss_.land_view.viewport;
  gfx::point const      viewport_center =
      viewport.covered_tiles().center();
  gfx::drect visible = tiles_visible_on_minimap();
  gfx::drect covered = fractional_tiles_inside_white_box();

  // FIXME: this auto panning needs to be made to happen at a
  // rate that is independent of frame rate. Currently it only
  // seems to matter on large maps, and so it is not urgent.
  auto advance = [&]( double& from, double const to ) {
    double const speed = ( to >= from ) ? kSpeed : -kSpeed;
    from += speed;
    if( speed >= 0 )
      from = std::min( from, to );
    else
      from = std::max( from, to );
    fix_invariants();
    visible = tiles_visible_on_minimap();
    covered = fractional_tiles_inside_white_box();
  };

  if( !tile_visible_on_minimap( viewport_center ) ) {
    gfx::dpoint const target_origin =
        gfx::to_double( viewport_center ) -
        tiles_visible_on_minimap().size / 2;
    if( target_origin.x > minimap.origin.x )
      advance( minimap.origin.x, target_origin.x );
    if( target_origin.x < minimap.origin.x )
      advance( minimap.origin.x, target_origin.x );
    if( target_origin.y > minimap.origin.y )
      advance( minimap.origin.y, target_origin.y );
    if( target_origin.y < minimap.origin.y )
      advance( minimap.origin.y, target_origin.y );
  }

  if( covered.left() < visible.left() &&
      covered.right() < visible.right() )
    advance( minimap.origin.x, covered.origin.x );
  if( covered.right() > visible.right() &&
      covered.left() > visible.left() )
    advance( minimap.origin.x,
             minimap.origin.x +
                 ( covered.right() - visible.right() ) );

  if( covered.top() < visible.top() &&
      covered.bottom() < visible.bottom() )
    advance( minimap.origin.y, covered.origin.y );
  if( covered.bottom() > visible.bottom() &&
      covered.top() > visible.top() )
    advance( minimap.origin.y,
             minimap.origin.y +
                 ( covered.bottom() - visible.bottom() ) );
}

/****************************************************************
** MiniMapView
*****************************************************************/
void MiniMapView::advance_state() { mini_map_.update(); }

void MiniMapView::draw_impl( rr::Renderer&     renderer,
                             Visibility const& viz ) const {
  gfx::rect const  actual{ .origin = {},
                           .size = mini_map_.pixels_occupied() };
  gfx::drect const squares =
      mini_map_.tiles_visible_on_minimap();

  rr::Painter painter = renderer.painter();

  painter.draw_solid_rect( actual, kHiddenColor );

  static Delta const pixel_size =
      Delta{ .w = kPixelsPerPoint, .h = kPixelsPerPoint };

  for( Coord c : Rect::from_gfx( squares.truncated() ) ) {
    Coord const land_coord = c;
    CHECK( viz.on_map( land_coord ) );
    if( !viz.visible( land_coord ) ) continue;
    gfx::pixel color = color_for_square( viz.square_at( c ) );
    // First check if there is a unit/colony on the square.
    if( maybe<e_nation> nation =
            nation_from_coord( ss_.units, ss_.colonies, c );
        nation.has_value() )
      color = config_nation.nations[*nation].flag_color;
    gfx::rect const pixel{
        .origin = actual.nw() +
                  ( c.to_gfx() - squares.nw().truncated() ) *
                      kPixelsPerPoint,
        .size = pixel_size };
    painter.draw_solid_rect( pixel, color );
  }

  // Finally we draw the white box. Actually we draw each segment
  // separately in case the viewing area is larger (possibly
  // along only one dimension) than the mini-map can show.
  gfx::rect const white_box = mini_map_.white_box_pixels();
  gfx::rect const bounds    = gfx::rect{
         .origin = {}, .size = mini_map_.pixels_occupied() };
  gfx::pixel const kBoxColor =
      gfx::pixel{ .r = 0xdf, .g = 0xdf, .b = 0xef, .a = 0xff };
  // Left.
  if( white_box.left() >= 0 ) {
    gfx::point const start = white_box.nw().clamped( bounds );
    gfx::point const end   = white_box.sw().clamped( bounds );
    painter.draw_vertical_line( start, end.y - start.y,
                                kBoxColor );
  }
  // Right.
  if( white_box.right() <= delta().w ) {
    gfx::point const start = white_box.ne().clamped( bounds );
    gfx::point const end   = white_box.se().clamped( bounds );
    painter.draw_vertical_line( start, end.y - start.y + 1,
                                kBoxColor );
  }
  // Top.
  if( white_box.top() >= 0 ) {
    gfx::point const start = white_box.nw().clamped( bounds );
    gfx::point const end   = white_box.ne().clamped( bounds );
    painter.draw_horizontal_line( start, end.x - start.x,
                                  kBoxColor );
  }
  // Bottom.
  if( white_box.bottom() <= delta().h ) {
    gfx::point const start = white_box.sw().clamped( bounds );
    gfx::point const end   = white_box.se().clamped( bounds );
    painter.draw_horizontal_line( start, end.x - start.x + 1,
                                  kBoxColor );
  }
}

void MiniMapView::draw( rr::Renderer& renderer,
                        Coord         where ) const {
  Visibility const viz = Visibility::create(
      ss_, ts_.map_updater.options().nation );
  SCOPED_RENDERER_MOD_ADD(
      painter_mods.repos.translation,
      gfx::to_double( where.distance_from_origin().to_gfx() ) );
  draw_impl( renderer, viz );
}

Delta MiniMapView::delta() const {
  return Delta::from_gfx( mini_map_.pixels_occupied() );
}

bool MiniMapView::on_wheel(
    input::mouse_wheel_event_t const& event ) {
  if( event.wheel_delta < 0 ) mini_map_.zoom_out();
  if( event.wheel_delta > 0 ) mini_map_.zoom_in();
  return true;
}

bool MiniMapView::on_mouse_drag(
    input::mouse_drag_event_t const& event ) {
  // Note that the event.pos (mouse position) may not be inside
  // the view here, because the framework allows continuing a
  // drag even when the cursor goes outside of the view where the
  // drag originated.

  // Here we want to allow dragging either the mini-map or the
  // white box with the left mouse button. To do that we need to
  // keep track of what we're dragging; we can't just check if
  // the mouse cursor is over the white box because the white box
  // can get pushed around when dragging the map in ways that
  // would cause us to start dragging the map but end up dragging
  // the white box.
  if( event.state.phase == input::e_drag_phase::begin ) {
    drag_state_ = event.state.origin.is_inside( Rect::from_gfx(
                      mini_map_.white_box_pixels() ) )
                      ? e_mini_map_drag::white_box
                      : e_mini_map_drag::map;
    // When the right mouse button is used, always drag the map.
    // This allows the player a way to drag the map when the
    // white box is occupying the entire mini-map (which can
    // happen when all the way zoomed out), in which case the
    // player would not have any map area (outside the white box)
    // to click on to drag it.
    if( event.button == input::e_mouse_button::r )
      drag_state_ = e_mini_map_drag::map;
  } else if( event.state.phase == input::e_drag_phase::end ) {
    drag_state_.reset();
    return true;
  }

  if( !drag_state_.has_value() )
    // This is just defensive... it could potentially happen if
    // the game window gets resized during a drag (I think).
    return false;

  switch( *drag_state_ ) {
    case e_mini_map_drag::white_box: {
      mini_map_.drag_box( event.delta() );
      break;
    }
    case e_mini_map_drag::map: {
      mini_map_.drag_map( event.delta() );
      break;
    }
  }

  return true;
}

bool MiniMapView::on_mouse_button(
    input::mouse_button_event_t const& event ) {
  if( event.buttons != input::e_mouse_button_event::left_up )
    return true;
  mini_map_.center_box_on_tile( event.pos );
  return true;
}

} // namespace rn
