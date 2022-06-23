/****************************************************************
**harbor-view.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-06-14.
*
* Description: Implements the harbor view.
*
*****************************************************************/
#include "harbor-view.hpp"

// Revolution Now
#include "anim.hpp"
#include "cargo.hpp"
#include "co-combinator.hpp"
#include "co-wait.hpp"
#include "commodity.hpp"
#include "compositor.hpp"
#include "coord.hpp"
#include "dragdrop.hpp"
#include "game-state.hpp"
#include "gui.hpp"
#include "harbor-units.hpp"
#include "image.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "macros.hpp"
#include "old-world-state.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "render.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "tiles.hpp"
#include "ustate.hpp"
#include "variant.hpp"
#include "wait.hpp"
#include "window.hpp"

// game-state
#include "gs/players.hpp"
#include "gs/units.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/attributes.hpp"
#include "base/lambda.hpp"
#include "base/range-lite.hpp"
#include "base/scope-exit.hpp"
#include "base/vocab.hpp"

// Rds
#include "harbor-view-impl.rds.hpp"

using namespace std;

namespace rn {

namespace rl = ::base::rl;

namespace {

// When we drag a commodity from the market this is the default
// amount that we take.
constexpr int const k_default_market_quantity = 100;

/****************************************************************
** State
*****************************************************************/
struct PS {
  Player&             player;
  UnitsState&         units_state;
  TerrainState const& terrain_state;
  IGui&               gui;

  wait_promise<> exit_promise = {};

  maybe<drag::State<HarborDraggableObject_t>> drag_state  = {};
  maybe<wait<>>                               drag_thread = {};

  HarborState& harbor_state() {
    return player.old_world.harbor_state;
  }
};

/****************************************************************
** Draggable Object
*****************************************************************/
maybe<HarborDraggableObject_t> cargo_slot_to_draggable(
    CargoSlotIndex slot_idx, CargoSlot_t const& slot ) {
  switch( slot.to_enum() ) {
    case CargoSlot::e::empty: {
      return nothing;
    }
    case CargoSlot::e::overflow: {
      return nothing;
    }
    case CargoSlot::e::cargo: {
      auto& cargo = slot.get<CargoSlot::cargo>();
      return overload_visit(
          cargo.contents,
          []( Cargo::unit u ) -> HarborDraggableObject_t {
            return HarborDraggableObject::unit{ /*id=*/u.id };
          },
          [&]( Cargo::commodity const& c )
              -> HarborDraggableObject_t {
            return HarborDraggableObject::cargo_commodity{
                /*comm=*/c.obj,
                /*slot=*/slot_idx };
          } );
    }
  }
}

maybe<Cargo_t> draggable_to_cargo_object(
    HarborDraggableObject_t const& draggable ) {
  switch( draggable.to_enum() ) {
    case HarborDraggableObject::e::unit: {
      auto& val = draggable.get<HarborDraggableObject::unit>();
      return Cargo::unit{ val.id };
    }
    case HarborDraggableObject::e::market_commodity:
      return nothing;
    case HarborDraggableObject::e::cargo_commodity: {
      auto& val =
          draggable
              .get<HarborDraggableObject::cargo_commodity>();
      return Cargo::commodity{ val.comm };
    }
  }
}

maybe<HarborDraggableObject_t> draggable_in_cargo_slot(
    PS& S, CargoSlotIndex slot ) {
  HarborState const& hb_state = S.harbor_state();
  return hb_state.selected_unit.fmap( unit_from_id )
      .bind( LC( _.cargo().at( slot ) ) )
      .bind( LC( cargo_slot_to_draggable( slot, _ ) ) );
}

maybe<HarborDraggableObject_t> draggable_in_cargo_slot(
    PS& S, int slot ) {
  return draggable_in_cargo_slot( S, CargoSlotIndex{ slot } );
}

/****************************************************************
** Helpers
*****************************************************************/
// Both rl::all and the lambda will take rect_proxy by reference
// so we therefore must have this function take a reference to a
// rect_proxy that outlives the use of the returned range. And of
// course the Rect referred to by the rect_proxy must outlive
// everything.
auto range_of_rects( RectGridProxyIteratorHelper const&
                         rect_proxy ATTR_LIFETIMEBOUND ) {
  return rl::all( rect_proxy )
      .map( [&rect_proxy]( Coord coord ) {
        return Rect::from( coord, rect_proxy.delta() );
      } );
}

auto range_of_rects( RectGridProxyIteratorHelper&& ) = delete;

/****************************************************************
** Harbor View Entities
*****************************************************************/
namespace entity {

struct EntityBase {
  EntityBase( PS& S ) : S{ &S } {}

  PS* S;
};

// Each entity is defined by a struct that holds its state and
// that has the following methods:
//
//  void draw( rr::Renderer& renderer, Delta offset ) const;
//  Rect bounds() const;
//  static maybe<EntityClass> create( ... );
//  maybe<pair<T,Rect>> obj_under_cursor( Coord const& );

// This object represents the array of cargo items available for
// trade in europe and which is show at the bottom of the screen.
class MarketCommodities : EntityBase {
  static constexpr W single_layer_blocks_width  = 16_w;
  static constexpr W double_layer_blocks_width  = 8_w;
  static constexpr H single_layer_blocks_height = 1_h;
  static constexpr H double_layer_blocks_height = 2_h;

  // Commodities will be 24x24 + 8 pixels for text.
  static constexpr auto sprite_scale = Scale{ 32 };
  static constexpr auto sprite_delta =
      Delta{ 1_w, 1_h } * sprite_scale;

  static constexpr W single_layer_width =
      single_layer_blocks_width * sprite_scale.sx;
  static constexpr W double_layer_width =
      double_layer_blocks_width * sprite_scale.sx;
  static constexpr H single_layer_height =
      single_layer_blocks_height * sprite_scale.sy;
  static constexpr H double_layer_height =
      double_layer_blocks_height * sprite_scale.sy;

 public:
  Rect bounds() const {
    return Rect::from( origin_,
                       Delta{ doubled_ ? double_layer_width
                                       : single_layer_width,
                              doubled_ ? double_layer_height
                                       : single_layer_height } );
  }

  void draw( rr::Renderer& renderer, Delta offset ) const {
    rr::Painter painter = renderer.painter();
    auto        bds     = bounds();
    auto        grid    = bds.to_grid_noalign( sprite_delta );
    auto        comm_it = refl::enum_values<e_commodity>.begin();
    auto        label   = CommodityLabel::buy_sell{ 100, 200 };
    for( auto rect : range_of_rects( grid ) ) {
      painter.draw_empty_rect(
          rect.shifted_by( offset ),
          rr::Painter::e_border_mode::in_out,
          gfx::pixel::white() );
      render_commodity_annotated(
          renderer,
          rect.shifted_by( offset ).upper_left() +
              kCommodityInCargoHoldRenderingOffset,
          *comm_it++, label );
      label.buy += 120;
      label.sell += 120;
    }
  }

  MarketCommodities( MarketCommodities&& ) = default;
  MarketCommodities& operator=( MarketCommodities&& ) = default;

  static maybe<MarketCommodities> create( PS&          S,
                                          Delta const& size ) {
    maybe<MarketCommodities> res;
    auto                     rect = Rect::from( Coord{}, size );
    if( size.w >= single_layer_width &&
        size.h >= single_layer_height ) {
      res = MarketCommodities(
          S,
          /*doubled_=*/false,
          /*origin_=*/
          Coord{
              /*x=*/rect.center().x - single_layer_width / 2_sx,
              /*y=*/rect.bottom_edge() - single_layer_height -
                  1_h } );
    } else if( rect.w >= double_layer_width &&
               rect.h >= double_layer_height ) {
      res = MarketCommodities{
          S,
          /*doubled_=*/true,
          /*origin_=*/
          Coord{
              /*x=*/rect.center().x - double_layer_width / 2_sx,
              /*y=*/rect.bottom_edge() - double_layer_height -
                  1_h } };

    } else {
      // cannot draw.
    }
    return res;
  }

  maybe<pair<e_commodity, Rect>> obj_under_cursor(
      Coord const& coord ) const {
    maybe<pair<e_commodity, Rect>> res;
    if( coord.is_inside( bounds() ) ) {
      auto boxes =
          bounds().with_new_upper_left( Coord{} ) / sprite_scale;
      auto maybe_type =
          boxes
              .rasterize( coord.with_new_origin(
                              bounds().upper_left() ) /
                          sprite_scale )
              .bind( L( commodity_from_index( _ ) ) );
      if( maybe_type ) {
        auto box_origin =
            coord.with_new_origin( bounds().upper_left() )
                .rounded_to_multiple_to_minus_inf( sprite_scale )
                .as_if_origin_were( bounds().upper_left() ) +
            kCommodityInCargoHoldRenderingOffset;
        auto box = Rect::from( box_origin,
                               Delta{ 1_w, 1_h } * Scale{ 16 } );

        res = pair{ *maybe_type, box };
      }
    }
    return res;
  }

  bool  doubled_{};
  Coord origin_{};

 private:
  MarketCommodities( PS& S, bool doubled, Coord origin )
    : EntityBase( S ), doubled_( doubled ), origin_( origin ) {}
};
NOTHROW_MOVE( MarketCommodities );

class ActiveCargoBox : EntityBase {
  static constexpr Delta size_blocks{ 6_w, 1_h };

 public:
  // Commodities will be 24x24.
  static constexpr auto box_scale = Scale{ 32 };
  static constexpr auto box_delta =
      Delta{ 1_w, 1_h } * box_scale;
  static constexpr Delta size_pixels = size_blocks * box_scale;

  Rect bounds() const {
    return Rect::from( origin_, size_pixels );
  }

  void draw( rr::Renderer& renderer, Delta offset ) const {
    rr::Painter painter = renderer.painter();
    auto        bds     = bounds();
    auto        grid    = bds.to_grid_noalign( box_delta );
    for( auto rect : range_of_rects( grid ) )
      painter.draw_empty_rect(
          rect.shifted_by( offset ),
          rr::Painter::e_border_mode::in_out,
          gfx::pixel::white() );
  }

  ActiveCargoBox( ActiveCargoBox&& ) = default;
  ActiveCargoBox& operator=( ActiveCargoBox&& ) = default;

  static maybe<ActiveCargoBox> create(
      PS& S, Delta const& size,
      maybe<MarketCommodities> const&
          maybe_market_commodities ) {
    maybe<ActiveCargoBox> res;
    auto                  rect = Rect::from( Coord{}, size );
    if( size.w < size_pixels.w || size.h < size_pixels.h )
      return res;
    if( maybe_market_commodities.has_value() ) {
      auto const& market_commodities = *maybe_market_commodities;
      if( market_commodities.origin_.y < 0_y + size_pixels.h )
        return res;
      if( market_commodities.doubled_ ) {
        res = ActiveCargoBox(
            S,
            /*origin_=*/Coord{
                market_commodities.origin_.y - size_pixels.h,
                rect.center().x - size_pixels.w / 2_sx } );
      } else {
        // Possibly just for now do this.
        res = ActiveCargoBox(
            S,
            /*origin_=*/Coord{
                market_commodities.origin_.y - size_pixels.h,
                rect.center().x - size_pixels.w / 2_sx } );
      }
    }
    return res;
  }

 private:
  ActiveCargoBox( PS& S, Coord origin )
    : EntityBase( S ), origin_( origin ) {}
  Coord origin_{};
};
NOTHROW_MOVE( ActiveCargoBox );

class DockAnchor : EntityBase {
  static constexpr H above_active_cargo_box{ 32_h };

 public:
  Rect bounds() const {
    // Just a point.
    return Rect::from( location_, Delta{} );
  }

  void draw( rr::Renderer& renderer, Delta offset ) const {
    // This mess just draws an X.
    gfx::size char_size =
        rr::rendered_text_line_size_pixels( "X" );
    auto loc =
        Rect::from( Coord{}, Delta::from_gfx( char_size ) )
            .centered_on( location_ )
            .upper_left() +
        offset;
    renderer.typer( loc, gfx::pixel::white() ).write( "X" );
  }

  DockAnchor( DockAnchor&& ) = default;
  DockAnchor& operator=( DockAnchor&& ) = default;

  static maybe<DockAnchor> create(
      PS& S, Delta const& size,
      maybe<ActiveCargoBox> const& maybe_active_cargo_box,
      maybe<MarketCommodities> const&
          maybe_market_commodities ) {
    maybe<DockAnchor> res;
    if( maybe_active_cargo_box && maybe_market_commodities ) {
      auto active_cargo_box_top =
          maybe_active_cargo_box->bounds().top_edge();
      auto location_y =
          active_cargo_box_top - above_active_cargo_box;
      auto location_x =
          maybe_market_commodities->bounds().right_edge() - 32_w;
      auto x_upper_bound = 0_x + size.w - 60_w;
      auto x_lower_bound =
          maybe_active_cargo_box->bounds().right_edge();
      if( x_upper_bound < x_lower_bound ) return res;
      location_x =
          std::clamp( location_x, x_lower_bound, x_upper_bound );
      if( location_y < 0_y ) return res;
      res              = DockAnchor( S, {} );
      res->location_.x = location_x;
      res->location_.y = location_y;
    }
    return res;
  }

 private:
  DockAnchor( PS& S, Coord location )
    : EntityBase( S ), location_( location ) {}
  Coord location_{};
};
NOTHROW_MOVE( DockAnchor );

class Backdrop : EntityBase {
  static constexpr Delta image_distance_from_anchor{ 950_w,
                                                     544_h };

 public:
  Rect bounds() const { return Rect::from( Coord{}, size_ ); }

  void draw( rr::Renderer& renderer, Delta offset ) const {
    rr::Painter painter = renderer.painter();
    render_sprite_section(
        painter, e_tile::harbor_background, Coord{} + offset,
        Rect::from( upper_left_of_render_rect_, size_ ) );
  }

  Backdrop( Backdrop&& ) = default;
  Backdrop& operator=( Backdrop&& ) = default;

  static maybe<Backdrop> create(
      PS& S, Delta const& size,
      maybe<DockAnchor> const& maybe_dock_anchor ) {
    maybe<Backdrop> res;
    if( maybe_dock_anchor )
      res =
          Backdrop( S,
                    -( maybe_dock_anchor->bounds().upper_left() -
                       image_distance_from_anchor ),
                    size );
    return res;
  }

 private:
  Backdrop( PS& S, Coord upper_left, Delta size )
    : EntityBase( S ),
      upper_left_of_render_rect_( upper_left ),
      size_( size ) {}
  Coord upper_left_of_render_rect_{};
  Delta size_{};
};
NOTHROW_MOVE( Backdrop );

class InPortBox : EntityBase {
 public:
  static constexpr Delta block_size{ 32_w, 32_h };
  static constexpr SY    height_blocks{ 3 };
  static constexpr SX    width_wide{ 3 };
  static constexpr SX    width_narrow{ 2 };

  Rect bounds() const {
    return Rect::from( origin_, block_size * size_in_blocks_ +
                                    Delta{ 1_w, 1_h } );
  }

  void draw( rr::Renderer& renderer, Delta offset ) const {
    rr::Painter painter = renderer.painter();
    painter.draw_empty_rect( bounds().shifted_by( offset ),
                             rr::Painter::e_border_mode::inside,
                             gfx::pixel::white() );
    rr::Typer typer = renderer.typer(
        bounds().upper_left() + Delta{ 2_w, 2_h } + offset,
        gfx::pixel::white() );
    typer.write( "In Port" );
  }

  InPortBox( InPortBox&& ) = default;
  InPortBox& operator=( InPortBox&& ) = default;

  static maybe<InPortBox> create(
      PS& S, Delta const& size,
      maybe<ActiveCargoBox> const& maybe_active_cargo_box,
      maybe<MarketCommodities> const&
          maybe_market_commodities ) {
    maybe<InPortBox> res;
    if( maybe_active_cargo_box && maybe_market_commodities ) {
      bool  is_wide = !maybe_market_commodities->doubled_;
      Scale size_in_blocks;
      size_in_blocks.sy = height_blocks;
      size_in_blocks.sx = is_wide ? width_wide : width_narrow;
      auto origin =
          maybe_active_cargo_box->bounds().upper_left() -
          block_size.h * size_in_blocks.sy;
      if( origin.y < 0_y || origin.x < 0_x ) return res;

      res = InPortBox{ S, origin,      //
                       size_in_blocks, //
                       is_wide };

      auto lr_delta = res->bounds().lower_right() - Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h )
        res = nothing;
    }
    return res;
  }

  Coord origin_{};
  Scale size_in_blocks_{};
  bool  is_wide_{};

 private:
  InPortBox( PS& S, Coord origin, Scale size_in_blocks,
             bool is_wide )
    : EntityBase( S ),
      origin_( origin ),
      size_in_blocks_( size_in_blocks ),
      is_wide_( is_wide ) {}
};
NOTHROW_MOVE( InPortBox );

class InboundBox : EntityBase {
 public:
  Rect bounds() const {
    return Rect::from( origin_,
                       InPortBox::block_size * size_in_blocks_ +
                           Delta{ 1_w, 1_h } );
  }

  void draw( rr::Renderer& renderer, Delta offset ) const {
    rr::Painter painter = renderer.painter();
    painter.draw_empty_rect( bounds().shifted_by( offset ),
                             rr::Painter::e_border_mode::inside,
                             gfx::pixel::white() );
    rr::Typer typer = renderer.typer(
        bounds().upper_left() + Delta{ 2_w, 2_h } + offset,
        gfx::pixel::white() );
    typer.write( "Inbound" );
  }

  InboundBox( InboundBox&& ) = default;
  InboundBox& operator=( InboundBox&& ) = default;

  static maybe<InboundBox> create(
      PS& S, Delta const& size,
      maybe<InPortBox> const& maybe_in_port_box ) {
    maybe<InboundBox> res;
    if( maybe_in_port_box ) {
      bool  is_wide = maybe_in_port_box->is_wide_;
      Scale size_in_blocks;
      size_in_blocks.sy = InPortBox::height_blocks;
      size_in_blocks.sx = is_wide ? InPortBox::width_wide
                                  : InPortBox::width_narrow;
      auto origin = maybe_in_port_box->bounds().upper_left() -
                    InPortBox::block_size.w * size_in_blocks.sx;
      if( origin.x < 0_x ) {
        // Screen is too narrow horizontally to fit this box, so
        // we need to try to put it on top of the InPortBox.
        origin = maybe_in_port_box->bounds().upper_left() -
                 InPortBox::block_size.h * size_in_blocks.sy;
      }
      res = InboundBox{
          S,
          origin,         //
          size_in_blocks, //
          is_wide         //
      };
      auto lr_delta = res->bounds().lower_right() - Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h )
        res = nothing;
      if( res->bounds().y < 0_y ) res = nothing;
      if( res->bounds().x < 0_x ) res = nothing;
    }
    return res;
  }

  bool is_wide() const { return is_wide_; }

 private:
  InboundBox( PS& S, Coord origin, Scale size_in_blocks,
              bool is_wide )
    : EntityBase( S ),
      origin_( origin ),
      size_in_blocks_( size_in_blocks ),
      is_wide_( is_wide ) {}
  Coord origin_{};
  Scale size_in_blocks_{};
  bool  is_wide_{};
};
NOTHROW_MOVE( InboundBox );

class OutboundBox : EntityBase {
 public:
  Rect bounds() const {
    return Rect::from( origin_,
                       InPortBox::block_size * size_in_blocks_ +
                           Delta{ 1_w, 1_h } );
  }

  void draw( rr::Renderer& renderer, Delta offset ) const {
    rr::Painter painter = renderer.painter();
    painter.draw_empty_rect( bounds().shifted_by( offset ),
                             rr::Painter::e_border_mode::inside,
                             gfx::pixel::white() );
    rr::Typer typer = renderer.typer(
        bounds().upper_left() + Delta{ 2_w, 2_h } + offset,
        gfx::pixel::white() );
    typer.write( "outbound" );
  }

  OutboundBox( OutboundBox&& ) = default;
  OutboundBox& operator=( OutboundBox&& ) = default;

  static maybe<OutboundBox> create(
      PS& S, Delta const& size,
      maybe<InboundBox> const& maybe_inbound_box ) {
    maybe<OutboundBox> res;
    if( maybe_inbound_box ) {
      bool  is_wide = maybe_inbound_box->is_wide();
      Scale size_in_blocks;
      size_in_blocks.sy = InPortBox::height_blocks;
      size_in_blocks.sx = is_wide ? InPortBox::width_wide
                                  : InPortBox::width_narrow;
      auto origin = maybe_inbound_box->bounds().upper_left() -
                    InPortBox::block_size.w * size_in_blocks.sx;
      if( origin.x < 0_x ) {
        // Screen is too narrow horizontally to fit this box, so
        // we need to try to put it on top of the InboundBox.
        origin = maybe_inbound_box->bounds().upper_left() -
                 InPortBox::block_size.h * size_in_blocks.sy;
      }
      res = OutboundBox{
          S,
          origin,         //
          size_in_blocks, //
          is_wide         //
      };
      auto lr_delta = res->bounds().lower_right() - Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h )
        res = nothing;
      if( res->bounds().y < 0_y ) res = nothing;
      if( res->bounds().x < 0_x ) res = nothing;
    }
    return res;
  }

  bool is_wide() const { return is_wide_; }

 private:
  OutboundBox( PS& S, Coord origin, Scale size_in_blocks,
               bool is_wide )
    : EntityBase( S ),
      origin_( origin ),
      size_in_blocks_( size_in_blocks ),
      is_wide_( is_wide ) {}
  Coord origin_{};
  Scale size_in_blocks_{};
  bool  is_wide_{};
};
NOTHROW_MOVE( OutboundBox );

class Exit : EntityBase {
  static constexpr Delta exit_block_pixels{ 26_w, 26_h };

 public:
  Rect bounds() const {
    return Rect::from( origin_, exit_block_pixels );
  }

  void draw( rr::Renderer& renderer, Delta offset ) const {
    rr::Painter   painter   = renderer.painter();
    auto          bds       = bounds();
    static string text      = "Exit";
    Delta         text_size = Delta::from_gfx(
                rr::rendered_text_line_size_pixels( text ) );
    rr::Typer typer = renderer.typer(
        centered( text_size, bds + Delta( 1_w, 1_h ) ) + offset,
        gfx::pixel::red() );
    typer.write( text );
    painter.draw_empty_rect( bds.shifted_by( offset ),
                             rr::Painter::e_border_mode::in_out,
                             gfx::pixel::white() );
  }

  Exit( Exit&& ) = default;
  Exit& operator=( Exit&& ) = default;

  static maybe<Exit> create( PS& S, Delta const& size,
                             maybe<MarketCommodities> const&
                                 maybe_market_commodities ) {
    maybe<Exit> res;
    if( maybe_market_commodities ) {
      auto origin =
          maybe_market_commodities->bounds().lower_right() -
          exit_block_pixels.h;
      auto lr_delta = origin + exit_block_pixels - Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h ) {
        origin =
            maybe_market_commodities->bounds().upper_right() -
            exit_block_pixels;
      }
      res = Exit( S, origin );
      lr_delta =
          ( res->bounds().lower_right() - Delta{ 1_w, 1_h } ) -
          Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h )
        res = nothing;
      if( res->bounds().y < 0_y ) res = nothing;
      if( res->bounds().x < 0_x ) res = nothing;
    }
    return res;
  }

 private:
  Exit( PS& S, Coord origin )
    : EntityBase( S ), origin_( origin ) {}
  Coord origin_{};
};
NOTHROW_MOVE( Exit );

class Dock : EntityBase {
  static constexpr Scale dock_block_pixels{ 24 };
  static constexpr Delta dock_block_pixels_delta =
      Delta{ 1_w, 1_h } * dock_block_pixels;

 public:
  Rect bounds() const {
    return Rect::from(
        origin_, Delta{ length_in_blocks_ * dock_block_pixels.sx,
                        1_h * dock_block_pixels.sy } );
  }

  void draw( rr::Renderer& renderer, Delta offset ) const {
    rr::Painter painter = renderer.painter();
    auto        bds     = bounds();
    auto grid = bds.to_grid_noalign( dock_block_pixels_delta );
    for( auto rect : range_of_rects( grid ) )
      painter.draw_empty_rect(
          rect.shifted_by( offset ),
          rr::Painter::e_border_mode::in_out,
          gfx::pixel::white() );
  }

  Dock( Dock&& ) = default;
  Dock& operator=( Dock&& ) = default;

  static maybe<Dock> create(
      PS& S, Delta const& size,
      maybe<DockAnchor> const& maybe_dock_anchor,
      maybe<InPortBox> const&  maybe_in_port_box ) {
    maybe<Dock> res;
    if( maybe_dock_anchor && maybe_in_port_box ) {
      auto available = maybe_dock_anchor->bounds().left_edge() -
                       maybe_in_port_box->bounds().right_edge();
      available /= dock_block_pixels.sx;
      auto origin = maybe_dock_anchor->bounds().upper_left() -
                    ( available * dock_block_pixels.sx );
      origin -= 1_h * dock_block_pixels.sy / 2;
      res = Dock( S, /*origin_=*/origin,
                  /*length_in_blocks_=*/available );
      auto lr_delta =
          ( res->bounds().lower_right() - Delta{ 1_w, 1_h } ) -
          Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h )
        res = nothing;
      if( res->bounds().y < 0_y ) res = nothing;
      if( res->bounds().x < 0_x ) res = nothing;
    }
    return res;
  }

 private:
  Dock( PS& S, Coord origin, W length_in_blocks )
    : EntityBase( S ),
      origin_( origin ),
      length_in_blocks_( length_in_blocks ) {}
  Coord origin_{};
  W     length_in_blocks_{};
};
NOTHROW_MOVE( Dock );

// Base class for other entities that just consist of a collec-
// tion of units.
class UnitCollection : EntityBase {
 public:
  Rect bounds() const {
    auto Union = L2( _1.uni0n( _2 ) );
    auto to_rect =
        L( Rect::from( _.pixel_coord, g_tile_delta ) );
    auto maybe_rect =
        rl::all( units_ ).map( to_rect ).accumulate_monoid(
            Union );
    return maybe_rect.value_or( bounds_when_no_units_ );
  }

  void draw( rr::Renderer& renderer, Delta offset ) const {
    rr::Painter        painter  = renderer.painter();
    HarborState const& hb_state = S->harbor_state();
    // auto bds = bounds();
    // painter.draw_empty_rect( bds.shifted_by( offset ),
    // rr::Painter::e_border_mode::inside, gfx::pixel::white() );
    for( auto const& unit_with_pos : units_ )
      if( !S->drag_state || S->drag_state->object !=
                                HarborDraggableObject_t{
                                    HarborDraggableObject::unit{
                                        unit_with_pos.id } } )
        render_unit( renderer,
                     unit_with_pos.pixel_coord + offset,
                     unit_with_pos.id,
                     UnitRenderOptions{ .flag = false } );
    if( hb_state.selected_unit ) {
      for( auto [id, coord] : units_ ) {
        if( id == *hb_state.selected_unit ) {
          painter.draw_empty_rect(
              Rect::from( coord, g_tile_delta )
                      .shifted_by( offset ) -
                  Delta( 1_w, 1_h ),
              rr::Painter::e_border_mode::in_out,
              gfx::pixel::green() );
          break;
        }
      }
    }
  }

  maybe<pair<UnitId, Rect>> obj_under_cursor(
      Coord const& pos ) const {
    maybe<pair<UnitId, Rect>> res;
    for( auto [id, coord] : units_ ) {
      auto rect = Rect::from( coord, g_tile_delta );
      if( pos.is_inside( rect ) ) {
        res = pair{ id, rect };
        // !! don't break in case units are overlapping.
      }
    }
    return res;
  }

  UnitCollection( UnitCollection&& ) = default;
  UnitCollection& operator=( UnitCollection&& ) = default;

 protected:
  struct UnitWithPosition {
    UnitId id;
    Coord  pixel_coord;
  };

  UnitCollection( PS& S, Rect bounds_when_no_units,
                  vector<UnitWithPosition>&& units )
    : EntityBase( S ),
      bounds_when_no_units_( bounds_when_no_units ),
      units_( std::move( units ) ) {}
  // Returned as rect when no units. This is actaully a point.
  Rect                     bounds_when_no_units_;
  vector<UnitWithPosition> units_;
};
NOTHROW_MOVE( UnitCollection );

class UnitsOnDock : public UnitCollection {
 public:
  UnitsOnDock( UnitsOnDock&& ) = default;
  UnitsOnDock& operator=( UnitsOnDock&& ) = default;

  static maybe<UnitsOnDock> create(
      PS& S, Delta const& size,
      maybe<DockAnchor> const& maybe_dock_anchor,
      maybe<Dock> const&       maybe_dock ) {
    maybe<UnitsOnDock> res;
    if( maybe_dock_anchor && maybe_dock ) {
      vector<UnitWithPosition> units;
      Coord                    coord =
          maybe_dock->bounds().upper_right() - g_tile_delta;
      for( auto id : harbor_units_on_dock( S.units_state,
                                           S.player.nation ) ) {
        units.push_back( { id, coord } );
        coord -= g_tile_delta.w;
        if( coord.x < maybe_dock->bounds().left_edge() )
          coord = Coord{ ( maybe_dock->bounds().upper_right() -
                           g_tile_delta )
                             .x,
                         coord.y - g_tile_delta.h };
      }
      // populate units...
      res = UnitsOnDock(
          S,
          /*bounds_when_no_units_=*/maybe_dock_anchor->bounds(),
          /*units_=*/std::move( units ) );
      auto bds = res->bounds();
      auto lr_delta =
          ( bds.lower_right() - Delta{ 1_w, 1_h } ) - Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h )
        res = nothing;
      if( bds.y < 0_y ) res = nothing;
      if( bds.x < 0_x ) res = nothing;
    }
    return res;
  }

 private:
  UnitsOnDock( PS& S, Rect dock_anchor,
               vector<UnitWithPosition>&& units )
    : UnitCollection( S, dock_anchor, std::move( units ) ) {}
};
NOTHROW_MOVE( UnitsOnDock );

class ShipsInPort : public UnitCollection {
 public:
  ShipsInPort( ShipsInPort&& ) = default;
  ShipsInPort& operator=( ShipsInPort&& ) = default;

  static maybe<ShipsInPort> create(
      PS& S, Delta const& size,
      maybe<InPortBox> const& maybe_in_port_box ) {
    maybe<ShipsInPort> res;
    if( maybe_in_port_box ) {
      vector<UnitWithPosition> units;
      auto  in_port_bds = maybe_in_port_box->bounds();
      Coord coord = in_port_bds.lower_right() - g_tile_delta;
      for( auto id : harbor_units_in_port( S.units_state,
                                           S.player.nation ) ) {
        units.push_back( { id, coord } );
        coord -= g_tile_delta.w;
        if( coord.x < in_port_bds.left_edge() )
          coord = Coord{
              ( in_port_bds.upper_right() - g_tile_delta ).x,
              coord.y - g_tile_delta.h };
      }
      // populate units...
      res = ShipsInPort(
          S, /*bounds_when_no_units_=*/
          Rect::from( in_port_bds.lower_right(), Delta{} ),
          /*units_=*/std::move( units ) );
      auto bds = res->bounds();
      auto lr_delta =
          ( bds.lower_right() - Delta{ 1_w, 1_h } ) - Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h )
        res = nothing;
      if( bds.y < 0_y ) res = nothing;
      if( bds.x < 0_x ) res = nothing;
    }
    return res;
  }

 private:
  ShipsInPort( PS& S, Rect dock_anchor,
               vector<UnitWithPosition>&& units )
    : UnitCollection( S, dock_anchor, std::move( units ) ) {}
};
NOTHROW_MOVE( ShipsInPort );

class ShipsInbound : public UnitCollection {
 public:
  ShipsInbound( ShipsInbound&& ) = default;
  ShipsInbound& operator=( ShipsInbound&& ) = default;

  static maybe<ShipsInbound> create(
      PS& S, Delta const& size,
      maybe<InboundBox> const& maybe_inbound_box ) {
    maybe<ShipsInbound> res;
    if( maybe_inbound_box ) {
      vector<UnitWithPosition> units;
      auto  frame_bds = maybe_inbound_box->bounds();
      Coord coord     = frame_bds.lower_right() - g_tile_delta;
      for( auto id : harbor_units_inbound( S.units_state,
                                           S.player.nation ) ) {
        units.push_back( { id, coord } );
        coord -= g_tile_delta.w;
        if( coord.x < frame_bds.left_edge() )
          coord = Coord{
              ( frame_bds.upper_right() - g_tile_delta ).x,
              coord.y - g_tile_delta.h };
      }
      // populate units...
      res = ShipsInbound(
          S, /*bounds_when_no_units_=*/
          Rect::from( frame_bds.lower_right(), Delta{} ),
          /*units_=*/std::move( units ) );
      auto bds = res->bounds();
      auto lr_delta =
          ( bds.lower_right() - Delta{ 1_w, 1_h } ) - Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h )
        res = nothing;
      if( bds.y < 0_y ) res = nothing;
      if( bds.x < 0_x ) res = nothing;
    }
    return res;
  }

 private:
  ShipsInbound( PS& S, Rect dock_anchor,
                vector<UnitWithPosition>&& units )
    : UnitCollection( S, dock_anchor, std::move( units ) ) {}
};
NOTHROW_MOVE( ShipsInbound );

class ShipsOutbound : public UnitCollection {
 public:
  ShipsOutbound( ShipsOutbound&& ) = default;
  ShipsOutbound& operator=( ShipsOutbound&& ) = default;

  static maybe<ShipsOutbound> create(
      PS& S, Delta const& size,
      maybe<OutboundBox> const& maybe_outbound_box ) {
    maybe<ShipsOutbound> res;
    if( maybe_outbound_box ) {
      vector<UnitWithPosition> units;
      auto  frame_bds = maybe_outbound_box->bounds();
      Coord coord     = frame_bds.lower_right() - g_tile_delta;
      for( auto id : harbor_units_outbound( S.units_state,
                                            S.player.nation ) ) {
        units.push_back( { id, coord } );
        coord -= g_tile_delta.w;
        if( coord.x < frame_bds.left_edge() )
          coord = Coord{
              ( frame_bds.upper_right() - g_tile_delta ).x,
              coord.y - g_tile_delta.h };
      }
      // populate units...
      res = ShipsOutbound(
          S, /*bounds_when_no_units_=*/
          Rect::from( frame_bds.lower_right(), Delta{} ),
          /*units_=*/std::move( units ) );
      auto bds = res->bounds();
      auto lr_delta =
          ( bds.lower_right() - Delta{ 1_w, 1_h } ) - Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h )
        res = nothing;
      if( bds.y < 0_y ) res = nothing;
      if( bds.x < 0_x ) res = nothing;
    }
    return res;
  }

 private:
  ShipsOutbound( PS& S, Rect dock_anchor,
                 vector<UnitWithPosition>&& units )
    : UnitCollection( S, dock_anchor, std::move( units ) ) {}
};
NOTHROW_MOVE( ShipsOutbound );

class ActiveCargo : EntityBase {
 public:
  Rect bounds() const { return bounds_; }

  void draw( rr::Renderer& renderer, Delta offset ) const {
    rr::Painter painter = renderer.painter();
    auto        bds     = bounds();
    auto grid = bds.to_grid_noalign( ActiveCargoBox::box_delta );
    if( maybe_active_unit_ ) {
      auto&       unit = unit_from_id( *maybe_active_unit_ );
      auto const& cargo_slots = unit.cargo().slots();
      for( auto const& [idx, cargo_slot, rect] :
           rl::zip( rl::ints(), cargo_slots,
                    range_of_rects( grid ) ) ) {
        if( S->drag_state.has_value() ) {
          if_get( S->drag_state->object,
                  HarborDraggableObject::cargo_commodity, cc ) {
            if( cc.slot._ == idx ) continue;
          }
        }
        auto dst_coord       = rect.upper_left() + offset;
        auto cargo_slot_copy = cargo_slot;
        switch( auto& v = cargo_slot_copy; v.to_enum() ) {
          case CargoSlot::e::empty: {
            break;
          }
          case CargoSlot::e::overflow: {
            break;
          }
          case CargoSlot::e::cargo: {
            auto& cargo = v.get<CargoSlot::cargo>();
            overload_visit(
                cargo.contents,
                [&]( Cargo::unit u ) {
                  if( !S->drag_state ||
                      S->drag_state->object !=
                          HarborDraggableObject_t{
                              HarborDraggableObject::unit{
                                  u.id } } )
                    render_unit(
                        renderer, dst_coord, u.id,
                        UnitRenderOptions{ .flag = false } );
                },
                [&]( Cargo::commodity const& c ) {
                  render_commodity_annotated(
                      renderer,
                      dst_coord +
                          kCommodityInCargoHoldRenderingOffset,
                      c.obj );
                } );
            break;
          }
        }
      }
      for( auto [idx, rect] :
           rl::zip( rl::ints(), range_of_rects( grid ) ) ) {
        if( idx >= unit.cargo().slots_total() )
          painter.draw_solid_rect( rect.shifted_by( offset ),
                                   gfx::pixel::white() );
      }
    } else {
      for( auto rect : range_of_rects( grid ) )
        painter.draw_solid_rect( rect.shifted_by( offset ),
                                 gfx::pixel::white() );
    }
  }

  ActiveCargo( ActiveCargo&& ) = default;
  ActiveCargo& operator=( ActiveCargo&& ) = default;

  static maybe<ActiveCargo> create(
      PS& S, Delta const& size,
      maybe<ActiveCargoBox> const& maybe_active_cargo_box,
      maybe<ShipsInPort> const&    maybe_ships_in_port ) {
    HarborState const& hb_state = S.harbor_state();
    maybe<ActiveCargo> res;
    if( maybe_active_cargo_box && maybe_ships_in_port ) {
      res = ActiveCargo(
          S,
          /*maybe_active_unit_=*/hb_state.selected_unit,
          /*bounds_=*/maybe_active_cargo_box->bounds() );
      // FIXME: if we are inside the active cargo box, and the
      // active cargo box exists, do we need the following
      // checks?
      auto lr_delta =
          ( res->bounds().lower_right() - Delta{ 1_w, 1_h } ) -
          Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h )
        res = nothing;
      if( res->bounds().y < 0_y ) res = nothing;
      if( res->bounds().x < 0_x ) res = nothing;
    }
    return res;
  }

  maybe<pair<CargoSlotIndex, Rect>> obj_under_cursor(
      Coord const& coord ) const {
    maybe<pair<CargoSlotIndex, Rect>> res;
    if( maybe_active_unit_ ) {
      if( coord.is_inside( bounds_ ) ) {
        auto boxes = bounds_.with_new_upper_left( Coord{} ) /
                     ActiveCargoBox::box_scale;
        auto maybe_slot = boxes.rasterize(
            coord.with_new_origin( bounds_.upper_left() ) /
            ActiveCargoBox::box_scale );
        if( maybe_slot ) {
          auto box_origin =
              coord.with_new_origin( bounds().upper_left() )
                  .rounded_to_multiple_to_minus_inf(
                      ActiveCargoBox::box_scale )
                  .as_if_origin_were( bounds().upper_left() );
          auto scale = ActiveCargoBox::box_scale;

          using HarborDraggableObject::cargo_commodity;
          if( draggable_in_cargo_slot( *S, *maybe_slot )
                  .bind( L( holds<cargo_commodity>( _ ) ) ) ) {
            box_origin += kCommodityInCargoHoldRenderingOffset;
            scale = Scale{ 16 };
          }
          auto box = Rect::from( box_origin,
                                 Delta{ 1_w, 1_h } * scale );

          res = pair{ *maybe_slot, box };
        }
      }
      auto& unit = unit_from_id( *maybe_active_unit_ );
      if( res && res->first >= unit.cargo().slots_total() )
        res = nothing;
    }
    return res;
  }

  // maybe<CargoSlot_t const&>> cargo_slot_from_coord(
  //    Coord coord ) const {
  //  // Lambda will only be called if a valid index is returned,
  //  // in which case there is guaranteed to be an active unit.
  //  return slot_idx_from_coord( coord ) //
  //         .fmap( LC( unit_from_id( *maybe_active_unit_ )
  //                         .cargo()[_] ) );
  //}

  maybe<UnitId> active_unit() const {
    return maybe_active_unit_;
  }

 private:
  ActiveCargo( PS& S, maybe<UnitId> maybe_active_unit,
               Rect bounds )
    : EntityBase( S ),
      maybe_active_unit_( maybe_active_unit ),
      bounds_( bounds ) {}
  maybe<UnitId> maybe_active_unit_;
  Rect          bounds_;
};
NOTHROW_MOVE( ActiveCargo );

} // namespace entity

//- Buttons
//- Message box
//- Stats area (money, tax rate, etc.)

struct Entities {
  maybe<entity::MarketCommodities> market_commodities;
  maybe<entity::ActiveCargoBox>    active_cargo_box;
  maybe<entity::DockAnchor>        dock_anchor;
  maybe<entity::Backdrop>          backdrop;
  maybe<entity::InPortBox>         in_port_box;
  maybe<entity::InboundBox>        inbound_box;
  maybe<entity::OutboundBox>       outbound_box;
  maybe<entity::Exit>              exit_label;
  maybe<entity::Dock>              dock;
  maybe<entity::UnitsOnDock>       units_on_dock;
  maybe<entity::ShipsInPort>       ships_in_port;
  maybe<entity::ShipsInbound>      ships_inbound;
  maybe<entity::ShipsOutbound>     ships_outbound;
  maybe<entity::ActiveCargo>       active_cargo;
};
NOTHROW_MOVE( Entities );

void create_entities( PS& S, Entities* entities ) {
  using namespace entity;
  UNWRAP_CHECK(
      normal_area,
      compositor::section( compositor::e_section::normal ) );
  Delta clip                   = normal_area.delta();
  entities->market_commodities = //
      MarketCommodities::create( S, clip );
  entities->active_cargo_box =         //
      ActiveCargoBox::create( S, clip, //
                              entities->market_commodities );
  entities->dock_anchor =                             //
      DockAnchor::create( S, clip,                    //
                          entities->active_cargo_box, //
                          entities->market_commodities );
  entities->backdrop =           //
      Backdrop::create( S, clip, //
                        entities->dock_anchor );
  entities->in_port_box =                            //
      InPortBox::create( S, clip,                    //
                         entities->active_cargo_box, //
                         entities->market_commodities );
  entities->inbound_box =          //
      InboundBox::create( S, clip, //
                          entities->in_port_box );
  entities->outbound_box =          //
      OutboundBox::create( S, clip, //
                           entities->inbound_box );
  entities->exit_label =     //
      Exit::create( S, clip, //
                    entities->market_commodities );
  entities->dock =                         //
      Dock::create( S, clip,               //
                    entities->dock_anchor, //
                    entities->in_port_box );
  entities->units_on_dock =                       //
      UnitsOnDock::create( S, clip,               //
                           entities->dock_anchor, //
                           entities->dock );
  entities->ships_in_port =         //
      ShipsInPort::create( S, clip, //
                           entities->in_port_box );
  entities->ships_inbound =          //
      ShipsInbound::create( S, clip, //
                            entities->inbound_box );
  entities->ships_outbound =          //
      ShipsOutbound::create( S, clip, //
                             entities->outbound_box );
  entities->active_cargo =                             //
      ActiveCargo::create( S, clip,                    //
                           entities->active_cargo_box, //
                           entities->ships_in_port );
}

void draw_entities( rr::Renderer&   renderer,
                    Entities const& entities ) {
  UNWRAP_CHECK(
      normal_area,
      compositor::section( compositor::e_section::normal ) );
  auto offset = normal_area.upper_left().distance_from_origin();
  if( entities.backdrop.has_value() )
    entities.backdrop->draw( renderer, offset );
  if( entities.market_commodities.has_value() )
    entities.market_commodities->draw( renderer, offset );
  if( entities.active_cargo_box.has_value() )
    entities.active_cargo_box->draw( renderer, offset );
  if( entities.dock_anchor.has_value() )
    entities.dock_anchor->draw( renderer, offset );
  if( entities.in_port_box.has_value() )
    entities.in_port_box->draw( renderer, offset );
  if( entities.inbound_box.has_value() )
    entities.inbound_box->draw( renderer, offset );
  if( entities.outbound_box.has_value() )
    entities.outbound_box->draw( renderer, offset );
  if( entities.exit_label.has_value() )
    entities.exit_label->draw( renderer, offset );
  if( entities.dock.has_value() )
    entities.dock->draw( renderer, offset );
  if( entities.units_on_dock.has_value() )
    entities.units_on_dock->draw( renderer, offset );
  if( entities.ships_in_port.has_value() )
    entities.ships_in_port->draw( renderer, offset );
  if( entities.ships_inbound.has_value() )
    entities.ships_inbound->draw( renderer, offset );
  if( entities.ships_outbound.has_value() )
    entities.ships_outbound->draw( renderer, offset );
  if( entities.active_cargo.has_value() )
    entities.active_cargo->draw( renderer, offset );
}

/****************************************************************
** Drag & Drop
*****************************************************************/
struct HarborDragSrcInfo {
  HarborDragSrc_t src;
  Rect            rect;
};

maybe<HarborDragSrcInfo> drag_src_from_coord(
    PS& S, Coord const& coord, Entities const* entities ) {
  using namespace entity;
  maybe<HarborDragSrcInfo> res;
  if( entities->units_on_dock.has_value() ) {
    if( auto maybe_pair =
            entities->units_on_dock->obj_under_cursor( coord );
        maybe_pair ) {
      auto const& [id, rect] = *maybe_pair;
      res                    = HarborDragSrcInfo{
          /*src=*/HarborDragSrc::dock{ /*id=*/id },
          /*rect=*/rect };
    }
  }
  if( entities->active_cargo.has_value() ) {
    auto const& active_cargo = *entities->active_cargo;
    auto        in_port =
        active_cargo.active_unit()
            .fmap( [&]( UnitId id ) {
              return is_unit_in_port( S.units_state, id );
            } )
            .is_value_truish();
    auto maybe_pair = base::just( coord ).bind(
        LC( active_cargo.obj_under_cursor( _ ) ) );
    if( in_port &&
        maybe_pair.fmap( L( _.first ) )
            .bind( LC( draggable_in_cargo_slot( S, _ ) ) ) ) {
      auto const& [slot, rect] = *maybe_pair;

      res = HarborDragSrcInfo{
          /*src=*/HarborDragSrc::cargo{ /*slot=*/slot,
                                        /*quantity=*/nothing },
          /*rect=*/rect };
    }
  }
  if( entities->ships_outbound.has_value() ) {
    if( auto maybe_pair =
            entities->ships_outbound->obj_under_cursor( coord );
        maybe_pair ) {
      auto const& [id, rect] = *maybe_pair;

      res = HarborDragSrcInfo{
          /*src=*/HarborDragSrc::outbound{ /*id=*/id },
          /*rect=*/rect };
    }
  }
  if( entities->ships_inbound.has_value() ) {
    if( auto maybe_pair =
            entities->ships_inbound->obj_under_cursor( coord );
        maybe_pair ) {
      auto const& [id, rect] = *maybe_pair;

      res = HarborDragSrcInfo{
          /*src=*/HarborDragSrc::inbound{ /*id=*/id },
          /*rect=*/rect };
    }
  }
  if( entities->ships_in_port.has_value() ) {
    if( auto maybe_pair =
            entities->ships_in_port->obj_under_cursor( coord );
        maybe_pair ) {
      auto const& [id, rect] = *maybe_pair;

      res = HarborDragSrcInfo{
          /*src=*/HarborDragSrc::inport{ /*id=*/id },
          /*rect=*/rect };
    }
  }
  if( entities->market_commodities.has_value() ) {
    if( auto maybe_pair =
            entities->market_commodities->obj_under_cursor(
                coord );
        maybe_pair ) {
      auto const& [type, rect] = *maybe_pair;

      res = HarborDragSrcInfo{
          /*src=*/HarborDragSrc::market{ /*type=*/type,
                                         /*quantity=*/nothing },
          /*rect=*/rect };
    }
  }

  return res;
}

// Important: in this function we should not return early; we
// should check all the entities (in order) to allow later ones
// to override earlier ones.
maybe<HarborDragDst_t> drag_dst_from_coord(
    Entities const* entities, Coord const& coord ) {
  using namespace entity;
  maybe<HarborDragDst_t> res;
  if( entities->active_cargo.has_value() ) {
    if( auto maybe_pair =
            entities->active_cargo->obj_under_cursor( coord );
        maybe_pair ) {
      auto const& slot = maybe_pair->first;

      res = HarborDragDst::cargo{
          /*slot=*/slot //
      };
    }
  }
  if( entities->dock.has_value() ) {
    if( coord.is_inside( entities->dock->bounds() ) )
      res = HarborDragDst::dock{};
  }
  if( entities->units_on_dock.has_value() ) {
    if( coord.is_inside( entities->units_on_dock->bounds() ) )
      res = HarborDragDst::dock{};
  }
  if( entities->outbound_box.has_value() ) {
    if( coord.is_inside( entities->outbound_box->bounds() ) )
      res = HarborDragDst::outbound{};
  }
  if( entities->inbound_box.has_value() ) {
    if( coord.is_inside( entities->inbound_box->bounds() ) )
      res = HarborDragDst::inbound{};
  }
  if( entities->in_port_box.has_value() ) {
    if( coord.is_inside( entities->in_port_box->bounds() ) )
      res = HarborDragDst::inport{};
  }
  if( entities->ships_in_port.has_value() ) {
    if( auto maybe_pair =
            entities->ships_in_port->obj_under_cursor( coord );
        maybe_pair ) {
      auto const& ship = maybe_pair->first;

      res = HarborDragDst::inport_ship{
          /*id=*/ship, //
      };
    }
  }
  if( entities->market_commodities.has_value() ) {
    if( coord.is_inside(
            entities->market_commodities->bounds() ) )
      return HarborDragDst::market{};
  }
  return res;
}

maybe<UnitId> active_cargo_ship( Entities const* entities ) {
  return entities->active_cargo.bind(
      &entity::ActiveCargo::active_unit );
}

HarborDraggableObject_t draggable_from_src(
    PS& S, HarborDragSrc_t const& drag_src ) {
  using namespace HarborDragSrc;
  return overload_visit<HarborDraggableObject_t>(
      drag_src,
      [&]( dock const& o ) {
        return HarborDraggableObject::unit{ o.id };
      },
      [&]( cargo const& o ) {
        // Not all cargo slots must have an item in them, but in
        // this case the slot should otherwise the
        // HarborDragSrc object should never have been created.
        UNWRAP_CHECK( object,
                      draggable_in_cargo_slot( S, o.slot ) );
        return object;
      },
      [&]( outbound const& o ) {
        return HarborDraggableObject::unit{ o.id };
      },
      [&]( inbound const& o ) {
        return HarborDraggableObject::unit{ o.id };
      },
      [&]( inport const& o ) {
        return HarborDraggableObject::unit{ o.id };
      },
      [&]( market const& o ) {
        return HarborDraggableObject::market_commodity{ o.type };
      } );
}

#define DRAG_CONNECT_CASE( src_, dst_ )                        \
  operator()( [[maybe_unused]] HarborDragSrc::src_ const& src, \
              [[maybe_unused]] HarborDragDst::dst_ const& dst )

struct DragConnector {
  PS& S;

  static bool visit( PS& S, Entities const* entities,
                     HarborDragSrc_t const& drag_src,
                     HarborDragDst_t const& drag_dst ) {
    DragConnector connector( S, entities );
    return std::visit( connector, drag_src, drag_dst );
  }

  Entities const* entities = nullptr;
  DragConnector( PS& S_arg, Entities const* entities_ )
    : S( S_arg ), entities( entities_ ) {}
  bool DRAG_CONNECT_CASE( dock, cargo ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    if( !is_unit_in_port( S.units_state, ship ) ) return false;
    return unit_from_id( ship ).cargo().fits_somewhere(
        GameState::units(), Cargo::unit{ src.id }, dst.slot._ );
  }
  bool DRAG_CONNECT_CASE( cargo, dock ) const {
    return holds<HarborDraggableObject::unit>(
               draggable_from_src( S, src ) )
        .has_value();
  }
  bool DRAG_CONNECT_CASE( cargo, cargo ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    if( !is_unit_in_port( S.units_state, ship ) ) return false;
    if( src.slot == dst.slot ) return true;
    UNWRAP_CHECK( cargo_object,
                  draggable_to_cargo_object(
                      draggable_from_src( S, src ) ) );
    return overload_visit(
        cargo_object,
        [&]( Cargo::unit ) {
          return unit_from_id( ship )
              .cargo()
              .fits_with_item_removed(
                  GameState::units(),
                  /*cargo=*/cargo_object,   //
                  /*remove_slot=*/src.slot, //
                  /*insert_slot=*/dst.slot  //
              );
        },
        [&]( Cargo::commodity const& c ) {
          // If at least one quantity of the commodity can be
          // moved then we will allow (at least a partial
          // transfer) to proceed.
          auto size_one     = c.obj;
          size_one.quantity = 1;
          return unit_from_id( ship ).cargo().fits(
              GameState::units(),
              /*cargo=*/Cargo::commodity{ size_one },
              /*slot=*/dst.slot );
        } );
  }
  bool DRAG_CONNECT_CASE( outbound, inbound ) const {
    return true;
  }
  bool DRAG_CONNECT_CASE( outbound, inport ) const {
    UnitsState const& units_state = S.units_state;
    UNWRAP_CHECK(
        info, units_state.maybe_harbor_view_state_of( src.id ) );
    ASSIGN_CHECK_V( outbound, info.port_status,
                    PortStatus::outbound );
    // We'd like to do == 0.0 here, but this will avoid rounding
    // errors.
    return outbound.turns == 0;
  }
  bool DRAG_CONNECT_CASE( inbound, outbound ) const {
    return true;
  }
  bool DRAG_CONNECT_CASE( inport, outbound ) const {
    return true;
  }
  bool DRAG_CONNECT_CASE( dock, inport_ship ) const {
    return unit_from_id( dst.id ).cargo().fits_somewhere(
        GameState::units(), Cargo::unit{ src.id } );
  }
  bool DRAG_CONNECT_CASE( cargo, inport_ship ) const {
    auto dst_ship = dst.id;
    UNWRAP_CHECK( cargo_object,
                  draggable_to_cargo_object(
                      draggable_from_src( S, src ) ) );
    return overload_visit(
        cargo_object,
        [&]( Cargo::unit u ) {
          if( is_unit_onboard( u.id ) == dst_ship ) return false;
          return unit_from_id( dst_ship )
              .cargo()
              .fits_somewhere( GameState::units(), u );
        },
        [&]( Cargo::commodity const& c ) {
          // If even 1 quantity can fit then we can proceed
          // with (at least) a partial transfer.
          auto size_one     = c.obj;
          size_one.quantity = 1;
          return unit_from_id( dst_ship )
              .cargo()
              .fits_somewhere( GameState::units(),
                               Cargo::commodity{ size_one } );
        } );
  }
  bool DRAG_CONNECT_CASE( market, cargo ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    if( !is_unit_in_port( S.units_state, ship ) ) return false;
    auto comm = Commodity{
        /*type=*/src.type, //
        // If the commodity can fit even with just one quan-
        // tity then it is allowed, since we will just insert
        // as much as possible if we can't insert 100.
        /*quantity=*/1 //
    };
    return unit_from_id( ship ).cargo().fits_somewhere(
        GameState::units(), Cargo::commodity{ comm },
        dst.slot._ );
  }
  bool DRAG_CONNECT_CASE( market, inport_ship ) const {
    auto comm = Commodity{
        /*type=*/src.type, //
        // If the commodity can fit even with just one quan-
        // tity then it is allowed, since we will just insert
        // as much as possible if we can't insert 100.
        /*quantity=*/1 //
    };
    return unit_from_id( dst.id ).cargo().fits_somewhere(
        GameState::units(), Cargo::commodity{ comm },
        /*starting_slot=*/0 );
  }
  bool DRAG_CONNECT_CASE( cargo, market ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    if( !is_unit_in_port( S.units_state, ship ) ) return false;
    return unit_from_id( ship )
        .cargo()
        .template slot_holds_cargo_type<Cargo::commodity>(
            src.slot._ )
        .has_value();
  }
  bool operator()( auto const&, auto const& ) const {
    return false;
  }
};

#define DRAG_CONFIRM_CASE( src_, dst_ )                  \
  operator()( [[maybe_unused]] HarborDragSrc::src_& src, \
              [[maybe_unused]] HarborDragDst::dst_& dst )

struct DragUserInput {
  PS&             S;
  Entities const* entities = nullptr;
  DragUserInput( PS& S_arg, Entities const* entities_ )
    : S( S_arg ), entities( entities_ ) {}

  static wait<base::NoDiscard<bool>> visit(
      PS& S, Entities const* entities, HarborDragSrc_t* drag_src,
      HarborDragDst_t* drag_dst ) {
    // Need to co_await here to keep parameters alive.
    bool proceed = co_await std::visit(
        DragUserInput( S, entities ), *drag_src, *drag_dst );
    co_return proceed;
  }

  static wait<maybe<int>> ask_for_quantity( PS&         S,
                                            e_commodity type,
                                            string_view verb ) {
    string text = fmt::format(
        "What quantity of @[H]{}@[] would you like to {}? "
        "(0-100):",
        commodity_display_name( type ), verb );

    // FIXME: allow the user to cancel by hitting escape.
    //
    // FIXME: add proper initial value.
    int res = co_await S.gui.int_input( { .msg           = text,
                                          .initial_value = 0,
                                          .min           = 0,
                                          .max = 100 } );
    co_return res;
  }

  wait<bool> DRAG_CONFIRM_CASE( market, cargo ) const {
    src.quantity =
        co_await ask_for_quantity( S, src.type, "buy" );
    co_return src.quantity.has_value();
  }
  wait<bool> DRAG_CONFIRM_CASE( market, inport_ship ) const {
    src.quantity =
        co_await ask_for_quantity( S, src.type, "buy" );
    co_return src.quantity.has_value();
  }
  wait<bool> DRAG_CONFIRM_CASE( cargo, market ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    CHECK( is_unit_in_port( S.units_state, ship ) );
    UNWRAP_CHECK(
        commodity_ref,
        unit_from_id( ship )
            .cargo()
            .template slot_holds_cargo_type<Cargo::commodity>(
                src.slot._ ) );
    src.quantity = co_await ask_for_quantity(
        S, commodity_ref.obj.type, "sell" );
    co_return src.quantity.has_value();
  }
  wait<bool> DRAG_CONFIRM_CASE( cargo, inport_ship ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    CHECK( is_unit_in_port( S.units_state, ship ) );
    auto maybe_commodity_ref =
        unit_from_id( ship )
            .cargo()
            .template slot_holds_cargo_type<Cargo::commodity>(
                src.slot._ );
    if( !maybe_commodity_ref.has_value() )
      // It's a unit.
      co_return true;
    src.quantity = co_await ask_for_quantity(
        S, maybe_commodity_ref->obj.type, "move" );
    co_return src.quantity.has_value();
  }
  wait<bool> operator()( auto const&, auto const& ) const {
    co_return true;
  }
};

#define DRAG_PERFORM_CASE( src_, dst_ )                        \
  operator()( [[maybe_unused]] HarborDragSrc::src_ const& src, \
              [[maybe_unused]] HarborDragDst::dst_ const& dst )

struct DragPerform {
  PS& S;

  Entities const* entities = nullptr;
  DragPerform( PS& S_arg, Entities const* entities_ )
    : S( S_arg ), entities( entities_ ) {}

  static void visit( PS& S, Entities const* entities,
                     HarborDragSrc_t const& src,
                     HarborDragDst_t const& dst ) {
    lg.debug( "performing drag: {} to {}", src, dst );
    std::visit( DragPerform( S, entities ), src, dst );
  }

  void DRAG_PERFORM_CASE( dock, cargo ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    // First try to respect the destination slot chosen by
    // the player,
    if( unit_from_id( ship ).cargo().fits( GameState::units(),
                                           Cargo::unit{ src.id },
                                           dst.slot._ ) )
      S.units_state.change_to_cargo_somewhere( ship, src.id,
                                               dst.slot._ );
    else
      S.units_state.change_to_cargo_somewhere( ship, src.id );
  }
  void DRAG_PERFORM_CASE( cargo, dock ) const {
    ASSIGN_CHECK_V( unit, draggable_from_src( S, src ),
                    HarborDraggableObject::unit );
    unit_move_to_port( S.units_state, unit.id );
  }
  void DRAG_PERFORM_CASE( cargo, cargo ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    UNWRAP_CHECK( cargo_object,
                  draggable_to_cargo_object(
                      draggable_from_src( S, src ) ) );
    overload_visit(
        cargo_object,
        [&]( Cargo::unit u ) {
          // Will first "disown" unit which will remove it
          // from the cargo.
          S.units_state.change_to_cargo_somewhere( ship, u.id,
                                                   dst.slot._ );
        },
        [&]( Cargo::commodity const& ) {
          move_commodity_as_much_as_possible(
              GameState::units(), ship, src.slot._, ship,
              dst.slot._,
              /*max_quantity=*/nothing,
              /*try_other_dst_slots=*/false );
        } );
  }
  void DRAG_PERFORM_CASE( outbound, inbound ) const {
    unit_sail_to_harbor( S.terrain_state, S.units_state,
                         S.player, src.id );
  }
  void DRAG_PERFORM_CASE( outbound, inport ) const {
    unit_sail_to_harbor( S.terrain_state, S.units_state,
                         S.player, src.id );
  }
  void DRAG_PERFORM_CASE( inbound, outbound ) const {
    unit_sail_to_new_world( S.terrain_state, S.units_state,
                            S.player, src.id );
  }
  void DRAG_PERFORM_CASE( inport, outbound ) const {
    HarborState& hb_state = S.harbor_state();
    unit_sail_to_new_world( S.terrain_state, S.units_state,
                            S.player, src.id );
    // This is not strictly necessary, but for a nice user expe-
    // rience we will auto-select another unit that is in-port
    // (if any) since that is likely what the user wants to work
    // with, as opposed to keeping the selection on the unit that
    // is now outbound. Or if there are no more units in port,
    // just deselect.
    hb_state.selected_unit = nothing;
    vector<UnitId> units_in_port =
        harbor_units_in_port( S.units_state, S.player.nation );
    hb_state.selected_unit = rl::all( units_in_port ).head();
  }
  void DRAG_PERFORM_CASE( dock, inport_ship ) const {
    S.units_state.change_to_cargo_somewhere( dst.id, src.id );
  }
  void DRAG_PERFORM_CASE( cargo, inport_ship ) const {
    UNWRAP_CHECK( cargo_object,
                  draggable_to_cargo_object(
                      draggable_from_src( S, src ) ) );
    overload_visit(
        cargo_object,
        [&]( Cargo::unit u ) {
          CHECK( !src.quantity.has_value() );
          // Will first "disown" unit which will remove it
          // from the cargo.
          S.units_state.change_to_cargo_somewhere( dst.id,
                                                   u.id );
        },
        [&]( Cargo::commodity const& ) {
          UNWRAP_CHECK( src_ship,
                        active_cargo_ship( entities ) );
          move_commodity_as_much_as_possible(
              GameState::units(), src_ship, src.slot._,
              /*dst_ship=*/dst.id,
              /*dst_slot=*/0,
              /*max_quantity=*/src.quantity,
              /*try_other_dst_slots=*/true );
        } );
  }
  void DRAG_PERFORM_CASE( market, cargo ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    auto comm = Commodity{
        /*type=*/src.type, //
        /*quantity=*/src.quantity.value_or(
            k_default_market_quantity ) //
    };
    comm.quantity = std::min(
        comm.quantity,
        unit_from_id( ship )
            .cargo()
            .max_commodity_quantity_that_fits( src.type ) );
    // Cap it.
    comm.quantity =
        std::min( comm.quantity, k_default_market_quantity );
    CHECK( comm.quantity > 0 );
    add_commodity_to_cargo( GameState::units(), comm, ship,
                            /*slot=*/dst.slot._,
                            /*try_other_slots=*/true );
  }
  void DRAG_PERFORM_CASE( market, inport_ship ) const {
    auto comm = Commodity{
        /*type=*/src.type, //
        /*quantity=*/src.quantity.value_or(
            k_default_market_quantity ) //
    };
    comm.quantity = std::min(
        comm.quantity,
        unit_from_id( dst.id )
            .cargo()
            .max_commodity_quantity_that_fits( src.type ) );
    // Cap it.
    comm.quantity =
        std::min( comm.quantity, k_default_market_quantity );
    CHECK( comm.quantity > 0 );
    add_commodity_to_cargo( GameState::units(), comm, dst.id,
                            /*slot=*/0,
                            /*try_other_slots=*/true );
  }
  void DRAG_PERFORM_CASE( cargo, market ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    UNWRAP_CHECK(
        commodity_ref,
        unit_from_id( ship )
            .cargo()
            .template slot_holds_cargo_type<Cargo::commodity>(
                src.slot._ ) );
    auto quantity_wants_to_sell =
        src.quantity.value_or( commodity_ref.obj.quantity );
    int       amount_to_sell = std::min( quantity_wants_to_sell,
                                         commodity_ref.obj.quantity );
    Commodity new_comm       = commodity_ref.obj;
    new_comm.quantity -= amount_to_sell;
    rm_commodity_from_cargo( GameState::units(), ship,
                             src.slot._ );
    if( new_comm.quantity > 0 )
      add_commodity_to_cargo( GameState::units(), new_comm, ship,
                              /*slot=*/src.slot._,
                              /*try_other_slots=*/false );
  }
  void operator()( auto const&, auto const& ) const {
    SHOULD_NOT_BE_HERE;
  }
};

void drag_n_drop_draw( PS const& S, rr::Renderer& renderer,
                       Rect const& canvas ) {
  if( !S.drag_state ) return;
  auto& state            = *S.drag_state;
  auto  to_screen_coords = [&]( Coord const& c ) {
    return c + canvas.upper_left().distance_from_origin();
  };
  auto origin_for = [&]( Delta const& tile_size ) {
    return to_screen_coords( state.where ) -
           tile_size / Scale{ 2 } - state.click_offset;
  };
  using namespace HarborDraggableObject;
  // Render the dragged item.
  overload_visit(
      state.object,
      [&]( unit const& o ) {
        auto size =
            sprite_size( unit_from_id( o.id ).desc().tile );
        render_unit( renderer, origin_for( size ), o.id,
                     UnitRenderOptions{ .flag = false } );
      },
      [&]( market_commodity const& o ) {
        auto size = commodity_tile_size( o.type );
        render_commodity( renderer, origin_for( size ), o.type );
      },
      [&]( cargo_commodity const& o ) {
        auto size = commodity_tile_size( o.comm.type );
        render_commodity( renderer, origin_for( size ),
                          o.comm.type );
      } );
  // Render any indicators on top of it.
  switch( state.indicator ) {
    using e = drag::e_status_indicator;
    case e::none: break;
    case e::bad: {
      Delta char_size = Delta::from_gfx(
          rr::rendered_text_line_size_pixels( "X" ) );
      rr::Typer typer = renderer.typer( origin_for( char_size ),
                                        gfx::pixel::red() );
      typer.write( "X" );
      break;
    }
    case e::good: {
      Delta char_size = Delta::from_gfx(
          rr::rendered_text_line_size_pixels( "+" ) );
      rr::Typer typer = renderer.typer( origin_for( char_size ),
                                        gfx::pixel::green() );
      typer.write( "+" );
      if( state.user_requests_input ) {
        auto mod_pos = to_screen_coords( state.where );
        mod_pos.y -= char_size.h;
        rr::Typer typer_mod = renderer.typer(
            mod_pos - state.click_offset, gfx::pixel::green() );
        typer_mod.write( "?" );
      }
      break;
    }
  }
}

void drag_n_drop_handle_input(
    input::event_t const&          event,
    co::finite_stream<drag::Step>& drag_stream ) {
  auto key_event = event.get_if<input::key_event_t>();
  if( !key_event ) return;
  if( key_event->keycode != ::SDLK_LSHIFT &&
      key_event->keycode != ::SDLK_RSHIFT )
    return;
  // This input event is a shift key being pressed or released.
  drag_stream.send(
      drag::Step{ .mod     = key_event->mod,
                  .current = input::current_mouse_position() } );
}

// This function takes a coordinate relative to the canvas and
// works in that frame.
wait<> dragging_thread( PS& S, Entities* entities,
                        input::e_mouse_button button,
                        Coord                 origin ) {
  // Must check first if there is anything to drag. If this is
  // not the start of a valid drag then we must return immedi-
  // ately without co_awaiting on anything.
  if( button != input::e_mouse_button::l ) co_return;
  maybe<HarborDragSrcInfo> src_info =
      drag_src_from_coord( S, origin, entities );
  if( !src_info ) co_return;
  HarborDragSrc_t& src = src_info->src;

  // Now we have a valid drag that has initiated.
  S.drag_state = drag::State<HarborDraggableObject_t>{
      .stream              = {},
      .object              = draggable_from_src( S, src ),
      .indicator           = drag::e_status_indicator::none,
      .user_requests_input = false,
      .where               = origin,
      .click_offset        = origin - src_info->rect.center() };
  SCOPE_EXIT( S.drag_state = nothing );
  auto& state = *S.drag_state;

  drag::Step             latest;
  maybe<HarborDragDst_t> dst;
  while( maybe<drag::Step> d = co_await state.stream.next() ) {
    latest      = *d;
    state.where = d->current;
    dst         = drag_dst_from_coord( entities, d->current );
    if( !dst ) {
      state.indicator = drag::e_status_indicator::none;
      continue;
    }
    if( !DragConnector::visit( S, entities, src, *dst ) ) {
      state.indicator = drag::e_status_indicator::bad;
      continue;
    }
    state.user_requests_input = d->mod.l_shf_down;
    state.indicator           = drag::e_status_indicator::good;
  }

  if( state.indicator == drag::e_status_indicator::good ) {
    CHECK( dst );
    bool proceed = true;
    if( state.user_requests_input )
      proceed = co_await DragUserInput::visit( S, entities, &src,
                                               &*dst );
    if( proceed ) {
      DragPerform::visit( S, entities, src, *dst );
      // Now that we've potentially changed the ownership of some
      // units, we need to recreate the entities otherwise we'll
      // potentially have one frame where the dragged unit is
      // back in its original position before moving to where it
      // was dragged.
      create_entities( S, entities );
      co_return;
    }
  }

  // Rubber-band back to starting point.
  state.indicator           = drag::e_status_indicator::none;
  state.user_requests_input = false;

  Coord         start   = state.where;
  Coord         end     = origin;
  double        percent = 0.0;
  AnimThrottler throttle( kAlmostStandardFrame );
  while( percent <= 1.0 ) {
    co_await throttle();
    state.where =
        start + ( end - start ).multiply_and_round( percent );
    percent += 0.15;
  }
}

} // namespace

/****************************************************************
** The Harbor Plane
*****************************************************************/
struct HarborPlane::Impl : public Plane {
  PS S_;

  Impl( Player& player, UnitsState& units_state,
        TerrainState const& terrain_state, IGui& gui )
    : S_{ .player        = player,
          .units_state   = units_state,
          .terrain_state = terrain_state,
          .gui           = gui } {}

  bool covers_screen() const override { return true; }

  // FIXME: find a better way to do this. One idea is that when
  // the compositor changes the layout it will inject a window
  // resize event into the input queue that will then be automat-
  // ically picked up by all of the planes.
  void advance_state() override {
    UNWRAP_CHECK(
        new_canvas,
        compositor::section( compositor::e_section::normal ) );
    if( new_canvas != canvas_ ) {
      canvas_ = new_canvas;
      // FIXME: this may cause a crash if it is done while a
      // coroutine is running that is holding references to the
      // existing entities, as may be the case during a drag.
      // This issue is solved in the colony-view plane where
      // input events are sent into a co::stream; see that module
      // for a proper solution. That will require that this plane
      // be rewritten to send its inputs into the co::stream as
      // the colony-view plane does.
      create_entities( S_, &entities_ );
    }
  }

  void draw( rr::Renderer& renderer ) const override {
    draw_entities( renderer, entities_ );
    // Should be last.
    drag_n_drop_draw( S_, renderer, canvas_ );
  }

  e_input_handled input(
      input::event_t const& event_untranslated ) override {
    input::event_t event = move_mouse_origin_by(
        event_untranslated,
        canvas_.upper_left().distance_from_origin() );
    // If there is a drag happening then the user's input should
    // not be needed for anything other than the drag.
    if( S_.drag_state ) {
      drag_n_drop_handle_input( event, S_.drag_state->stream );
      return e_input_handled::yes;
    }
    switch( event.to_enum() ) {
      case input::e_input_event::unknown_event:
        return e_input_handled::no;
      case input::e_input_event::quit_event:
        return e_input_handled::no;
      case input::e_input_event::win_event:
        create_entities( S_, &entities_ );
        // Generally we should return no here because this is an
        // event that we want all planes to see.
        return e_input_handled::no;
      case input::e_input_event::key_event:
        return e_input_handled::no;
      case input::e_input_event::mouse_wheel_event:
        return e_input_handled::no;
      case input::e_input_event::mouse_move_event:
        return e_input_handled::yes;
      case input::e_input_event::mouse_button_event: {
        auto& val = event.get<input::mouse_button_event_t>();
        if( val.buttons !=
            input::e_mouse_button_event::left_down )
          return e_input_handled::yes;

        // Exit button.
        if( entities_.exit_label.has_value() ) {
          if( val.pos.is_inside(
                  entities_.exit_label->bounds() ) ) {
            S_.exit_promise.set_value_emplace();
            return e_input_handled::yes;
          }
        }

        // Unit selection.
        auto handled         = e_input_handled::no;
        auto try_select_unit = [&]( auto const& maybe_entity ) {
          HarborState& hb_state = S_.harbor_state();
          if( maybe_entity ) {
            if( auto maybe_pair =
                    maybe_entity->obj_under_cursor( val.pos );
                maybe_pair ) {
              hb_state.selected_unit = maybe_pair->first;
              handled                = e_input_handled::yes;
              create_entities( S_, &entities_ );
            }
          }
        };
        try_select_unit( entities_.ships_in_port );
        try_select_unit( entities_.ships_inbound );
        try_select_unit( entities_.ships_outbound );
        return handled;
      }
      case input::e_input_event::mouse_drag_event:
        return e_input_handled::no;
    }
    UNREACHABLE_LOCATION;
  }

  /**************************************************************
  ** Dragging
  ***************************************************************/
  Plane::e_accept_drag can_drag( input::e_mouse_button button,
                                 Coord origin ) override {
    if( S_.drag_state ) return Plane::e_accept_drag::swallow;
    wait<> w = dragging_thread(
        S_, &entities_, button,
        origin.with_new_origin( canvas_.upper_left() ) );
    if( w.ready() ) return e_accept_drag::no;
    S_.drag_thread = std::move( w );
    return e_accept_drag::yes;
  }

  void on_drag( input::mod_keys const& mod,
                input::e_mouse_button /*button*/,
                Coord /*origin*/, Coord /*prev*/,
                Coord current ) override {
    CHECK( S_.drag_state );
    S_.drag_state->stream.send( drag::Step{
        .mod = mod,
        .current =
            current.with_new_origin( canvas_.upper_left() ) } );
  }

  void on_drag_finished( input::mod_keys const& /*mod*/,
                         input::e_mouse_button /*button*/,
                         Coord /*origin*/,
                         Coord /*end*/ ) override {
    CHECK( S_.drag_state );
    S_.drag_state->stream.finish();
    // At this point we assume that the callback will finish on
    // its own after doing any post-drag stuff it needs to do. No
    // new drags can start until then.
  }

  // ------------------------------------------------------------
  // Main Thread
  // ------------------------------------------------------------
  wait<> run_harbor_view() {
    create_entities( S_, &entities_ );
    // TODO: how does this thread interact with the dragging
    // thread? It should probably somehow co_await on it when a
    // drag happens.
    co_await S_.exit_promise.wait();
  }

  // ------------------------------------------------------------
  // API
  // ------------------------------------------------------------
  wait<> show_harbor_view() {
    HarborState& hb_state = S_.harbor_state();
    S_.exit_promise       = {};
    if( hb_state.selected_unit ) {
      UnitId id = *hb_state.selected_unit;
      // We could have a case where the unit that was last
      // selected went to the new world and was then disbanded,
      // or is just no longer in the harbor.
      if( !S_.units_state.exists( id ) ||
          !S_.units_state.maybe_harbor_view_state_of( id ) )
        hb_state.selected_unit = nothing;
    }
    lg.info( "entering harbor view." );
    co_await run_harbor_view();
    lg.info( "leaving harbor view." );
  }

  void set_selected_unit( UnitId id ) {
    HarborState&      hb_state    = S_.harbor_state();
    UnitsState const& units_state = S_.units_state;
    // Ensure that the unit is either in port or on the high
    // seas, otherwise it doesn't make sense for the unit to be
    // selected on this screen.
    CHECK( units_state.maybe_harbor_view_state_of( id ) );
    hb_state.selected_unit = id;
  }

  // ------------------------------------------------------------
  // Members
  // ------------------------------------------------------------
  Entities entities_{};
  Rect     canvas_;
};

/****************************************************************
** HarborPlane
*****************************************************************/
Plane& HarborPlane::impl() { return *impl_; }

HarborPlane::~HarborPlane() = default;

HarborPlane::HarborPlane( Player&             player,
                          UnitsState&         units_state,
                          TerrainState const& terrain_state,
                          IGui&               gui )
  : impl_(
        new Impl( player, units_state, terrain_state, gui ) ) {}

void HarborPlane::set_selected_unit( UnitId id ) {
  impl_->set_selected_unit( id );
}

wait<> HarborPlane::show_harbor_view() {
  return impl_->show_harbor_view();
}

/****************************************************************
** API
*****************************************************************/
wait<> show_harbor_view( Planes& planes, Player& player,
                         UnitsState&         units_state,
                         TerrainState const& terrain_state,
                         maybe<UnitId>       selected_unit ) {
  WindowPlane window_plane;
  RealGui     gui( window_plane );
  HarborPlane harbor_plane( player, units_state, terrain_state,
                            gui );
  if( selected_unit.has_value() )
    harbor_plane.set_selected_unit( *selected_unit );
  auto        popper = planes.new_group();
  PlaneGroup& group  = planes.back();
  group.push( harbor_plane );
  group.push( window_plane );
  co_await harbor_plane.show_harbor_view();
}

} // namespace rn