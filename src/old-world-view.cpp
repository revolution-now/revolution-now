/****************************************************************
**old-world-view.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-06-14.
*
* Description: Implements the Old World port view.
*
*****************************************************************/
#include "old-world-view.hpp"

// Revolution Now
#include "anim.hpp"
#include "cargo.hpp"
#include "co-combinator.hpp"
#include "co-waitable.hpp"
#include "commodity.hpp"
#include "coord.hpp"
#include "dragdrop.hpp"
#include "fb.hpp"
#include "fmt-helper.hpp"
#include "gfx.hpp"
#include "image.hpp"
#include "init.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "macros.hpp"
#include "old-world.hpp"
#include "plane-ctrl.hpp"
#include "plane.hpp"
#include "render.hpp"
#include "screen.hpp"
#include "sg-macros.hpp"
#include "text.hpp"
#include "tiles.hpp"
#include "ustate.hpp"
#include "variant.hpp"
#include "waitable.hpp"
#include "window.hpp"

// base
#include "base/attributes.hpp"
#include "base/lambda.hpp"
#include "base/range-lite.hpp"
#include "base/scope-exit.hpp"
#include "base/vocab.hpp"

// Rds
#include "rds/old-world-view-impl.hpp"

// Flatbuffers
#include "fb/sg-old-world-view_generated.h"

using namespace std;

namespace rn {

namespace rl = ::base::rl;

DECLARE_SAVEGAME_SERIALIZERS( OldWorldView );

namespace {

// When we drag a commodity from the market this is the default
// amount that we take.
constexpr int const k_default_market_quantity = 100;

/****************************************************************
** Globals
*****************************************************************/
waitable_promise<> g_exit_promise;

/****************************************************************
** Save-Game State
*****************************************************************/
struct SAVEGAME_STRUCT( OldWorldView ) {
  // Fields that are actually serialized.

  // clang-format off
  SAVEGAME_MEMBERS( OldWorldView,
  ( maybe<UnitId>, selected_unit ));
  // clang-format on

 public:
  // Fields that are derived from the serialized fields.

 private:
  SAVEGAME_FRIENDS( OldWorldView );
  SAVEGAME_SYNC() {
    // Sync all fields that are derived from serialized fields
    // and then validate (check invariants).

    return valid;
  }
  // Called after all modules are deserialized.
  SAVEGAME_VALIDATE() { return valid; }
};
SAVEGAME_IMPL( OldWorldView );

/****************************************************************
** Dragging
*****************************************************************/
maybe<drag::State<OldWorldDraggableObject_t>> g_drag_state;
maybe<waitable<>>                             g_drag_thread;

/****************************************************************
** Draggable Object
*****************************************************************/
maybe<OldWorldDraggableObject_t> cargo_slot_to_draggable(
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
          []( UnitId id ) -> OldWorldDraggableObject_t {
            return OldWorldDraggableObject::unit{ /*id=*/id };
          },
          [&](
              Commodity const& c ) -> OldWorldDraggableObject_t {
            return OldWorldDraggableObject::cargo_commodity{
                /*comm=*/c,
                /*slot=*/slot_idx };
          } );
    }
  }
}

maybe<Cargo> draggable_to_cargo_object(
    OldWorldDraggableObject_t const& draggable ) {
  switch( draggable.to_enum() ) {
    case OldWorldDraggableObject::e::unit: {
      auto& val = draggable.get<OldWorldDraggableObject::unit>();
      return val.id;
    }
    case OldWorldDraggableObject::e::market_commodity:
      return nothing;
    case OldWorldDraggableObject::e::cargo_commodity: {
      auto& val =
          draggable
              .get<OldWorldDraggableObject::cargo_commodity>();
      return val.comm;
    }
  }
}

maybe<OldWorldDraggableObject_t> draggable_in_cargo_slot(
    CargoSlotIndex slot ) {
  return SG()
      .selected_unit.fmap( unit_from_id )
      .bind( LC( _.cargo().at( slot ) ) )
      .bind( LC( cargo_slot_to_draggable( slot, _ ) ) );
}

maybe<OldWorldDraggableObject_t> draggable_in_cargo_slot(
    int slot ) {
  return draggable_in_cargo_slot( CargoSlotIndex{ slot } );
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
** Old World View Entities
*****************************************************************/
namespace entity {

// Each entity is defined by a struct that holds its state and
// that has the following methods:
//
//  void draw( Texture& tx, Delta offset ) const;
//  Rect bounds() const;
//  static maybe<EntityClass> create( ... );
//  maybe<pair<T,Rect>> obj_under_cursor( Coord const& );

// This object represents the array of cargo items available for
// trade in europe and which is show at the bottom of the screen.
class MarketCommodities {
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

  void draw( Texture& tx, Delta offset ) const {
    auto bds     = bounds();
    auto grid    = bds.to_grid_noalign( sprite_delta );
    auto comm_it = enum_traits<e_commodity>::values.begin();
    auto label   = CommodityLabel::buy_sell{ 100, 200 };
    for( auto rect : range_of_rects( grid ) ) {
      render_rect( tx, Color::white(),
                   rect.shifted_by( offset ) );
      render_commodity_annotated(
          tx, *comm_it++,
          rect.shifted_by( offset ).upper_left() +
              kCommodityInCargoHoldRenderingOffset,
          label );
      label.buy += 120;
      label.sell += 120;
    }
  }

  MarketCommodities( MarketCommodities&& ) = default;
  MarketCommodities& operator=( MarketCommodities&& ) = default;

  static maybe<MarketCommodities> create( Delta const& size ) {
    maybe<MarketCommodities> res;
    auto                     rect = Rect::from( Coord{}, size );
    if( size.w >= single_layer_width &&
        size.h >= single_layer_height ) {
      res = MarketCommodities{
          /*doubled_=*/false,
          /*origin_=*/Coord{
              /*x=*/rect.center().x - single_layer_width / 2_sx,
              /*y=*/rect.bottom_edge() - single_layer_height } };
    } else if( rect.w >= double_layer_width &&
               rect.h >= double_layer_height ) {
      res = MarketCommodities{
          /*doubled_=*/true,
          /*origin_=*/Coord{
              /*x=*/rect.center().x - double_layer_width / 2_sx,
              /*y=*/rect.bottom_edge() - double_layer_height } };

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
  MarketCommodities() = default;
  MarketCommodities( bool doubled, Coord origin )
    : doubled_( doubled ), origin_( origin ) {}
};
NOTHROW_MOVE( MarketCommodities );

class ActiveCargoBox {
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

  void draw( Texture& tx, Delta offset ) const {
    auto bds  = bounds();
    auto grid = bds.to_grid_noalign( box_delta );
    for( auto rect : range_of_rects( grid ) )
      render_rect( tx, Color::white(),
                   rect.shifted_by( offset ) );
  }

  ActiveCargoBox( ActiveCargoBox&& ) = default;
  ActiveCargoBox& operator=( ActiveCargoBox&& ) = default;

  static maybe<ActiveCargoBox> create(
      Delta const& size, maybe<MarketCommodities> const&
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
            /*origin_=*/Coord{
                market_commodities.origin_.y - size_pixels.h +
                    1_h,
                rect.center().x - size_pixels.w / 2_sx } );
      } else {
        // Possibly just for now do this.
        res = ActiveCargoBox(
            /*origin_=*/Coord{
                market_commodities.origin_.y - size_pixels.h +
                    1_h,
                rect.center().x - size_pixels.w / 2_sx } );
      }
    }
    return res;
  }

 private:
  ActiveCargoBox() = default;
  ActiveCargoBox( Coord origin ) : origin_( origin ) {}
  Coord origin_{};
};
NOTHROW_MOVE( ActiveCargoBox );

class DockAnchor {
  static constexpr Delta cross_leg_size{ 5_w, 5_h };
  static constexpr H     above_active_cargo_box{ 32_h };

 public:
  Rect bounds() const {
    // Just a point.
    return Rect::from( location_, Delta{} );
  }

  void draw( Texture& tx, Delta offset ) const {
    // This mess just draws an X.
    render_line(
        tx, Color::white(), location_ - cross_leg_size + offset,
        cross_leg_size * Scale{ 2 } + Delta{ 1_w, 1_h } );
    render_line(
        tx, Color::white(),
        location_ - cross_leg_size.mirrored_vertically() +
            offset,
        cross_leg_size.mirrored_vertically() * Scale{ 2 } +
            Delta{ 1_w, -1_h } );
  }

  DockAnchor( DockAnchor&& ) = default;
  DockAnchor& operator=( DockAnchor&& ) = default;

  static maybe<DockAnchor> create(
      Delta const&                 size,
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
      res              = DockAnchor{};
      res->location_.x = location_x;
      res->location_.y = location_y;
    }
    return res;
  }

 private:
  DockAnchor() = default;
  DockAnchor( Coord location ) : location_( location ) {}
  Coord location_{};
};
NOTHROW_MOVE( DockAnchor );

class Backdrop {
  static constexpr Delta image_distance_from_anchor{ 950_w,
                                                     544_h };

 public:
  Rect bounds() const { return Rect::from( Coord{}, size_ ); }

  void draw( Texture& tx, Delta offset ) const {
    copy_texture(
        image( e_image::europe ), tx,
        Rect::from( upper_left_of_render_rect_, size_ ),
        Coord{} + offset );
  }

  Backdrop( Backdrop&& ) = default;
  Backdrop& operator=( Backdrop&& ) = default;

  static maybe<Backdrop> create(
      Delta const&             size,
      maybe<DockAnchor> const& maybe_dock_anchor ) {
    maybe<Backdrop> res;
    if( maybe_dock_anchor ) {
      res =
          Backdrop{ -( maybe_dock_anchor->bounds().upper_left() -
                       image_distance_from_anchor ),
                    size };
    }
    return res;
  }

 private:
  Backdrop() = default;
  Backdrop( Coord upper_left, Delta size )
    : upper_left_of_render_rect_( upper_left ), size_( size ) {}
  Coord upper_left_of_render_rect_{};
  Delta size_{};
};
NOTHROW_MOVE( Backdrop );

class InPortBox {
 public:
  static constexpr Delta block_size{ 32_w, 32_h };
  static constexpr SY    height_blocks{ 3 };
  static constexpr SX    width_wide{ 3 };
  static constexpr SX    width_narrow{ 2 };

  Rect bounds() const {
    return Rect::from( origin_, block_size * size_in_blocks_ +
                                    Delta{ 1_w, 1_h } );
  }

  void draw( Texture& tx, Delta offset ) const {
    render_rect( tx, Color::white(),
                 bounds().shifted_by( offset ) );
    auto const& label_tx =
        render_text( "In Port", Color::white() );
    copy_texture(
        label_tx, tx,
        bounds().upper_left() + Delta{ 2_w, 2_h } + offset );
  }

  InPortBox( InPortBox&& ) = default;
  InPortBox& operator=( InPortBox&& ) = default;

  static maybe<InPortBox> create(
      Delta const&                 size,
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

      res = InPortBox{ origin,         //
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
  InPortBox() = default;
  InPortBox( Coord origin, Scale size_in_blocks, bool is_wide )
    : origin_( origin ),
      size_in_blocks_( size_in_blocks ),
      is_wide_( is_wide ) {}
};
NOTHROW_MOVE( InPortBox );

class InboundBox {
 public:
  Rect bounds() const {
    return Rect::from( origin_,
                       InPortBox::block_size * size_in_blocks_ +
                           Delta{ 1_w, 1_h } );
  }

  void draw( Texture& tx, Delta offset ) const {
    render_rect( tx, Color::white(),
                 bounds().shifted_by( offset ) );
    auto const& label_tx =
        render_text( "Inbound", Color::white() );
    copy_texture(
        label_tx, tx,
        bounds().upper_left() + Delta{ 2_w, 2_h } + offset );
  }

  InboundBox( InboundBox&& ) = default;
  InboundBox& operator=( InboundBox&& ) = default;

  static maybe<InboundBox> create(
      Delta const&            size,
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
  InboundBox() = default;
  InboundBox( Coord origin, Scale size_in_blocks, bool is_wide )
    : origin_( origin ),
      size_in_blocks_( size_in_blocks ),
      is_wide_( is_wide ) {}
  Coord origin_{};
  Scale size_in_blocks_{};
  bool  is_wide_{};
};
NOTHROW_MOVE( InboundBox );

class OutboundBox {
 public:
  Rect bounds() const {
    return Rect::from( origin_,
                       InPortBox::block_size * size_in_blocks_ +
                           Delta{ 1_w, 1_h } );
  }

  void draw( Texture& tx, Delta offset ) const {
    render_rect( tx, Color::white(),
                 bounds().shifted_by( offset ) );
    auto const& label_tx =
        render_text( "Outbound", Color::white() );
    copy_texture(
        label_tx, tx,
        bounds().upper_left() + Delta{ 2_w, 2_h } + offset );
  }

  OutboundBox( OutboundBox&& ) = default;
  OutboundBox& operator=( OutboundBox&& ) = default;

  static maybe<OutboundBox> create(
      Delta const&             size,
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
  OutboundBox() = default;
  OutboundBox( Coord origin, Scale size_in_blocks, bool is_wide )
    : origin_( origin ),
      size_in_blocks_( size_in_blocks ),
      is_wide_( is_wide ) {}
  Coord origin_{};
  Scale size_in_blocks_{};
  bool  is_wide_{};
};
NOTHROW_MOVE( OutboundBox );

class Exit {
  static constexpr Delta exit_block_pixels{ 24_w, 24_h };

 public:
  Rect bounds() const {
    return Rect::from( origin_, exit_block_pixels ) +
           Delta{ 2_w, 2_h };
  }

  void draw( Texture& tx, Delta offset ) const {
    auto bds = bounds().with_inc_size();
    bds      = bds.shifted_by( Delta{ -2_w, -2_h } );
    auto const& exit_tx =
        render_text( font::standard(), Color::red(), "Exit" );
    auto drawing_origin = centered( exit_tx.size(), bds );
    copy_texture( exit_tx, tx, drawing_origin + offset );
    render_rect( tx, Color::white(), bds.shifted_by( offset ) );
  }

  Exit( Exit&& ) = default;
  Exit& operator=( Exit&& ) = default;

  static maybe<Exit> create( Delta const& size,
                             maybe<MarketCommodities> const&
                                 maybe_market_commodities ) {
    maybe<Exit> res;
    if( maybe_market_commodities ) {
      auto origin =
          maybe_market_commodities->bounds().lower_right() -
          Delta{ 1_w, 1_h } - exit_block_pixels.h;
      auto lr_delta = origin + exit_block_pixels - Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h ) {
        origin =
            maybe_market_commodities->bounds().upper_right() -
            1_w - exit_block_pixels;
      }
      res = Exit{ origin };
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
  Exit() = default;
  Exit( Coord origin ) : origin_( origin ) {}
  Coord origin_{};
};
NOTHROW_MOVE( Exit );

class Dock {
  static constexpr Scale dock_block_pixels{ 24 };
  static constexpr Delta dock_block_pixels_delta =
      Delta{ 1_w, 1_h } * dock_block_pixels;

 public:
  Rect bounds() const {
    return Rect::from(
        origin_, Delta{ length_in_blocks_ * dock_block_pixels.sx,
                        1_h * dock_block_pixels.sy } );
  }

  void draw( Texture& tx, Delta offset ) const {
    auto bds  = bounds();
    auto grid = bds.to_grid_noalign( dock_block_pixels_delta );
    for( auto rect : range_of_rects( grid ) )
      render_rect( tx, Color::white(),
                   rect.shifted_by( offset ) );
  }

  Dock( Dock&& ) = default;
  Dock& operator=( Dock&& ) = default;

  static maybe<Dock> create(
      Delta const&             size,
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
      res = Dock{ /*origin_=*/origin,
                  /*length_in_blocks_=*/available };
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
  Dock() = default;
  Dock( Coord origin, W length_in_blocks )
    : origin_( origin ), length_in_blocks_( length_in_blocks ) {}
  Coord origin_{};
  W     length_in_blocks_{};
};
NOTHROW_MOVE( Dock );

// Base class for other entities that just consist of a collec-
// tion of units.
class UnitCollection {
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

  void draw( Texture& tx, Delta offset ) const {
    // auto bds = bounds();
    // render_rect( tx, Color::white(), bds.shifted_by( offset )
    // );
    for( auto const& unit_with_pos : units_ )
      if( !g_drag_state || g_drag_state->object !=
                               OldWorldDraggableObject_t{
                                   OldWorldDraggableObject::unit{
                                       unit_with_pos.id } } )
        render_unit( tx, unit_with_pos.id,
                     unit_with_pos.pixel_coord + offset,
                     /*with_icon=*/false );
    if( SG().selected_unit ) {
      for( auto [id, coord] : units_ ) {
        if( id == *SG().selected_unit ) {
          render_rect( tx, Color::green(),
                       Rect::from( coord, g_tile_delta )
                           .shifted_by( offset ) );
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

  UnitCollection( Rect bounds_when_no_units,
                  vector<UnitWithPosition>&& units )
    : bounds_when_no_units_( bounds_when_no_units ),
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
      Delta const&             size,
      maybe<DockAnchor> const& maybe_dock_anchor,
      maybe<Dock> const&       maybe_dock ) {
    maybe<UnitsOnDock> res;
    if( maybe_dock_anchor && maybe_dock ) {
      vector<UnitWithPosition> units;
      Coord                    coord =
          maybe_dock->bounds().upper_right() - g_tile_delta;
      for( auto id : old_world_units_on_dock() ) {
        units.push_back( { id, coord } );
        coord -= g_tile_delta.w;
        if( coord.x < maybe_dock->bounds().left_edge() )
          coord = Coord{ ( maybe_dock->bounds().upper_right() -
                           g_tile_delta )
                             .x,
                         coord.y - g_tile_delta.h };
      }
      // populate units...
      res = UnitsOnDock{
          /*bounds_when_no_units_=*/maybe_dock_anchor->bounds(),
          /*units_=*/std::move( units ) };
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
  UnitsOnDock( Rect                       dock_anchor,
               vector<UnitWithPosition>&& units )
    : UnitCollection( dock_anchor, std::move( units ) ) {}
};
NOTHROW_MOVE( UnitsOnDock );

class ShipsInPort : public UnitCollection {
 public:
  ShipsInPort( ShipsInPort&& ) = default;
  ShipsInPort& operator=( ShipsInPort&& ) = default;

  static maybe<ShipsInPort> create(
      Delta const&            size,
      maybe<InPortBox> const& maybe_in_port_box ) {
    maybe<ShipsInPort> res;
    if( maybe_in_port_box ) {
      vector<UnitWithPosition> units;
      auto  in_port_bds = maybe_in_port_box->bounds();
      Coord coord = in_port_bds.lower_right() - g_tile_delta;
      for( auto id : old_world_units_in_port() ) {
        units.push_back( { id, coord } );
        coord -= g_tile_delta.w;
        if( coord.x < in_port_bds.left_edge() )
          coord = Coord{
              ( in_port_bds.upper_right() - g_tile_delta ).x,
              coord.y - g_tile_delta.h };
      }
      // populate units...
      res =
          ShipsInPort{ /*bounds_when_no_units_=*/Rect::from(
                           in_port_bds.lower_right(), Delta{} ),
                       /*units_=*/std::move( units ) };
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
  ShipsInPort( Rect                       dock_anchor,
               vector<UnitWithPosition>&& units )
    : UnitCollection( dock_anchor, std::move( units ) ) {}
};
NOTHROW_MOVE( ShipsInPort );

class ShipsInbound : public UnitCollection {
 public:
  ShipsInbound( ShipsInbound&& ) = default;
  ShipsInbound& operator=( ShipsInbound&& ) = default;

  static maybe<ShipsInbound> create(
      Delta const&             size,
      maybe<InboundBox> const& maybe_inbound_box ) {
    maybe<ShipsInbound> res;
    if( maybe_inbound_box ) {
      vector<UnitWithPosition> units;
      auto  frame_bds = maybe_inbound_box->bounds();
      Coord coord     = frame_bds.lower_right() - g_tile_delta;
      for( auto id : old_world_units_inbound() ) {
        units.push_back( { id, coord } );
        coord -= g_tile_delta.w;
        if( coord.x < frame_bds.left_edge() )
          coord = Coord{
              ( frame_bds.upper_right() - g_tile_delta ).x,
              coord.y - g_tile_delta.h };
      }
      // populate units...
      res = ShipsInbound{ /*bounds_when_no_units_=*/Rect::from(
                              frame_bds.lower_right(), Delta{} ),
                          /*units_=*/std::move( units ) };
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
  ShipsInbound( Rect                       dock_anchor,
                vector<UnitWithPosition>&& units )
    : UnitCollection( dock_anchor, std::move( units ) ) {}
};
NOTHROW_MOVE( ShipsInbound );

class ShipsOutbound : public UnitCollection {
 public:
  ShipsOutbound( ShipsOutbound&& ) = default;
  ShipsOutbound& operator=( ShipsOutbound&& ) = default;

  static maybe<ShipsOutbound> create(
      Delta const&              size,
      maybe<OutboundBox> const& maybe_outbound_box ) {
    maybe<ShipsOutbound> res;
    if( maybe_outbound_box ) {
      vector<UnitWithPosition> units;
      auto  frame_bds = maybe_outbound_box->bounds();
      Coord coord     = frame_bds.lower_right() - g_tile_delta;
      for( auto id : old_world_units_outbound() ) {
        units.push_back( { id, coord } );
        coord -= g_tile_delta.w;
        if( coord.x < frame_bds.left_edge() )
          coord = Coord{
              ( frame_bds.upper_right() - g_tile_delta ).x,
              coord.y - g_tile_delta.h };
      }
      // populate units...
      res =
          ShipsOutbound{ /*bounds_when_no_units_=*/Rect::from(
                             frame_bds.lower_right(), Delta{} ),
                         /*units_=*/std::move( units ) };
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
  ShipsOutbound( Rect                       dock_anchor,
                 vector<UnitWithPosition>&& units )
    : UnitCollection( dock_anchor, std::move( units ) ) {}
};
NOTHROW_MOVE( ShipsOutbound );

class ActiveCargo {
 public:
  Rect bounds() const { return bounds_; }

  void draw( Texture& tx, Delta offset ) const {
    auto bds  = bounds();
    auto grid = bds.to_grid_noalign( ActiveCargoBox::box_delta );
    if( maybe_active_unit_ ) {
      auto&       unit = unit_from_id( *maybe_active_unit_ );
      auto const& cargo_slots = unit.cargo().slots();
      for( auto const& [idx, cargo_slot, rect] :
           rl::zip( rl::ints(), cargo_slots,
                    range_of_rects( grid ) ) ) {
        if( g_drag_state.has_value() ) {
          if_get( g_drag_state->object,
                  OldWorldDraggableObject::cargo_commodity,
                  cc ) {
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
                [&]( UnitId id ) {
                  if( !g_drag_state ||
                      g_drag_state->object !=
                          OldWorldDraggableObject_t{
                              OldWorldDraggableObject::unit{
                                  id } } )
                    render_unit( tx, id, dst_coord,
                                 /*with_icon=*/false );
                },
                [&]( Commodity const& c ) {
                  render_commodity_annotated(
                      tx, c,
                      dst_coord +
                          kCommodityInCargoHoldRenderingOffset );
                } );
            break;
          }
        }
      }
      for( auto [idx, rect] :
           rl::zip( rl::ints(), range_of_rects( grid ) ) ) {
        if( idx >= unit.cargo().slots_total() )
          render_fill_rect( tx, Color::white(),
                            rect.shifted_by( offset ) );
      }
    } else {
      for( auto rect : range_of_rects( grid ) )
        render_fill_rect( tx, Color::white(),
                          rect.shifted_by( offset ) );
    }
  }

  ActiveCargo( ActiveCargo&& ) = default;
  ActiveCargo& operator=( ActiveCargo&& ) = default;

  static maybe<ActiveCargo> create(
      Delta const&                 size,
      maybe<ActiveCargoBox> const& maybe_active_cargo_box,
      maybe<ShipsInPort> const&    maybe_ships_in_port ) {
    maybe<ActiveCargo> res;
    if( maybe_active_cargo_box && maybe_ships_in_port ) {
      res = ActiveCargo{
          /*maybe_active_unit_=*/SG().selected_unit,
          /*bounds_=*/maybe_active_cargo_box->bounds() };
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

          using OldWorldDraggableObject::cargo_commodity;
          if( draggable_in_cargo_slot( *maybe_slot )
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
  ActiveCargo() = default;
  ActiveCargo( maybe<UnitId> maybe_active_unit, Rect bounds )
    : maybe_active_unit_( maybe_active_unit ),
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

void create_entities( Entities* entities ) {
  using namespace entity;
  Delta clip                   = main_window_logical_size();
  entities->market_commodities = //
      MarketCommodities::create( clip );
  entities->active_cargo_box =      //
      ActiveCargoBox::create( clip, //
                              entities->market_commodities );
  entities->dock_anchor =                             //
      DockAnchor::create( clip,                       //
                          entities->active_cargo_box, //
                          entities->market_commodities );
  entities->backdrop =        //
      Backdrop::create( clip, //
                        entities->dock_anchor );
  entities->in_port_box =                            //
      InPortBox::create( clip,                       //
                         entities->active_cargo_box, //
                         entities->market_commodities );
  entities->inbound_box =       //
      InboundBox::create( clip, //
                          entities->in_port_box );
  entities->outbound_box =       //
      OutboundBox::create( clip, //
                           entities->inbound_box );
  entities->exit_label =  //
      Exit::create( clip, //
                    entities->market_commodities );
  entities->dock =                         //
      Dock::create( clip,                  //
                    entities->dock_anchor, //
                    entities->in_port_box );
  entities->units_on_dock =                       //
      UnitsOnDock::create( clip,                  //
                           entities->dock_anchor, //
                           entities->dock );
  entities->ships_in_port =      //
      ShipsInPort::create( clip, //
                           entities->in_port_box );
  entities->ships_inbound =       //
      ShipsInbound::create( clip, //
                            entities->inbound_box );
  entities->ships_outbound =       //
      ShipsOutbound::create( clip, //
                             entities->outbound_box );
  entities->active_cargo =                             //
      ActiveCargo::create( clip,                       //
                           entities->active_cargo_box, //
                           entities->ships_in_port );
}

void draw_entities( Texture& tx, Entities const& entities ) {
  auto offset = Delta{};
  if( entities.backdrop.has_value() )
    entities.backdrop->draw( tx, offset );
  if( entities.market_commodities.has_value() )
    entities.market_commodities->draw( tx, offset );
  if( entities.active_cargo_box.has_value() )
    entities.active_cargo_box->draw( tx, offset );
  if( entities.dock_anchor.has_value() )
    entities.dock_anchor->draw( tx, offset );
  if( entities.in_port_box.has_value() )
    entities.in_port_box->draw( tx, offset );
  if( entities.inbound_box.has_value() )
    entities.inbound_box->draw( tx, offset );
  if( entities.outbound_box.has_value() )
    entities.outbound_box->draw( tx, offset );
  if( entities.exit_label.has_value() )
    entities.exit_label->draw( tx, offset );
  if( entities.dock.has_value() )
    entities.dock->draw( tx, offset );
  if( entities.units_on_dock.has_value() )
    entities.units_on_dock->draw( tx, offset );
  if( entities.ships_in_port.has_value() )
    entities.ships_in_port->draw( tx, offset );
  if( entities.ships_inbound.has_value() )
    entities.ships_inbound->draw( tx, offset );
  if( entities.ships_outbound.has_value() )
    entities.ships_outbound->draw( tx, offset );
  if( entities.active_cargo.has_value() )
    entities.active_cargo->draw( tx, offset );
}

/****************************************************************
** Drag & Drop
*****************************************************************/
struct OldWorldDragSrcInfo {
  OldWorldDragSrc_t src;
  Rect              rect;
};

maybe<OldWorldDragSrcInfo> drag_src_from_coord(
    Coord const& coord, Entities const* entities ) {
  using namespace entity;
  maybe<OldWorldDragSrcInfo> res;
  if( entities->units_on_dock.has_value() ) {
    if( auto maybe_pair =
            entities->units_on_dock->obj_under_cursor( coord );
        maybe_pair ) {
      auto const& [id, rect] = *maybe_pair;
      res                    = OldWorldDragSrcInfo{
          /*src=*/OldWorldDragSrc::dock{ /*id=*/id },
          /*rect=*/rect };
    }
  }
  if( entities->active_cargo.has_value() ) {
    auto const& active_cargo = *entities->active_cargo;
    auto        in_port      = active_cargo.active_unit()
                       .fmap( is_unit_in_port )
                       .is_value_truish();
    auto maybe_pair = base::just( coord ).bind(
        LC( active_cargo.obj_under_cursor( _ ) ) );
    if( in_port &&
        maybe_pair.fmap( L( _.first ) )
            .bind( L( draggable_in_cargo_slot( _ ) ) ) ) {
      auto const& [slot, rect] = *maybe_pair;

      res = OldWorldDragSrcInfo{
          /*src=*/OldWorldDragSrc::cargo{ /*slot=*/slot,
                                          /*quantity=*/nothing },
          /*rect=*/rect };
    }
  }
  if( entities->ships_outbound.has_value() ) {
    if( auto maybe_pair =
            entities->ships_outbound->obj_under_cursor( coord );
        maybe_pair ) {
      auto const& [id, rect] = *maybe_pair;

      res = OldWorldDragSrcInfo{
          /*src=*/OldWorldDragSrc::outbound{ /*id=*/id },
          /*rect=*/rect };
    }
  }
  if( entities->ships_inbound.has_value() ) {
    if( auto maybe_pair =
            entities->ships_inbound->obj_under_cursor( coord );
        maybe_pair ) {
      auto const& [id, rect] = *maybe_pair;

      res = OldWorldDragSrcInfo{
          /*src=*/OldWorldDragSrc::inbound{ /*id=*/id },
          /*rect=*/rect };
    }
  }
  if( entities->ships_in_port.has_value() ) {
    if( auto maybe_pair =
            entities->ships_in_port->obj_under_cursor( coord );
        maybe_pair ) {
      auto const& [id, rect] = *maybe_pair;

      res = OldWorldDragSrcInfo{
          /*src=*/OldWorldDragSrc::inport{ /*id=*/id },
          /*rect=*/rect };
    }
  }
  if( entities->market_commodities.has_value() ) {
    if( auto maybe_pair =
            entities->market_commodities->obj_under_cursor(
                coord );
        maybe_pair ) {
      auto const& [type, rect] = *maybe_pair;

      res = OldWorldDragSrcInfo{ /*src=*/OldWorldDragSrc::market{
                                     /*type=*/type,
                                     /*quantity=*/nothing },
                                 /*rect=*/rect };
    }
  }

  return res;
}

// Important: in this function we should not return early; we
// should check all the entities (in order) to allow later ones
// to override earlier ones.
maybe<OldWorldDragDst_t> drag_dst_from_coord(
    Entities const* entities, Coord const& coord ) {
  using namespace entity;
  maybe<OldWorldDragDst_t> res;
  if( entities->active_cargo.has_value() ) {
    if( auto maybe_pair =
            entities->active_cargo->obj_under_cursor( coord );
        maybe_pair ) {
      auto const& slot = maybe_pair->first;

      res = OldWorldDragDst::cargo{
          /*slot=*/slot //
      };
    }
  }
  if( entities->dock.has_value() ) {
    if( coord.is_inside( entities->dock->bounds() ) )
      res = OldWorldDragDst::dock{};
  }
  if( entities->units_on_dock.has_value() ) {
    if( coord.is_inside( entities->units_on_dock->bounds() ) )
      res = OldWorldDragDst::dock{};
  }
  if( entities->outbound_box.has_value() ) {
    if( coord.is_inside( entities->outbound_box->bounds() ) )
      res = OldWorldDragDst::outbound{};
  }
  if( entities->inbound_box.has_value() ) {
    if( coord.is_inside( entities->inbound_box->bounds() ) )
      res = OldWorldDragDst::inbound{};
  }
  if( entities->in_port_box.has_value() ) {
    if( coord.is_inside( entities->in_port_box->bounds() ) )
      res = OldWorldDragDst::inport{};
  }
  if( entities->ships_in_port.has_value() ) {
    if( auto maybe_pair =
            entities->ships_in_port->obj_under_cursor( coord );
        maybe_pair ) {
      auto const& ship = maybe_pair->first;

      res = OldWorldDragDst::inport_ship{
          /*id=*/ship, //
      };
    }
  }
  if( entities->market_commodities.has_value() ) {
    if( coord.is_inside(
            entities->market_commodities->bounds() ) )
      return OldWorldDragDst::market{};
  }
  return res;
}

maybe<UnitId> active_cargo_ship( Entities const* entities ) {
  return entities->active_cargo.bind(
      &entity::ActiveCargo::active_unit );
}

OldWorldDraggableObject_t draggable_from_src(
    OldWorldDragSrc_t const& drag_src ) {
  using namespace OldWorldDragSrc;
  return overload_visit<OldWorldDraggableObject_t>(
      drag_src,
      [&]( dock const& o ) {
        return OldWorldDraggableObject::unit{ o.id };
      },
      [&]( cargo const& o ) {
        // Not all cargo slots must have an item in them, but in
        // this case the slot should otherwise the
        // OldWorldDragSrc object should never have been created.
        UNWRAP_CHECK( object,
                      draggable_in_cargo_slot( o.slot ) );
        return object;
      },
      [&]( outbound const& o ) {
        return OldWorldDraggableObject::unit{ o.id };
      },
      [&]( inbound const& o ) {
        return OldWorldDraggableObject::unit{ o.id };
      },
      [&]( inport const& o ) {
        return OldWorldDraggableObject::unit{ o.id };
      },
      [&]( market const& o ) {
        return OldWorldDraggableObject::market_commodity{
            o.type };
      } );
}

#define DRAG_CONNECT_CASE( src_, dst_ )                  \
  operator()(                                            \
      [[maybe_unused]] OldWorldDragSrc::src_ const& src, \
      [[maybe_unused]] OldWorldDragDst::dst_ const& dst )

struct DragConnector {
  static bool visit( Entities const*          entities,
                     OldWorldDragSrc_t const& drag_src,
                     OldWorldDragDst_t const& drag_dst ) {
    DragConnector connector( entities );
    return std::visit( connector, drag_src, drag_dst );
  }

  Entities const* entities = nullptr;
  DragConnector( Entities const* entities_ )
    : entities( entities_ ) {}
  bool DRAG_CONNECT_CASE( dock, cargo ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    if( !is_unit_in_port( ship ) ) return false;
    return unit_from_id( ship ).cargo().fits_somewhere(
        src.id, dst.slot._ );
  }
  bool DRAG_CONNECT_CASE( cargo, dock ) const {
    return holds<OldWorldDraggableObject::unit>(
               draggable_from_src( src ) )
        .has_value();
  }
  bool DRAG_CONNECT_CASE( cargo, cargo ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    if( !is_unit_in_port( ship ) ) return false;
    if( src.slot == dst.slot ) return true;
    UNWRAP_CHECK(
        cargo_object,
        draggable_to_cargo_object( draggable_from_src( src ) ) );
    return overload_visit(
        cargo_object,
        [&]( UnitId ) {
          return unit_from_id( ship )
              .cargo()
              .fits_with_item_removed(
                  /*cargo=*/cargo_object,   //
                  /*remove_slot=*/src.slot, //
                  /*insert_slot=*/dst.slot  //
              );
        },
        [&]( Commodity const& c ) {
          // If at least one quantity of the commodity can be
          // moved then we will allow (at least a partial
          // transfer) to proceed.
          auto size_one     = c;
          size_one.quantity = 1;
          return unit_from_id( ship ).cargo().fits(
              /*cargo=*/size_one,
              /*slot=*/dst.slot );
        } );
  }
  bool DRAG_CONNECT_CASE( outbound, inbound ) const {
    return true;
  }
  bool DRAG_CONNECT_CASE( outbound, inport ) const {
    UNWRAP_CHECK( info, unit_old_world_view_info( src.id ) );
    ASSIGN_CHECK_V( outbound, info,
                    UnitOldWorldViewState::outbound );
    // We'd like to do == 0.0 here, but this will avoid rounding
    // errors.
    return outbound.percent < 0.01;
  }
  bool DRAG_CONNECT_CASE( inbound, outbound ) const {
    return true;
  }
  bool DRAG_CONNECT_CASE( inport, outbound ) const {
    return true;
  }
  bool DRAG_CONNECT_CASE( dock, inport_ship ) const {
    return unit_from_id( dst.id ).cargo().fits_somewhere(
        src.id );
  }
  bool DRAG_CONNECT_CASE( cargo, inport_ship ) const {
    auto dst_ship = dst.id;
    UNWRAP_CHECK(
        cargo_object,
        draggable_to_cargo_object( draggable_from_src( src ) ) );
    return overload_visit(
        cargo_object,
        [&]( UnitId id ) {
          if( is_unit_onboard( id ) == dst_ship ) return false;
          return unit_from_id( dst_ship )
              .cargo()
              .fits_somewhere( id );
        },
        [&]( Commodity const& c ) {
          // If even 1 quantity can fit then we can proceed
          // with (at least) a partial transfer.
          auto size_one     = c;
          size_one.quantity = 1;
          return unit_from_id( dst_ship )
              .cargo()
              .fits_somewhere( size_one );
        } );
  }
  bool DRAG_CONNECT_CASE( market, cargo ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    if( !is_unit_in_port( ship ) ) return false;
    auto comm = Commodity{
        /*type=*/src.type, //
        // If the commodity can fit even with just one quan-
        // tity then it is allowed, since we will just insert
        // as much as possible if we can't insert 100.
        /*quantity=*/1 //
    };
    return unit_from_id( ship ).cargo().fits_somewhere(
        comm, dst.slot._ );
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
        comm, /*starting_slot=*/0 );
  }
  bool DRAG_CONNECT_CASE( cargo, market ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    if( !is_unit_in_port( ship ) ) return false;
    return unit_from_id( ship )
        .cargo()
        .template slot_holds_cargo_type<Commodity>( src.slot._ )
        .has_value();
  }
  bool operator()( auto const&, auto const& ) const {
    return false;
  }
};

#define DRAG_CONFIRM_CASE( src_, dst_ )                    \
  operator()( [[maybe_unused]] OldWorldDragSrc::src_& src, \
              [[maybe_unused]] OldWorldDragDst::dst_& dst )

struct DragUserInput {
  Entities const* entities = nullptr;
  DragUserInput( Entities const* entities_ )
    : entities( entities_ ) {}

  static waitable<base::NoDiscard<bool>> visit(
      Entities const* entities, OldWorldDragSrc_t* drag_src,
      OldWorldDragDst_t* drag_dst ) {
    // Need to co_await here to keep parameters alive.
    bool proceed = co_await std::visit(
        DragUserInput( entities ), *drag_src, *drag_dst );
    co_return proceed;
  }

  static waitable<maybe<int>> ask_for_quantity(
      e_commodity type, string_view verb ) {
    string text = fmt::format(
        "What quantity of @[H]{}@[] would you like to {}? "
        "(0-100):",
        commodity_display_name( type ), verb );

    return ui::int_input_box( { .title = "Choose Quantity",
                                .msg   = text,
                                .min   = 0,
                                .max   = 100 } );
  }

  waitable<bool> DRAG_CONFIRM_CASE( market, cargo ) const {
    src.quantity = co_await ask_for_quantity( src.type, "buy" );
    co_return src.quantity.has_value();
  }
  waitable<bool> DRAG_CONFIRM_CASE( market, inport_ship ) const {
    src.quantity = co_await ask_for_quantity( src.type, "buy" );
    co_return src.quantity.has_value();
  }
  waitable<bool> DRAG_CONFIRM_CASE( cargo, market ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    CHECK( is_unit_in_port( ship ) );
    UNWRAP_CHECK( commodity_ref,
                  unit_from_id( ship )
                      .cargo()
                      .template slot_holds_cargo_type<Commodity>(
                          src.slot._ ) );
    src.quantity =
        co_await ask_for_quantity( commodity_ref.type, "sell" );
    co_return src.quantity.has_value();
  }
  waitable<bool> DRAG_CONFIRM_CASE( cargo, inport_ship ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    CHECK( is_unit_in_port( ship ) );
    auto maybe_commodity_ref =
        unit_from_id( ship )
            .cargo()
            .template slot_holds_cargo_type<Commodity>(
                src.slot._ );
    if( !maybe_commodity_ref.has_value() )
      // It's a unit.
      co_return true;
    src.quantity = co_await ask_for_quantity(
        maybe_commodity_ref->type, "move" );
    co_return src.quantity.has_value();
  }
  waitable<bool> operator()( auto const&, auto const& ) const {
    co_return true;
  }
};

#define DRAG_PERFORM_CASE( src_, dst_ )                  \
  operator()(                                            \
      [[maybe_unused]] OldWorldDragSrc::src_ const& src, \
      [[maybe_unused]] OldWorldDragDst::dst_ const& dst )

struct DragPerform {
  Entities const* entities = nullptr;
  DragPerform( Entities const* entities_ )
    : entities( entities_ ) {}

  static void visit( Entities const*          entities,
                     OldWorldDragSrc_t const& src,
                     OldWorldDragDst_t const& dst ) {
    lg.debug( "performing drag: {} to {}", src, dst );
    std::visit( DragPerform( entities ), src, dst );
  }

  void DRAG_PERFORM_CASE( dock, cargo ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    // First try to respect the destination slot chosen by
    // the player,
    if( unit_from_id( ship ).cargo().fits( src.id, dst.slot._ ) )
      ustate_change_to_cargo_somewhere( ship, src.id,
                                        dst.slot._ );
    else
      ustate_change_to_cargo_somewhere( ship, src.id );
  }
  void DRAG_PERFORM_CASE( cargo, dock ) const {
    ASSIGN_CHECK_V( unit, draggable_from_src( src ),
                    OldWorldDraggableObject::unit );
    unit_move_to_old_world_dock( unit.id );
  }
  void DRAG_PERFORM_CASE( cargo, cargo ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    UNWRAP_CHECK(
        cargo_object,
        draggable_to_cargo_object( draggable_from_src( src ) ) );
    overload_visit(
        cargo_object,
        [&]( UnitId id ) {
          // Will first "disown" unit which will remove it
          // from the cargo.
          ustate_change_to_cargo_somewhere( ship, id,
                                            dst.slot._ );
        },
        [&]( Commodity const& ) {
          move_commodity_as_much_as_possible(
              ship, src.slot._, ship, dst.slot._,
              /*max_quantity=*/nothing,
              /*try_other_dst_slots=*/false );
        } );
  }
  void DRAG_PERFORM_CASE( outbound, inbound ) const {
    unit_sail_to_old_world( src.id );
  }
  void DRAG_PERFORM_CASE( outbound, inport ) const {
    unit_sail_to_old_world( src.id );
  }
  void DRAG_PERFORM_CASE( inbound, outbound ) const {
    unit_sail_to_new_world( src.id );
  }
  void DRAG_PERFORM_CASE( inport, outbound ) const {
    unit_sail_to_new_world( src.id );
    // This is not strictly necessary, but for a nice user expe-
    // rience we will auto-select another unit that is in-port
    // (if any) since that is likely what the user wants to work
    // with, as opposed to keeping the selection on the unit that
    // is now outbound. Or if there are no more units in port,
    // just deselect.
    SG().selected_unit           = nothing;
    vector<UnitId> units_in_port = old_world_units_in_port();
    SG().selected_unit = rl::all( units_in_port ).head();
  }
  void DRAG_PERFORM_CASE( dock, inport_ship ) const {
    ustate_change_to_cargo_somewhere( dst.id, src.id );
  }
  void DRAG_PERFORM_CASE( cargo, inport_ship ) const {
    UNWRAP_CHECK(
        cargo_object,
        draggable_to_cargo_object( draggable_from_src( src ) ) );
    overload_visit(
        cargo_object,
        [&]( UnitId id ) {
          CHECK( !src.quantity.has_value() );
          // Will first "disown" unit which will remove it
          // from the cargo.
          ustate_change_to_cargo_somewhere( dst.id, id );
        },
        [&]( Commodity const& ) {
          UNWRAP_CHECK( src_ship,
                        active_cargo_ship( entities ) );
          move_commodity_as_much_as_possible(
              src_ship, src.slot._,
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
    add_commodity_to_cargo( comm, ship,
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
    add_commodity_to_cargo( comm, dst.id, /*slot=*/0,
                            /*try_other_slots=*/true );
  }
  void DRAG_PERFORM_CASE( cargo, market ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    UNWRAP_CHECK( commodity_ref,
                  unit_from_id( ship )
                      .cargo()
                      .template slot_holds_cargo_type<Commodity>(
                          src.slot._ ) );
    auto quantity_wants_to_sell =
        src.quantity.value_or( commodity_ref.quantity );
    int       amount_to_sell = std::min( quantity_wants_to_sell,
                                         commodity_ref.quantity );
    Commodity new_comm       = commodity_ref;
    new_comm.quantity -= amount_to_sell;
    rm_commodity_from_cargo( ship, src.slot._ );
    if( new_comm.quantity > 0 )
      add_commodity_to_cargo( new_comm, ship,
                              /*slot=*/src.slot._,
                              /*try_other_slots=*/false );
  }
  void operator()( auto const&, auto const& ) const {
    SHOULD_NOT_BE_HERE;
  }
};

void drag_n_drop_draw( Texture& tx ) {
  if( !g_drag_state ) return;
  auto& state      = *g_drag_state;
  auto  origin_for = [&]( Delta const& tile_size ) {
    return state.where - tile_size / Scale{ 2 } -
           state.click_offset;
  };
  using namespace OldWorldDraggableObject;
  // Render the dragged item.
  overload_visit(
      state.object,
      [&]( unit const& o ) {
        auto size =
            lookup_sprite( unit_from_id( o.id ).desc().tile )
                .size();
        render_unit( tx, o.id, origin_for( size ),
                     /*with_icon=*/false );
      },
      [&]( market_commodity const& o ) {
        auto size = commodity_tile_size( o.type );
        render_commodity( tx, o.type, origin_for( size ) );
      },
      [&]( cargo_commodity const& o ) {
        auto size = commodity_tile_size( o.comm.type );
        render_commodity( tx, o.comm.type, origin_for( size ) );
      } );
  // Render any indicators on top of it.
  switch( state.indicator ) {
    using e = drag::e_status_indicator;
    case e::none: break;
    case e::bad: {
      auto const& status_tx = render_text( "X", Color::red() );
      copy_texture( status_tx, tx,
                    origin_for( status_tx.size() ) );
      break;
    }
    case e::good: {
      auto const& status_tx = render_text( "+", Color::green() );
      copy_texture( status_tx, tx,
                    origin_for( status_tx.size() ) );
      if( state.user_requests_input ) {
        auto const& mod_tx  = render_text( "?", Color::green() );
        auto        mod_pos = state.where;
        mod_pos.y -= mod_tx.size().h;
        copy_texture( mod_tx, tx, mod_pos - state.click_offset );
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

waitable<> dragging_thread( Entities*             entities,
                            input::e_mouse_button button,
                            Coord                 origin ) {
  // Must check first if there is anything to drag. If this is
  // not the start of a valid drag then we must return immedi-
  // ately without co_awaiting on anything.
  if( button != input::e_mouse_button::l ) co_return;
  maybe<OldWorldDragSrcInfo> src_info =
      drag_src_from_coord( origin, entities );
  if( !src_info ) co_return;
  OldWorldDragSrc_t& src = src_info->src;

  // Now we have a valid drag that has initiated.
  g_drag_state = drag::State<OldWorldDraggableObject_t>{
      .stream              = {},
      .object              = draggable_from_src( src ),
      .indicator           = drag::e_status_indicator::none,
      .user_requests_input = false,
      .where               = origin,
      .click_offset        = origin - src_info->rect.center() };
  SCOPE_EXIT( g_drag_state = nothing );
  auto& state = *g_drag_state;

  drag::Step               latest;
  maybe<OldWorldDragDst_t> dst;
  while( maybe<drag::Step> d = co_await state.stream.next() ) {
    latest      = *d;
    state.where = d->current;
    dst         = drag_dst_from_coord( entities, d->current );
    if( !dst ) {
      state.indicator = drag::e_status_indicator::none;
      continue;
    }
    if( !DragConnector::visit( entities, src, *dst ) ) {
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
      proceed =
          co_await DragUserInput::visit( entities, &src, &*dst );
    if( proceed ) {
      DragPerform::visit( entities, src, *dst );
      // Now that we've potentially changed the ownership of some
      // units, we need to recreate the entities otherwise we'll
      // potentially have one frame where the dragged unit is
      // back in its original position before moving to where it
      // was dragged.
      create_entities( entities );
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

/****************************************************************
** The Old World Plane
*****************************************************************/
struct OldWorldPlane : public Plane {
  OldWorldPlane() = default;
  bool covers_screen() const override { return true; }

  void draw( Texture& tx ) const override {
    clear_texture_transparent( tx );
    draw_entities( tx, entities_ );
    // Should be last.
    drag_n_drop_draw( tx );
  }

  e_input_handled input( input::event_t const& event ) override {
    // If there is a drag happening then the user's input should
    // not be needed for anything other than the drag.
    if( g_drag_state ) {
      drag_n_drop_handle_input( event, g_drag_state->stream );
      return e_input_handled::yes;
    }
    switch( event.to_enum() ) {
      case input::e_input_event::unknown_event:
        return e_input_handled::no;
      case input::e_input_event::quit_event:
        return e_input_handled::no;
      case input::e_input_event::win_event:
        create_entities( &entities_ );
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
            g_exit_promise.set_value_emplace();
            return e_input_handled::yes;
          }
        }

        // Unit selection.
        auto handled         = e_input_handled::no;
        auto try_select_unit = [&]( auto const& maybe_entity ) {
          if( maybe_entity ) {
            if( auto maybe_pair =
                    maybe_entity->obj_under_cursor( val.pos );
                maybe_pair ) {
              SG().selected_unit = maybe_pair->first;
              handled            = e_input_handled::yes;
              create_entities( &entities_ );
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
    if( g_drag_state ) return Plane::e_accept_drag::swallow;
    waitable<> w = dragging_thread( &entities_, button, origin );
    if( w.ready() ) return e_accept_drag::no;
    g_drag_thread = std::move( w );
    return e_accept_drag::yes;
  }

  void on_drag( input::mod_keys const& mod,
                input::e_mouse_button /*button*/,
                Coord /*origin*/, Coord /*prev*/,
                Coord current ) override {
    CHECK( g_drag_state );
    g_drag_state->stream.send(
        drag::Step{ .mod = mod, .current = current } );
  }

  void on_drag_finished( input::mod_keys const& /*mod*/,
                         input::e_mouse_button /*button*/,
                         Coord /*origin*/,
                         Coord /*end*/ ) override {
    CHECK( g_drag_state );
    g_drag_state->stream.finish();
    // At this point we assume that the callback will finish on
    // its own after doing any post-drag stuff it needs to do. No
    // new drags can start until then.
  }

  // ------------------------------------------------------------
  // Members
  // ------------------------------------------------------------
  Entities entities_{};
};

OldWorldPlane g_old_world_plane;

/****************************************************************
** Initialization / Cleanup
*****************************************************************/
void init_old_world_view() {}

void cleanup_old_world_view() { g_drag_thread = nothing; }

REGISTER_INIT_ROUTINE( old_world_view );

/****************************************************************
** Main Thread
*****************************************************************/
waitable<> run_old_world_view() {
  create_entities( &g_old_world_plane.entities_ );
  // TODO: how does this thread interact with the dragging
  // thread? It should probably somehow co_await on it when a
  // drag happens.
  co_await g_exit_promise.waitable();
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
waitable<> show_old_world_view() {
  g_exit_promise = {};
  if( SG().selected_unit ) {
    UnitId id = *SG().selected_unit;
    // We could have a case where the unit that was last selected
    // went to the new world and was then disbanded, or is just
    // no longer in the old world.
    if( !unit_exists( id ) || !unit_old_world_view_info( id ) )
      SG().selected_unit = nothing;
  }
  ScopedPlanePush pusher( e_plane_config::old_world );
  lg.info( "entering old world view." );
  co_await run_old_world_view();
  lg.info( "leaving old world view." );
}

void old_world_view_set_selected_unit( UnitId id ) {
  // Ensure that the unit is either in port or on the high seas,
  // otherwise it doesn't make sense for the unit to be selected
  // on this screen.
  CHECK( unit_old_world_view_info( id ) );
  SG().selected_unit = id;
}

Plane* old_world_plane() { return &g_old_world_plane; }

/****************************************************************
** Menu Handlers
*****************************************************************/

MENU_ITEM_HANDLER(
    old_world_close, [] { g_exit_promise.set_value_emplace(); },
    [] { return is_plane_enabled( e_plane::old_world ); } )

} // namespace rn