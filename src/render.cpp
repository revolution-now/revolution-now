/****************************************************************
**render.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-31.
*
* Description: Performs all rendering for game.
*
*****************************************************************/
#include "render.hpp"

// Revolution Now
#include "aliases.hpp"
#include "compositor.hpp"
#include "config-files.hpp"
#include "errors.hpp"
#include "gfx.hpp"
#include "logging.hpp"
#include "orders.hpp"
#include "plane.hpp"
#include "rand.hpp"
#include "screen.hpp"
#include "terrain.hpp"
#include "text.hpp"
#include "travel.hpp"
#include "ustate.hpp"
#include "variant.hpp"
#include "viewport.hpp"
#include "views.hpp"
#include "window.hpp"

// Revolution Now (config)
#include "../config/ucl/rn.inl"

// base-util
#include "base-util/algo.hpp"
#include "base-util/keyval.hpp"
#include "base-util/variant.hpp"

// SDL
#include "SDL.h"

// Abseil
#include "absl/container/flat_hash_map.h"

// C++ standard library
#include <vector>

using namespace std;

namespace rn {

namespace {

constexpr Delta nationality_icon_size( 13_h, 13_w );

// Unit only, no nationality icon.
void render_unit_no_icon( Texture& tx, e_unit_type unit_type,
                          Coord pixel_coord ) {
  auto const& desc = unit_desc( unit_type );
  render_sprite( tx, desc.tile, pixel_coord, 0, 0 );
}

} // namespace

/****************************************************************
** Rendering Building Blocks
*****************************************************************/
// FIXME: temporary
SmoothViewport& viewport() {
  static SmoothViewport svp;
  return svp;
}

Texture render_nationality_icon_impl( e_nation nation, char c ) {
  Delta       delta = nationality_icon_size;
  Rect        rect  = Rect::from( Coord{}, delta );
  Coord       origin{};
  auto const& nation_o = nation_obj( nation );

  auto tx = create_texture( delta );

  auto color  = nation_o.flag_color;
  auto dark1  = color.shaded( 2 );
  auto dark2  = dark1.shaded( 2 );
  auto dark3  = dark2.shaded( 2 );
  auto light1 = color.highlighted( 1 );
  auto light2 = light1.highlighted( 1 );
  auto light3 = light2.highlighted( 1 );

  auto text_color = color.shaded( 7 );

  render_fill_rect( tx, color, rect );

  render_line( tx, light1, origin + 1_w,
               { 0_h, delta.w - 1_w } );
  render_line( tx, light1, origin + ( delta.w - 1_w ),
               { delta.h - 1_h, 0_w } );
  render_line( tx, light2, origin + 4_w,
               { 0_h, delta.w - 4_w } );
  render_line( tx, light2, origin + ( delta.w - 1_w ),
               { delta.h - 4_h, 0_w } );
  render_line( tx, light3, origin + 7_w,
               { 0_h, delta.w - 7_w } );
  render_line( tx, light3, origin + ( delta.w - 1_w ),
               { delta.h - 7_h, 0_w } );

  render_line( tx, dark1, origin + 1_h, { delta.h - 1_h, 0_w } );
  render_line( tx, dark1, origin + ( delta.h - 1_h ),
               { 0_h, delta.w - 1_w } );
  render_line( tx, dark2, origin + 4_h, { delta.h - 4_h, 0_w } );
  render_line( tx, dark2, origin + ( delta.h - 1_h ),
               { 0_h, delta.w - 4_w } );
  render_line( tx, dark3, origin + 7_h, { delta.h - 7_h, 0_w } );
  render_line( tx, dark3, origin + ( delta.h - 1_h ),
               { 0_h, delta.w - 7_w } );

  auto const& char_tx = render_text(
      font::nat_icon(), text_color, string( 1, c ) );

  auto char_tx_size = char_tx.size();
  copy_texture(
      char_tx, tx,
      centered( char_tx_size, rect ) + Delta{ 1_w, 0_h } );

  return tx;
}

struct NatIconRenderDesc {
  e_nation nation;
  char     c;

  auto to_tuple() const { return tuple{ nation, c }; }

  // Abseil hashing API.
  template<typename H>
  friend H AbslHashValue( H h, NatIconRenderDesc const& c ) {
    return H::combine( std::move( h ), c.to_tuple() );
  }

  friend bool operator==( NatIconRenderDesc const& lhs,
                          NatIconRenderDesc const& rhs ) {
    return lhs.to_tuple() == rhs.to_tuple();
  }
};
NOTHROW_MOVE( NatIconRenderDesc );

absl::flat_hash_map<NatIconRenderDesc, Texture> nat_icon_cache;

Texture const& render_nationality_icon( e_nation nation,
                                        char     c ) {
  auto do_render = [&] {
    return render_nationality_icon_impl( nation, c );
  };

  NatIconRenderDesc desc{ nation, c };

  if( auto maybe_cached = bu::val_safe( nat_icon_cache, desc );
      maybe_cached.has_value() )
    return maybe_cached.value().get();

  nat_icon_cache.emplace( desc, do_render() );
  return nat_icon_cache[desc];
}

void render_nationality_icon( Texture&              dest,
                              UnitDescriptor const& desc,
                              e_nation              nation,
                              e_unit_orders         orders,
                              Coord pixel_coord ) {
  // Now we will advance the pixel_coord to put the icon at the
  // location specified in the unit descriptor.
  auto position = desc.nat_icon_position;
  switch( position ) {
    case +e_direction::nw: break;
    case +e_direction::ne:
      pixel_coord +=
          ( ( 1_w * g_tile_width ) - nationality_icon_size.w );
      break;
    case +e_direction::se:
      pixel_coord += ( ( Delta{ 1_w, 1_h } * g_tile_scale ) -
                       nationality_icon_size );
      break;
    case +e_direction::sw:
      pixel_coord +=
          ( ( 1_h * g_tile_height ) - nationality_icon_size.h );
      break;
      // By default we keep it in the northwest corner.
    default: break;
  };

  char c{ '-' }; // gcc seems to want us to initialize this
  switch( orders ) {
    case +e_unit_orders::none: c = '-'; break;
    case +e_unit_orders::sentry: c = 'S'; break;
    case +e_unit_orders::fortified: c = 'F'; break;
  };
  auto const& nat_icon = render_nationality_icon( nation, c );
  copy_texture( nat_icon, dest, pixel_coord );
}

void render_nationality_icon( Texture& dest, e_unit_type type,
                              e_nation      nation,
                              e_unit_orders orders,
                              Coord         pixel_coord ) {
  render_nationality_icon( dest, unit_desc( type ), nation,
                           orders, pixel_coord );
}

void render_nationality_icon( Texture& dest, UnitId id,
                              Coord pixel_coord ) {
  auto const& unit = unit_from_id( id );
  render_nationality_icon( dest, unit.desc(), unit.nation(),
                           unit.orders(), pixel_coord );
}

void render_unit( Texture& tx, UnitId id, Coord pixel_coord,
                  bool with_icon ) {
  auto const& unit = unit_from_id( id );
  if( with_icon ) {
    // Should the icon be in front of the unit or in back.
    if( !unit.desc().nat_icon_front ) {
      render_nationality_icon( tx, id, pixel_coord );
      render_unit_no_icon( tx, unit.desc().type, pixel_coord );
    } else {
      render_unit_no_icon( tx, unit.desc().type, pixel_coord );
      render_nationality_icon( tx, id, pixel_coord );
    }
  } else {
    render_unit_no_icon( tx, unit.desc().type, pixel_coord );
  }
}

void render_unit( Texture& tx, e_unit_type unit_type,
                  Coord pixel_coord ) {
  render_unit_no_icon( tx, unit_type, pixel_coord );
}

/****************************************************************
** Panel Rendering
*****************************************************************/
namespace {

struct PanelPlane : public Plane {
  PanelPlane() = default;
  bool covers_screen() const override { return false; }

  static auto rect() {
    ASSIGN_CHECK_OPT( res, compositor::section(
                               compositor::e_section::panel ) );
    return res;
  }
  static W panel_width() { return rect().w; }
  static H panel_height() { return rect().h; }

  Delta delta() const {
    return { panel_width(), panel_height() };
  }
  Coord origin() const { return rect().upper_left(); };

  ui::ButtonView& next_turn_button() {
    auto p_view = view->at( 0 );
    return *p_view.view->cast<ui::ButtonView>();
  }

  void initialize() override {
    vector<ui::OwningPositionedView> view_vec;

    auto button_view =
        make_unique<ui::ButtonView>( "Next Turn", [this] {
          lg.debug( "on to next turn." );
          this->next_turn_clicked = true;
          // Disable the button as soon as it is clicked.
          this->next_turn_button().enable( /*enabled=*/false );
        } );
    button_view->blink( /*enabled=*/true );

    auto button_size = button_view->delta();
    auto where =
        Coord{} + ( panel_width() / 2 ) - ( button_size.w / 2 );
    where += 16_h;

    ui::OwningPositionedView p_view( std::move( button_view ),
                                     where );
    view_vec.emplace_back( std::move( p_view ) );

    view = make_unique<ui::InvisibleView>(
        delta(), std::move( view_vec ) );

    next_turn_button().enable( false );
  }

  void draw( Texture& tx ) const override {
    clear_texture_transparent( tx );
    auto left_side =
        0_x + main_window_logical_size().w - panel_width();

    auto const& wood = lookup_sprite( g_tile::wood_middle );
    auto        wood_width  = wood.size().w;
    auto        wood_height = wood.size().h;

    for( Y i = rect().top_edge(); i < rect().bottom_edge();
         i += wood_height )
      render_sprite( tx, g_tile::wood_middle, i,
                     left_side + wood_width, 0, 0 );
    for( Y i = rect().top_edge(); i < rect().bottom_edge();
         i += wood_height )
      render_sprite( tx, g_tile::wood_left_edge, i, left_side, 0,
                     0 );

    view->draw( tx, origin() );
  }

  bool input( input::event_t const& event ) override {
    // FIXME: we need a window manager in the panel to avoid du-
    // plicating logic between here and the window module.
    if( input::is_mouse_event( event ) ) {
      auto maybe_pos = input::mouse_position( event );
      CHECK( maybe_pos.has_value() );
      if( maybe_pos.value().get().is_inside( rect() ) ) {
        auto new_event =
            move_mouse_origin_by( event, origin() - Coord{} );
        (void)view->input( new_event );
        return true;
      }
      return false;
    } else
      return view->input( event );
  }

  void mark_end_of_turn() {
    next_turn_button().enable( /*enabled=*/true );
    next_turn_clicked = false;
  }
  bool was_next_turn_button_clicked() const {
    return next_turn_clicked;
  }

  UPtr<ui::InvisibleView> view;
  bool                    next_turn_clicked{ false };
};

PanelPlane g_panel_plane;

} // namespace

Plane* panel_plane() { return &g_panel_plane; }

void mark_end_of_turn() { g_panel_plane.mark_end_of_turn(); }

bool was_next_turn_button_clicked() {
  return g_panel_plane.was_next_turn_button_clicked();
}

namespace {

struct EffectsPlane : public Plane {
  EffectsPlane() = default;
  bool covers_screen() const override { return false; }
  void draw( Texture& tx ) const override {
    using namespace chrono;
    clear_texture_transparent( tx );
    auto now = system_clock::now();
    if( now < start_time ) return;
    uint8_t alpha = target_alpha;
    if( now < end_time ) {
      // TODO: are these casts necessary?
      milliseconds delta =
          duration_cast<milliseconds>( end_time - now );
      milliseconds total =
          duration_cast<milliseconds>( end_time - start_time );
      double ratio = double( delta.count() ) / total.count();
      CHECK( ratio >= 0.0 && ratio <= 1.0 );
      alpha =
          uint8_t( double( target_alpha ) * ( 1.0 - ratio ) );
    }
    // TODO: this may not require changing alpha to work.
    render_fill_rect( tx, Color( 0, 0, 0, alpha ),
                      main_window_logical_rect() );
  }
  Time_t  start_time;
  Time_t  end_time;
  uint8_t target_alpha;
};

EffectsPlane g_effects_plane;

} // namespace

Plane* effects_plane() { return &g_effects_plane; }

void reset_fade_to_dark( chrono::milliseconds wait,
                         chrono::milliseconds fade,
                         uint8_t              target_alpha ) {
  CHECK( wait >= 0ms && fade >= 0ms );
  auto now                     = chrono::system_clock::now();
  g_effects_plane.start_time   = now + wait;
  g_effects_plane.end_time     = now + wait + fade;
  g_effects_plane.target_alpha = target_alpha;
}

} // namespace rn
