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
#include "land-view.hpp"
#include "plane-stack.hpp"
#include "render.hpp"
#include "society.hpp"
#include "time.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
#include "visibility.hpp"

// config
#include "config/ui.rds.hpp"

// ss
#include "ss/land-view.rds.hpp"
#include "ss/nation.rds.hpp"
#include "ss/native-enums.rds.hpp"
#include "ss/natives.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"

// render
#include "render/renderer.hpp"

// gfx
#include "gfx/iter.hpp"

// base
#include "base/scope-exit.hpp"

using namespace std;

namespace rn {

namespace {

int constexpr kPixelsPerPoint = 2;

// Used to implement <= and >= on doubles below.
double const kEp = .0001;

gfx::pixel parse_color( string_view hex ) {
  UNWRAP_CHECK_MSG( res, gfx::pixel::parse_from_hex( hex ),
                    "failed to parse hex color '{}'.", hex );
  return res;
}

gfx::pixel const kHiddenColor =
    parse_color( "2c3468" ).shaded( 3 );
gfx::pixel const kOceanColor   = parse_color( "404b78" );
gfx::pixel const kSeaLaneColor = parse_color( "46557d" );

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
  if( square.surface == e_surface::water )
    return square.sea_lane ? kSeaLaneColor : kOceanColor;
  if( square.overlay.has_value() ) {
    switch( *square.overlay ) {
      case e_land_overlay::mountains:
        return kMountainsColor;
      case e_land_overlay::hills:
        return kHillsColor;
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
    case e_ground_terrain::arctic:
      return kArcticColor;
    case e_ground_terrain::desert:
      return kDesertColor;
    case e_ground_terrain::grassland:
      return kGrasslandColor;
    case e_ground_terrain::marsh:
      return kMarshColor;
    case e_ground_terrain::plains:
      return kPlainsColor;
    case e_ground_terrain::prairie:
      return kPrairieColor;
    case e_ground_terrain::savannah:
      return kSavannahColor;
    case e_ground_terrain::swamp:
      return kSwampColor;
    case e_ground_terrain::tundra:
      return kTundraColor;
  }
}

} // namespace

/****************************************************************
** MiniMap
*****************************************************************/
void MiniMap::fix_invariants() {
  gfx::drect const world =
      ss_.terrain.world_rect_tiles().to_gfx().to_double();
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

  static constexpr double kEpsilon = 0.000001;

  visible = tiles_visible_on_minimap();
  CHECK_GE( visible.left(), world.left() - kEpsilon );
  CHECK_GE( visible.top(), world.top() - kEpsilon );
  CHECK_LE( visible.right(), world.right() + kEpsilon );
  CHECK_LE( visible.bottom(), world.bottom() + kEpsilon );
}

MiniMap::MiniMap( SS& ss, gfx::size available_size )
  : ss_( ss ) {
  SCOPE_EXIT( fix_invariants() );
  // Compute size_screen_pixels_.
  gfx::size const world_size    = ss_.terrain.world_size_tiles();
  gfx::size const pixels_needed = world_size * kPixelsPerPoint;
  gfx::rect const available =
      gfx::rect{ .origin = {}, .size = available_size };
  gfx::rect const clamped_pixels =
      gfx::rect{ .origin = available.nw(),
                 .size   = pixels_needed }
          .clamped( available );
  size_screen_pixels_ = clamped_pixels.size;
  // We want to make it square, but only if it doesn't fit along
  // at least one dimension.
  if( pixels_needed.w > available.size.w ||
      pixels_needed.h > available.size.h )
    size_screen_pixels_.w = size_screen_pixels_.h =
        std::min( size_screen_pixels_.w, size_screen_pixels_.h );
}

void MiniMap::drag_map( gfx::size const mouse_delta ) {
  SCOPE_EXIT( fix_invariants() );
  SmoothViewport& viewport = ss_.land_view.viewport;
  viewport.stop_auto_zoom();
  viewport.stop_auto_panning();

  gfx::drect const prev_visible = tiles_visible_on_minimap();
  gfx::drect const prev_white_box =
      fractional_tiles_inside_white_box();

  ss_.land_view.minimap.origin -=
      mouse_delta.to_double() / kPixelsPerPoint;
  fix_invariants();

  // Needs to be computed after changing the origin.
  gfx::drect const visible = tiles_visible_on_minimap();

  // Needs to be recomputed after each viewport change.
  gfx::drect white_box = fractional_tiles_inside_white_box();

  auto pan_horizontal = [&]( double const w ) {
    viewport.pan_by_world_coords( gfx::dsize{ .w = w * 32 } );
    white_box = fractional_tiles_inside_white_box();
  };

  auto pan_vertical = [&]( double const h ) {
    viewport.pan_by_world_coords( gfx::dsize{ .h = h * 32 } );
    white_box = fractional_tiles_inside_white_box();
  };

  // The below is what makes the viewport (white box) change po-
  // sition when we are dragging the mini-map and it pushes the
  // white box up against the edge while there is still more
  // mini-map to scroll. If we didn't do this, then the white box
  // would just run off of the mini-map. For each direction of
  // drag, we have to deal with two edges of the white box that
  // can come up against the edge of the mini-map: one in the
  // "push" direction and the other in the "pull" direction.

  if( mouse_delta.w > 0 ) {
    if( white_box.left() > visible.left() &&
        prev_white_box.left() < prev_visible.left() + kEp &&
        white_box.right() > visible.right() )
      pan_horizontal( visible.left() - white_box.left() );
    if( white_box.right() > visible.right() &&
        prev_white_box.right() < prev_visible.right() + kEp &&
        white_box.left() > visible.left() )
      pan_horizontal( visible.right() - white_box.right() );
  }

  if( mouse_delta.w < 0 ) {
    if( white_box.right() < visible.right() &&
        prev_white_box.right() > prev_visible.right() - kEp &&
        white_box.left() < visible.left() )
      pan_horizontal( visible.right() - white_box.right() );
    if( white_box.left() < visible.left() &&
        prev_white_box.left() > prev_visible.left() - kEp &&
        white_box.right() < visible.right() )
      pan_horizontal( visible.left() - white_box.left() );
  }

  if( mouse_delta.h > 0 ) {
    if( white_box.top() > visible.top() &&
        prev_white_box.top() < prev_visible.top() + kEp &&
        white_box.bottom() > visible.bottom() )
      pan_vertical( visible.top() - white_box.top() );
    if( white_box.bottom() > visible.bottom() &&
        prev_white_box.bottom() < prev_visible.bottom() + kEp &&
        white_box.top() > visible.top() )
      pan_vertical( visible.bottom() - white_box.bottom() );
  }

  if( mouse_delta.h < 0 ) {
    if( white_box.bottom() < visible.bottom() &&
        prev_white_box.bottom() > prev_visible.bottom() - kEp &&
        white_box.top() < visible.top() )
      pan_vertical( visible.bottom() - white_box.bottom() );
    if( white_box.top() < visible.top() &&
        prev_white_box.top() > prev_visible.top() - kEp &&
        white_box.bottom() < visible.bottom() )
      pan_vertical( visible.top() - white_box.top() );
  }
}

void MiniMap::drag_box( gfx::size const mouse_delta ) {
  SCOPE_EXIT( fix_invariants() );
  SmoothViewport& viewport = ss_.land_view.viewport;
  viewport.stop_auto_zoom();
  viewport.stop_auto_panning();
  static_assert( kPixelsPerPoint / 2 > 0 );
  viewport.pan_by_world_coords(
      Delta::from_gfx( mouse_delta * 32 / kPixelsPerPoint ) );
  MiniMapState& minimap = ss_.land_view.minimap;

  gfx::drect const white_box =
      fractional_tiles_inside_white_box();

  // Needs to be recomputed after each origin change.
  gfx::drect visible = tiles_visible_on_minimap();

  // This is what causes the mini-map to scroll when we push
  // the white box up against the boundaries.

  if( white_box.size.w <= visible.size.w ) {
    if( mouse_delta.w < 0 ) {
      if( white_box.left() < visible.left() &&
          white_box.right() < visible.right() ) {
        minimap.origin.x = white_box.origin.x;
        fix_invariants();
        visible = tiles_visible_on_minimap();
      }
    }

    if( mouse_delta.w > 0 ) {
      visible = tiles_visible_on_minimap();
      if( white_box.right() > visible.right() &&
          white_box.left() > visible.left() ) {
        minimap.origin.x +=
            ( white_box.right() - visible.right() );
        fix_invariants();
        visible = tiles_visible_on_minimap();
      }
    }
  }

  if( white_box.size.h <= visible.size.h ) {
    if( white_box.top() < visible.top() &&
        white_box.bottom() < visible.bottom() ) {
      minimap.origin.y = white_box.origin.y;
      fix_invariants();
      visible = tiles_visible_on_minimap();
    }

    if( white_box.bottom() > visible.bottom() &&
        white_box.top() > visible.top() ) {
      minimap.origin.y +=
          ( white_box.bottom() - visible.bottom() );
      fix_invariants();
      visible = tiles_visible_on_minimap();
    }
  }
}

gfx::drect MiniMap::tiles_visible_on_minimap() const {
  return { .origin = origin(),
           .size   = size_screen_pixels_.to_double() /
                   kPixelsPerPoint };
}

gfx::dpoint MiniMap::origin() const {
  return ss_.land_view.minimap.origin;
}

void MiniMap::set_origin( gfx::dpoint p ) {
  ss_.land_view.minimap.origin = p;
  fix_invariants();
}

gfx::drect MiniMap::fractional_tiles_inside_white_box() const {
  return ss_.land_view.viewport.covered_pixels() / 32.0;
}

void MiniMap::advance_auto_pan() {
  SCOPE_EXIT( fix_invariants() );
  MiniMapState& minimap   = ss_.land_view.minimap;
  gfx::drect    visible   = tiles_visible_on_minimap();
  gfx::drect    white_box = fractional_tiles_inside_white_box();

  // FIXME: this auto panning needs to be made to happen at a
  // rate that is independent of frame rate. Currently it only
  // seems to matter on large maps, and so it is not urgent.
  auto advance = [&]( double& from, double const to ) {
    double const speed =
        ( to >= from ) ? animation_speed_ : -animation_speed_;
    from += speed;
    if( speed >= 0 )
      from = std::min( from, to );
    else
      from = std::max( from, to );
    fix_invariants();
    visible   = tiles_visible_on_minimap();
    white_box = fractional_tiles_inside_white_box();
  };

  if( white_box.left() < visible.left() &&
      white_box.right() < visible.right() )
    advance( minimap.origin.x, white_box.origin.x );

  if( white_box.right() > visible.right() &&
      white_box.left() > visible.left() )
    advance( minimap.origin.x,
             minimap.origin.x +
                 ( white_box.right() - visible.right() ) );

  if( white_box.top() < visible.top() &&
      white_box.bottom() < visible.bottom() )
    advance( minimap.origin.y, white_box.origin.y );

  if( white_box.bottom() > visible.bottom() &&
      white_box.top() > visible.top() )
    advance( minimap.origin.y,
             minimap.origin.y +
                 ( white_box.bottom() - visible.bottom() ) );
}

/****************************************************************
** MiniMapView
*****************************************************************/
void MiniMapView::advance_state() {
  if( !drag_state_.has_value() ) mini_map_.advance_auto_pan();
}

gfx::rect MiniMapView::white_box_pixels() const {
  gfx::drect white_rect =
      mini_map_.fractional_tiles_inside_white_box();
  white_rect =
      white_rect.point_becomes_origin( mini_map_.origin() );
  white_rect = white_rect * kPixelsPerPoint;
  return white_rect.truncated();
}

void MiniMapView::draw_impl( rr::Renderer&     renderer,
                             Visibility const& viz ) const {
  gfx::rect const actual{
      .origin = {}, .size = mini_map_.size_screen_pixels() };
  gfx::drect const squares =
      mini_map_.tiles_visible_on_minimap();

  rr::Painter painter = renderer.painter();

  painter.draw_solid_rect( actual, kHiddenColor );

  // Draw border.
  {
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .75 );
    render_shadow_hightlight_border(
        renderer, actual.with_border_added( 1 ),
        config_ui.window.border_light,
        config_ui.window.border_dark );
    render_shadow_hightlight_border(
        renderer, actual, config_ui.window.border_dark,
        config_ui.window.border_darker );
  }

  static Delta const pixel_size =
      Delta{ .w = kPixelsPerPoint, .h = kPixelsPerPoint };

  // See if there is a unit blinking; if so then we want to show
  // the dot blinking on the mini-map as well so that the player
  // can easily find the blinking unit.
  maybe<UnitId> const blinker =
      ts_.planes.land_view().unit_blinking();
  maybe<Coord> const blinker_coord =
      blinker.has_value()
          ? maybe<Coord>{ coord_for_unit_multi_ownership_or_die(
                ss_, *blinker ) }
          : nothing;
  // FIXME: this blinking logic needs to be sync'd with the one
  // in land-view.
  bool const blink_on =
      chrono::duration_cast<chrono::milliseconds>(
          Clock_t::now().time_since_epoch() ) %
          chrono::milliseconds{ 1000 } >
      chrono::milliseconds{ 500 };

  for( gfx::rect r : gfx::subrects( squares.truncated() ) ) {
    Coord const land_coord = Coord::from_gfx( r.nw() );
    CHECK( viz.on_map( land_coord ) );
    if( viz.visible( land_coord ) == e_tile_visibility::hidden )
      continue;
    gfx::pixel color =
        color_for_square( viz.square_at( land_coord ) );
    if( viz.visible( land_coord ) ==
        e_tile_visibility::visible_and_clear ) {
      // We have full visibility, so consider overriding the
      // normal land pixel with the colors of any units or
      // colonies that are on the square.
      if( maybe<Society> const society =
              society_on_square( ss_, land_coord );
          society.has_value() ) {
        bool const blinking_but_off =
            ( land_coord == blinker_coord && !blink_on );
        if( !blinking_but_off )
          color = flag_color_for_society( *society );
      }
    }
    gfx::rect const pixel{
        .origin = actual.nw() + ( land_coord.to_gfx() -
                                  squares.nw().truncated() ) *
                                    kPixelsPerPoint,
        .size = pixel_size };
    painter.draw_solid_rect( pixel, color );
  }

  // Finally we draw the white box. Actually we draw each segment
  // separately so that we can make rounded corners.
  gfx::rect const white_box =
      white_box_pixels().clamped( rect( Coord{} ) );
  gfx::rect const bounds = gfx::rect{
      .origin = {}, .size = mini_map_.size_screen_pixels() };
  gfx::pixel const kBoxColor =
      gfx::pixel{ .r = 0xdf, .g = 0xdf, .b = 0xef, .a = 0xff };
  // Left.
  {
    gfx::point const start = white_box.nw().clamped( bounds );
    gfx::point const end   = white_box.sw().clamped( bounds );
    painter.draw_vertical_line(
        start + gfx::size{ .h = 1 },
        std::max( 0, end.y - start.y - 2 + 1 ), kBoxColor );
  }
  // Right.
  {
    gfx::point const start = white_box.ne().clamped( bounds );
    gfx::point const end   = white_box.se().clamped( bounds );
    painter.draw_vertical_line(
        start + gfx::size{ .h = 1 },
        std::max( 0, end.y - start.y - 2 + 1 ), kBoxColor );
  }
  // Top.
  {
    gfx::point const start = white_box.nw().clamped( bounds );
    gfx::point const end   = white_box.ne().clamped( bounds );
    painter.draw_horizontal_line(
        start + gfx::size{ .w = 1 },
        std::max( 0, end.x - start.x - 2 + 1 ), kBoxColor );
  }
  // Bottom.
  {
    gfx::point const start = white_box.sw().clamped( bounds );
    gfx::point const end   = white_box.se().clamped( bounds );
    painter.draw_horizontal_line(
        start + gfx::size{ .w = 1 },
        std::max( 0, end.x - start.x - 2 + 1 ), kBoxColor );
  }
}

void MiniMapView::draw( rr::Renderer& renderer,
                        Coord         where ) const {
  Visibility const viz( ss_, ts_.map_updater.options().nation );
  SCOPED_RENDERER_MOD_ADD(
      painter_mods.repos.translation,
      where.distance_from_origin().to_gfx().to_double() );
  draw_impl( renderer, viz );
}

Delta MiniMapView::delta() const {
  return Delta::from_gfx( mini_map_.size_screen_pixels() );
}

bool MiniMapView::on_wheel(
    input::mouse_wheel_event_t const& event ) {
  SmoothViewport& viewport = ss_.land_view.viewport;
  viewport.stop_auto_zoom();
  viewport.stop_auto_panning();
  viewport.set_zoom_push( e_push_direction::positive, nothing );
  if( event.wheel_delta < 0 ) {
    viewport.stop_auto_zoom();
    viewport.stop_auto_panning();
    viewport.set_zoom_push( e_push_direction::negative,
                            nothing );
  }
  if( event.wheel_delta > 0 ) {
    viewport.stop_auto_zoom();
    viewport.stop_auto_panning();
    viewport.set_zoom_push( e_push_direction::positive,
                            nothing );
  }
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
    drag_state_ = event.state.origin.is_inside(
                      Rect::from_gfx( white_box_pixels() ) )
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
    CHECK( event.delta() == Delta{} );
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
  SmoothViewport& viewport = ss_.land_view.viewport;
  viewport.stop_auto_zoom();
  viewport.stop_auto_panning();
  // We shouldn't have received this event if the position was
  // not in the view, and the view is exactly the size of the
  // mini-map, so we can assume that the location of the click
  // will always correspond to a real tile.
  gfx::dpoint const p = event.pos.to_gfx().to_double();
  viewport.center_on_tile( Coord::from_gfx(
      ( p / kPixelsPerPoint +
        mini_map_.origin().distance_from_origin() )
          .truncated() ) );
  return true;
}

} // namespace rn
