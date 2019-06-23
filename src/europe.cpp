/****************************************************************
**europe.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-06-14.
*
* Description: Implements the Europe port view.
*
*****************************************************************/
#include "europe.hpp"

// Revolution Now
#include "coord.hpp"
#include "init.hpp"
#include "input.hpp"
#include "menu.hpp"
#include "plane.hpp"
#include "screen.hpp"
#include "tiles.hpp"
#include "variant.hpp"

namespace rn {

namespace {

/****************************************************************
** The Clip Rect
*****************************************************************/
Delta g_clip;

Rect clip_rect() {
  return Rect::from(
      centered( g_clip, main_window_logical_rect() ), g_clip );
}

Delta clip_rect_drag_region{4_w, 4_h};

bool is_on_clip_rect_left_side( Coord const& coord ) {
  Rect rect = clip_rect();
  return coord.x > rect.x - clip_rect_drag_region.w &&
         coord.x < rect.x + clip_rect_drag_region.w &&
         coord.y > rect.y - clip_rect_drag_region.h &&
         coord.y < rect.bottom_edge() + clip_rect_drag_region.h;
}

bool is_on_clip_rect_right_side( Coord const& coord ) {
  Rect rect = clip_rect();
  return coord.x > rect.right_edge() - clip_rect_drag_region.w &&
         coord.x < rect.right_edge() + clip_rect_drag_region.w &&
         coord.y > rect.y - clip_rect_drag_region.h &&
         coord.y < rect.bottom_edge() + clip_rect_drag_region.h;
}

bool is_on_clip_rect_top_side( Coord const& coord ) {
  Rect rect = clip_rect();
  return coord.y > rect.y - clip_rect_drag_region.h &&
         coord.y < rect.y + clip_rect_drag_region.h &&
         coord.x > rect.x - clip_rect_drag_region.w &&
         coord.x < rect.right_edge() + clip_rect_drag_region.w;
}

bool is_on_clip_rect_bottom_side( Coord const& coord ) {
  Rect rect = clip_rect();
  return coord.y >
             rect.bottom_edge() - clip_rect_drag_region.h &&
         coord.y <
             rect.bottom_edge() + clip_rect_drag_region.h &&
         coord.x > rect.x - clip_rect_drag_region.w &&
         coord.x < rect.right_edge() + clip_rect_drag_region.w;
}

bool is_on_clip_rect( Coord const& coord ) {
  return is_on_clip_rect_left_side( coord ) ||
         is_on_clip_rect_right_side( coord ) ||
         is_on_clip_rect_top_side( coord ) ||
         is_on_clip_rect_bottom_side( coord );
}

/****************************************************************
** Europe View Entities
*****************************************************************/

namespace entity {

// Each entity is defined by a struct that holds its state and
// that has the following methods:
//
//  void draw( Texture const& tx, Delta offset ) const;
//  Rect bounds() const;
//  static Opt<EntityClass> create( Delta const& size, ... );

// This object represents the array of cargo items available for
// trade in europe and which is show at the bottom of the screen.
class MarketCommodities {
  static constexpr W single_layer_blocks_width  = 16_w;
  static constexpr W double_layer_blocks_width  = 8_w;
  static constexpr H single_layer_blocks_height = 1_h;
  static constexpr H double_layer_blocks_height = 2_h;

  // Commodities will be 24x24 + 8 pixels for text.
  static constexpr auto sprite_with_border_scale = Scale{32 + 1};

  static constexpr W single_layer_width =
      single_layer_blocks_width * sprite_with_border_scale.sx +
      1_w;
  static constexpr W double_layer_width =
      double_layer_blocks_width * sprite_with_border_scale.sx +
      1_w;
  static constexpr H single_layer_height =
      single_layer_blocks_height * sprite_with_border_scale.sy +
      1_h;
  static constexpr H double_layer_height =
      double_layer_blocks_height * sprite_with_border_scale.sy +
      1_h;

public:
  Rect bounds() const {
    return Rect::from(
        origin_,
        Delta{doubled_ ? double_layer_width : single_layer_width,
              doubled_ ? double_layer_height
                       : single_layer_height} );
  }

  void draw( Texture const& tx, Delta offset ) const {
    for( auto coord : bounds().with_new_upper_left( Coord{} ) /
                          sprite_with_border_scale )
      render_rect(
          tx, Color::black(),
          Rect::from(
              coord * sprite_with_border_scale +
                  origin_.distance_from_origin() + offset,
              Delta{1_w, 1_h} * sprite_with_border_scale +
                  Delta{1_w, 1_h} ) );
  }

  MarketCommodities( MarketCommodities&& ) = default;
  MarketCommodities& operator=( MarketCommodities&& ) = default;

  static Opt<MarketCommodities> create( Delta const& size ) {
    Opt<MarketCommodities> res;
    auto                   rect = Rect::from( Coord{}, size );
    if( size.w >= single_layer_width &&
        size.h >= single_layer_height ) {
      res = MarketCommodities{
          /*doubled_=*/false,
          /*origin_=*/Coord{
              /*x=*/rect.center().x - single_layer_width / 2_sx,
              /*y=*/rect.bottom_edge() - single_layer_height}};
    } else if( rect.w >= double_layer_width &&
               rect.h >= double_layer_height ) {
      res = MarketCommodities{
          /*doubled_=*/true,
          /*origin_=*/Coord{
              /*x=*/rect.center().x - double_layer_width / 2_sx,
              /*y=*/rect.bottom_edge() - double_layer_height}};

    } else {
      // cannot draw.
    }
    return res;
  }

  bool  doubled_{};
  Coord origin_{};

private:
  MarketCommodities() = default;
  MarketCommodities( bool doubled, Coord origin )
    : doubled_( doubled ), origin_( origin ) {}
};

// This object represents the array of cargo items held by the
// ship currently selected in dock.
class ActiveCargo {
  static constexpr Delta size_blocks{6_w, 1_h};

  // Commodities will be 24x24.
  static constexpr auto sprite_with_border_scale = Scale{24 + 1};

  static constexpr Delta size_pixels =
      size_blocks * sprite_with_border_scale + Delta{1_w, 1_h};

public:
  Rect bounds() const {
    return Rect::from( origin_, size_pixels );
  }

  void draw( Texture const& tx, Delta offset ) const {
    for( auto coord : bounds().with_new_upper_left( Coord{} ) /
                          sprite_with_border_scale )
      render_rect(
          tx, Color::black(),
          Rect::from(
              coord * sprite_with_border_scale +
                  origin_.distance_from_origin() + offset,
              Delta{1_w, 1_h} * sprite_with_border_scale +
                  Delta{1_w, 1_h} ) );
  }

  ActiveCargo( ActiveCargo&& ) = default;
  ActiveCargo& operator=( ActiveCargo&& ) = default;

  static Opt<ActiveCargo> create(
      Delta const&                  size,
      Opt<MarketCommodities> const& maybe_market_commodities ) {
    Opt<ActiveCargo> res;
    auto             rect = Rect::from( Coord{}, size );
    if( size.w < size_pixels.w || size.h < size_pixels.h )
      return res;
    if( maybe_market_commodities.has_value() ) {
      auto const& market_commodities = *maybe_market_commodities;
      if( market_commodities.origin_.y < 0_y + size_pixels.h )
        return res;
      if( market_commodities.doubled_ ) {
        res = ActiveCargo(
            /*origin_=*/Coord{
                market_commodities.origin_.y - size_pixels.h +
                    1_h,
                rect.center().x - size_pixels.w / 2_sx} );
      } else {
        // Possibly just for now do this.
        res = ActiveCargo(
            /*origin_=*/Coord{
                market_commodities.origin_.y - size_pixels.h +
                    1_h,
                rect.center().x - size_pixels.w / 2_sx} );
      }
    }
    return res;
  }

private:
  ActiveCargo() = default;
  ActiveCargo( Coord origin ) : origin_( origin ) {}
  Coord origin_{};
};

//- Outbound ships
//- Inbound ships
//- Ships in dock
//- Dock
//- Units on dock
//- Exit button
//- Buttons
//- Message box
//- Stats area (money, tax rate, etc.)

struct Entities {
  Opt<MarketCommodities> market_commodities;
  Opt<ActiveCargo>       active_cargo;
};

void create_entities( Entities* entities ) {
  entities->market_commodities = //
      MarketCommodities::create( g_clip );
  entities->active_cargo = //
      ActiveCargo::create( g_clip,
                           entities->market_commodities );
}

void draw_entities( Texture const&  tx,
                    Entities const& entities ) {
  auto offset = clip_rect().upper_left().distance_from_origin();
  if( entities.market_commodities.has_value() )
    entities.market_commodities->draw( tx, offset );
  if( entities.active_cargo.has_value() )
    entities.active_cargo->draw( tx, offset );
}

} // namespace entity
/****************************************************************
** The Europe Plane
*****************************************************************/
struct EuropePlane : public Plane {
  EuropePlane() = default;
  bool enabled() const override { return true; }
  bool covers_screen() const override { return false; }
  void draw( Texture const& tx ) const override {
    clear_texture_transparent( tx );
    // clear_texture( tx, Color::white() );
    // We need to keep the checkers pattern stationary.
    auto tile = ( clip_rect().upper_left().x._ +
                  clip_rect().upper_left().y._ ) %
                            2 ==
                        0
                    ? g_tile::checkers
                    : g_tile::checkers_inv;
    tile_sprite( tx, tile, clip_rect() );
    render_rect( tx, rect_color, clip_rect() );
    entity::Entities entities;
    create_entities( &entities );
    draw_entities( tx, entities );
  }
  bool input( input::event_t const& event ) override {
    return matcher_( event ) {
      case_( input::unknown_event_t ) result_ false;
      case_( input::quit_event_t ) result_ false;
      case_( input::key_event_t ) result_ false;
      case_( input::mouse_wheel_event_t ) result_ false;
      case_( input::mouse_move_event_t ) {
        if( is_on_clip_rect( val.pos ) )
          this->rect_color = Color::blue();
        else
          this->rect_color = Color::black();
        result_ true;
      }
      case_( input::mouse_button_event_t ) result_ false;
      case_( input::mouse_drag_event_t ) result_ false;
      matcher_exhaustive;
    }
  }
  Plane::DragInfo can_drag( input::e_mouse_button button,
                            Coord origin ) override {
    if( button == input::e_mouse_button::l &&
        is_on_clip_rect( origin ) ) {
      DragInfo res( Plane::e_accept_drag::yes );
      bool     left   = is_on_clip_rect_left_side( origin );
      bool     right  = is_on_clip_rect_right_side( origin );
      bool     top    = is_on_clip_rect_top_side( origin );
      bool     bottom = is_on_clip_rect_bottom_side( origin );
      // test for corners.
      if( ( left && top ) || ( left && bottom ) ||
          ( right && top ) || ( right && bottom ) )
        return res;
      if( left || right ) res.projection = Delta{0_h, 1_w};
      if( top || bottom ) res.projection = Delta{1_h, 0_w};
      return res;
    }
    return Plane::e_accept_drag::no;
  }
  void on_drag( input::e_mouse_button /*button*/, Coord origin,
                Coord prev, Coord current ) override {
    auto delta = ( current - prev );
    delta.h *= 2_sy;
    delta.w *= 2_sx;
    if( origin.x >= main_window_logical_rect().center().x )
      g_clip.w += delta.w;
    else
      g_clip.w -= delta.w;
    if( origin.y >= main_window_logical_rect().center().y )
      g_clip.h += delta.h;
    else
      g_clip.h -= delta.h;
    g_clip.w = g_clip.w < 0_w ? 0_w : g_clip.w;
    g_clip.h = g_clip.h < 0_h ? 0_h : g_clip.h;
    g_clip   = g_clip.clamp( main_window_logical_size() );
  }
  Color rect_color{Color::black()};
};

EuropePlane g_europe_plane;

/****************************************************************
** Initialization / Cleanup
*****************************************************************/
void init_europe() {
  g_clip = main_window_logical_size() - menu_height() -
           Delta{32_w, 32_h};
}

void cleanup_europe() {}

REGISTER_INIT_ROUTINE( europe );

} // namespace

/****************************************************************
** Public API
*****************************************************************/
Plane* europe_plane() { return &g_europe_plane; }

} // namespace rn
