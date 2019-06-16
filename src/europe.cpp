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
//  void draw( Texture const& tx ) const;
//  Rect bounds() const;

// This object represents the array of cargo items available for
// trade in europe and which is show at the bottom of the screen.
class MarketCommodities {
  static constexpr W single_layer_blocks_width  = 16_w;
  static constexpr W double_layer_blocks_width  = 8_w;
  static constexpr H single_layer_blocks_height = 1_h;
  static constexpr H double_layer_blocks_height = 2_h;

  static constexpr auto sprite_with_border_scale = Scale{16 + 1};

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

  void draw( Texture const& tx ) const {
    auto rect = bounds().with_new_upper_left( Coord{} );
    for( auto coord : rect / sprite_with_border_scale )
      render_rect(
          tx, Color::black(),
          Rect::from(
              coord * sprite_with_border_scale +
                  origin_.distance_from_origin(),
              Delta{1_w, 1_h} * sprite_with_border_scale +
                  Delta{1_w, 1_h} ) );
  }

  MarketCommodities( MarketCommodities&& ) = default;
  MarketCommodities& operator=( MarketCommodities&& ) = default;

  static Opt<MarketCommodities> create( Rect const& rect ) {
    Opt<MarketCommodities> res;
    if( rect.w >= single_layer_width &&
        rect.h >= single_layer_height ) {
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

private:
  MarketCommodities() = default;
  MarketCommodities( bool doubled, Coord origin )
    : doubled_( doubled ), origin_( origin ) {}
  bool  doubled_{};
  Coord origin_{};
};

//- Outbound ships
//- Inbound ships
//- Ships in dock
//- Dock
//- Units on dock
//- Ship cargo
//- Exit button
//- Buttons
//- Message box
//- Stats area (money, tax rate, etc.)

struct Entities {
  Opt<MarketCommodities> market_commodities;
  // ...
};

void create_entities( Entities* entities ) {
  entities->market_commodities =
      MarketCommodities::create( clip_rect() );
  // ...
}

void draw_entities( Texture const&  tx,
                    Entities const& entities ) {
  if( entities.market_commodities.has_value() )
    entities.market_commodities->draw( tx );
  // ...
}

} // namespace entity
/****************************************************************
** The Europe Plane
*****************************************************************/
struct EuropePlane : public Plane {
  EuropePlane() = default;
  bool enabled() const override { return true; }
  bool covers_screen() const override { return true; }
  void draw( Texture const& tx ) const override {
    // clear_texture_transparent( tx );
    clear_texture( tx, Color::white() );
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
    auto matcher = scelta::match(
        []( input::unknown_event_t ) { return false; },
        []( input::quit_event_t ) { return false; },
        []( input::key_event_t const& ) { return false; },
        []( input::mouse_wheel_event_t ) { return false; },
        [&]( input::mouse_move_event_t mv_event ) {
          if( is_on_clip_rect( mv_event.pos ) )
            this->rect_color = Color::blue();
          else
            this->rect_color = Color::black();
          return true;
        },
        [&]( input::mouse_button_event_t ) { return false; },
        []( input::mouse_drag_event_t ) {
          // The framework does not send us mouse drag events
          // directly; instead it uses the api methods on the
          // Plane class.
          SHOULD_NOT_BE_HERE;
          return false;
        } );
    return matcher( event );
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
