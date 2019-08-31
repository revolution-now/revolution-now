/****************************************************************
**europort-view.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-06-14.
*
* Description: Implements the Europe port view.
*
*****************************************************************/
#include "europort-view.hpp"

// Revolution Now
#include "cargo.hpp"
#include "commodity.hpp"
#include "compositor.hpp"
#include "coord.hpp"
#include "dragdrop.hpp"
#include "europort.hpp"
#include "fmt-helper.hpp"
#include "gfx.hpp"
#include "image.hpp"
#include "init.hpp"
#include "input.hpp"
#include "logging.hpp"
#include "menu.hpp"
#include "ownership.hpp"
#include "plane.hpp"
#include "ranges.hpp"
#include "render.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "tiles.hpp"
#include "variant.hpp"

// base-util
#include "base-util/optional.hpp"

// Range-v3
#include "range/v3/view/all.hpp"
#include "range/v3/view/iota.hpp"
#include "range/v3/view/transform.hpp"
#include "range/v3/view/zip.hpp"

using namespace std;
using namespace util::infix;

namespace rn {

namespace {

constexpr Delta const k_rendered_commodity_offset{8_w, 3_h};

// When we drag a commodity from the market this is the default
// amount that we take.
constexpr int const k_default_market_quantity = 30;

/****************************************************************
** Selected Unit
*****************************************************************/
// Global State. This can be any ship that is visible on the eu-
// rope view.
Opt<UnitId> g_selected_unit;

/****************************************************************
** Draggable Object
*****************************************************************/
adt_rn_( DraggableObject,             //
         ( unit,                      //
           ( UnitId, id ) ),          //
         ( market_commodity,          //
           ( e_commodity, type ) ),   //
         ( cargo_commodity,           //
           ( Commodity, comm ),       //
           ( CargoSlotIndex, slot ) ) //
);

static_assert( std::is_copy_constructible_v<DraggableObject_t> );

// Global State.
Opt<DraggableObject_t> g_dragging_object;

Opt<DraggableObject_t> cargo_slot_to_draggable(
    CargoSlotIndex slot_idx, CargoSlot_t const& slot ) {
  return matcher_( slot, ->, Opt<DraggableObject_t> ) {
    case_( CargoSlot::empty ) result_    nullopt;
    case_( CargoSlot::overflow ) result_ nullopt;
    case_( CargoSlot::cargo, contents ) {
      return matcher_( contents, ->, DraggableObject_t ) {
        case_( UnitId ) {
          return DraggableObject::unit{/*id=*/val};
        }
        case_( Commodity ) {
          return DraggableObject::cargo_commodity{
              /*comm=*/val,
              /*slot=*/slot_idx};
        }
        matcher_exhaustive;
      }
    }
    matcher_exhaustive;
  }
}

Opt<Cargo> draggable_to_cargo_object(
    DraggableObject_t const& draggable ) {
  return matcher_( draggable, ->, Opt<Cargo> ) {
    case_( DraggableObject::unit ) return val.id;
    case_( DraggableObject::market_commodity ) return nullopt;
    case_( DraggableObject::cargo_commodity ) return val.comm;
    matcher_exhaustive;
  }
}

Opt<DraggableObject_t> draggable_in_cargo_slot(
    CargoSlotIndex slot ) {
  return g_selected_unit                                 //
         | fmap( unit_from_id )                          //
         | fmap_join( LC( _.get().cargo().at( slot ) ) ) //
         | fmap_join( LC( cargo_slot_to_draggable( slot, _ ) ) );
}

Texture draw_draggable_object(
    DraggableObject_t const& object ) {
  return matcher_( object ) {
    case_( DraggableObject::unit, id ) {
      auto tx = create_texture_transparent(
          lookup_sprite( unit_from_id( id ).desc().tile )
              .size() );
      render_unit( tx, id, Coord{}, /*with_icon=*/false );
      return tx;
    }
    case_( DraggableObject::market_commodity, type ) {
      return render_commodity_create( type );
    }
    case_( DraggableObject::cargo_commodity ) {
      return render_commodity_create( val.comm.type );
    }
    matcher_exhaustive;
  }
}

/****************************************************************
** The Clip Rect
*****************************************************************/
// Global State.
Delta g_clip;

Rect clip_rect() {
  return Rect::from(
      centered( g_clip,
                compositor::section(
                    compositor::e_section::non_menu_bar ) ),
      g_clip );
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
** Helpers
*****************************************************************/
// Both rv::all and the lambda will take rect_proxy by reference
// so we therefore must have this function take a reference to a
// rect_proxy that outlives the use of the returned range. And of
// course the Rect referred to by the rect_proxy must outlive
// everything.
auto range_of_rects(
    RectGridProxyIteratorHelper const& rect_proxy ) {
  return rv::all( rect_proxy ) |
         rv::transform( [&rect_proxy]( Coord coord ) {
           return Rect::from(
               coord, Delta{1_w, 1_h} * rect_proxy.scale() );
         } );
}

auto range_of_rects( RectGridProxyIteratorHelper&& ) = delete;

/****************************************************************
** Europe View Entities
*****************************************************************/
namespace entity {

// Each entity is defined by a struct that holds its state and
// that has the following methods:
//
//  void draw( Texture& tx, Delta offset ) const;
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
  static constexpr auto sprite_scale = Scale{32};

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
    return Rect::from(
        origin_,
        Delta{doubled_ ? double_layer_width : single_layer_width,
              doubled_ ? double_layer_height
                       : single_layer_height} );
  }

  void draw( Texture& tx, Delta offset ) const {
    auto bds     = bounds();
    auto grid    = bds.to_grid_noalign( sprite_scale );
    auto comm_it = values<e_commodity>.begin();
    auto label   = CommodityLabel::buy_sell{100, 200};
    for( auto rect : range_of_rects( grid ) ) {
      render_rect( tx, Color::white(),
                   rect.shifted_by( offset ) );
      render_commodity_annotated(
          tx, *comm_it++,
          rect.shifted_by( offset ).upper_left() +
              k_rendered_commodity_offset,
          label );
      label.buy += 120;
      label.sell += 120;
    }
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

  Opt<e_commodity> commodity_under_cursor(
      Coord const& coord ) const {
    Opt<e_commodity> res;
    if( coord.is_inside( bounds() ) ) {
      auto boxes =
          bounds().with_new_upper_left( Coord{} ) / sprite_scale;
      res = boxes.rasterize(
                coord.with_new_origin( bounds().upper_left() ) /
                sprite_scale ) //
            | fmap_join( commodity_from_index );
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

class ActiveCargoBox {
  static constexpr Delta size_blocks{6_w, 1_h};

public:
  // Commodities will be 24x24.
  static constexpr auto  box_scale   = Scale{32};
  static constexpr Delta size_pixels = size_blocks * box_scale;

  Rect bounds() const {
    return Rect::from( origin_, size_pixels );
  }

  void draw( Texture& tx, Delta offset ) const {
    auto bds  = bounds();
    auto grid = bds.to_grid_noalign( box_scale );
    for( auto rect : range_of_rects( grid ) )
      render_rect( tx, Color::white(),
                   rect.shifted_by( offset ) );
  }

  ActiveCargoBox( ActiveCargoBox&& ) = default;
  ActiveCargoBox& operator=( ActiveCargoBox&& ) = default;

  static Opt<ActiveCargoBox> create(
      Delta const&                  size,
      Opt<MarketCommodities> const& maybe_market_commodities ) {
    Opt<ActiveCargoBox> res;
    auto                rect = Rect::from( Coord{}, size );
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
                rect.center().x - size_pixels.w / 2_sx} );
      } else {
        // Possibly just for now do this.
        res = ActiveCargoBox(
            /*origin_=*/Coord{
                market_commodities.origin_.y - size_pixels.h +
                    1_h,
                rect.center().x - size_pixels.w / 2_sx} );
      }
    }
    return res;
  }

  Opt<CargoSlotIndex> slot_idx_from_coord( Coord coord ) const {
    Opt<CargoSlotIndex> res;
    if( coord.is_inside( bounds() ) ) {
      auto boxes =
          bounds().with_new_upper_left( Coord{} ) / box_scale;
      res = boxes.rasterize(
          coord.with_new_origin( bounds().upper_left() ) /
          box_scale );
    }
    return res;
  }

private:
  ActiveCargoBox() = default;
  ActiveCargoBox( Coord origin ) : origin_( origin ) {}
  Coord origin_{};
};

class DockAnchor {
  static constexpr Delta cross_leg_size{5_w, 5_h};
  static constexpr H     above_active_cargo_box{32_h};

public:
  Rect bounds() const {
    // Just a point.
    return Rect::from( location_, Delta{} );
  }

  void draw( Texture& tx, Delta offset ) const {
    // This mess just draws an X.
    render_line( tx, Color::white(),
                 location_ - cross_leg_size + offset,
                 cross_leg_size * Scale{2} + Delta{1_w, 1_h} );
    render_line(
        tx, Color::white(),
        location_ - cross_leg_size.mirrored_vertically() +
            offset,
        cross_leg_size.mirrored_vertically() * Scale{2} +
            Delta{1_w, -1_h} );
  }

  DockAnchor( DockAnchor&& ) = default;
  DockAnchor& operator=( DockAnchor&& ) = default;

  static Opt<DockAnchor> create(
      Delta const&                  size,
      Opt<ActiveCargoBox> const&    maybe_active_cargo_box,
      Opt<MarketCommodities> const& maybe_market_commodities ) {
    Opt<DockAnchor> res;
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

class Backdrop {
  static constexpr Delta image_distance_from_anchor{950_w,
                                                    544_h};

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

  static Opt<Backdrop> create(
      Delta const&           size,
      Opt<DockAnchor> const& maybe_dock_anchor ) {
    Opt<Backdrop> res;
    if( maybe_dock_anchor ) {
      res =
          Backdrop{-( maybe_dock_anchor->bounds().upper_left() -
                      image_distance_from_anchor ),
                   size};
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

class InPortBox {
public:
  static constexpr Delta block_size{32_w, 32_h};
  static constexpr SY    height_blocks{3};
  static constexpr SX    width_wide{3};
  static constexpr SX    width_narrow{2};

  Rect bounds() const {
    return Rect::from( origin_, block_size * size_in_blocks_ +
                                    Delta{1_w, 1_h} );
  }

  void draw( Texture& tx, Delta offset ) const {
    render_rect( tx, Color::white(),
                 bounds().shifted_by( offset ) );
    auto const& label_tx =
        render_text( "In Port", Color::white() );
    copy_texture(
        label_tx, tx,
        bounds().upper_left() + Delta{2_w, 2_h} + offset );
  }

  InPortBox( InPortBox&& ) = default;
  InPortBox& operator=( InPortBox&& ) = default;

  static Opt<InPortBox> create(
      Delta const&                  size,
      Opt<ActiveCargoBox> const&    maybe_active_cargo_box,
      Opt<MarketCommodities> const& maybe_market_commodities ) {
    Opt<InPortBox> res;
    if( maybe_active_cargo_box && maybe_market_commodities ) {
      bool  is_wide = !maybe_market_commodities->doubled_;
      Scale size_in_blocks;
      size_in_blocks.sy = height_blocks;
      size_in_blocks.sx = is_wide ? width_wide : width_narrow;
      auto origin =
          maybe_active_cargo_box->bounds().upper_left() -
          block_size.h * size_in_blocks.sy;
      if( origin.y < 0_y || origin.x < 0_x ) return res;

      res = InPortBox{origin,         //
                      size_in_blocks, //
                      is_wide};

      auto lr_delta = res->bounds().lower_right() - Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h )
        res = nullopt;
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

class InboundBox {
public:
  Rect bounds() const {
    return Rect::from( origin_,
                       InPortBox::block_size * size_in_blocks_ +
                           Delta{1_w, 1_h} );
  }

  void draw( Texture& tx, Delta offset ) const {
    render_rect( tx, Color::white(),
                 bounds().shifted_by( offset ) );
    auto const& label_tx =
        render_text( "Inbound", Color::white() );
    copy_texture(
        label_tx, tx,
        bounds().upper_left() + Delta{2_w, 2_h} + offset );
  }

  InboundBox( InboundBox&& ) = default;
  InboundBox& operator=( InboundBox&& ) = default;

  static Opt<InboundBox> create(
      Delta const&          size,
      Opt<InPortBox> const& maybe_in_port_box ) {
    Opt<InboundBox> res;
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
        res = nullopt;
      if( res->bounds().y < 0_y ) res = nullopt;
      if( res->bounds().x < 0_x ) res = nullopt;
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

class OutboundBox {
public:
  Rect bounds() const {
    return Rect::from( origin_,
                       InPortBox::block_size * size_in_blocks_ +
                           Delta{1_w, 1_h} );
  }

  void draw( Texture& tx, Delta offset ) const {
    render_rect( tx, Color::white(),
                 bounds().shifted_by( offset ) );
    auto const& label_tx =
        render_text( "Outbound", Color::white() );
    copy_texture(
        label_tx, tx,
        bounds().upper_left() + Delta{2_w, 2_h} + offset );
  }

  OutboundBox( OutboundBox&& ) = default;
  OutboundBox& operator=( OutboundBox&& ) = default;

  static Opt<OutboundBox> create(
      Delta const&           size,
      Opt<InboundBox> const& maybe_inbound_box ) {
    Opt<OutboundBox> res;
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
        res = nullopt;
      if( res->bounds().y < 0_y ) res = nullopt;
      if( res->bounds().x < 0_x ) res = nullopt;
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

class Exit {
  static constexpr Delta exit_block_pixels{24_w, 24_h};

public:
  Rect bounds() const {
    return Rect::from( origin_, exit_block_pixels ) +
           Delta{2_w, 2_h};
  }

  void draw( Texture& tx, Delta offset ) const {
    auto bds = bounds().with_inc_size();
    bds      = bds.shifted_by( Delta{-2_w, -2_h} );
    auto const& exit_tx =
        render_text( font::standard(), Color::red(), "Exit" );
    auto drawing_origin = centered( exit_tx.size(), bds );
    copy_texture( exit_tx, tx, drawing_origin + offset );
    render_rect( tx, Color::white(), bds.shifted_by( offset ) );
  }

  Exit( Exit&& ) = default;
  Exit& operator=( Exit&& ) = default;

  static Opt<Exit> create(
      Delta const&                  size,
      Opt<MarketCommodities> const& maybe_market_commodities ) {
    Opt<Exit> res;
    if( maybe_market_commodities ) {
      auto origin =
          maybe_market_commodities->bounds().lower_right() -
          Delta{1_w, 1_h} - exit_block_pixels.h;
      auto lr_delta = origin + exit_block_pixels - Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h ) {
        origin =
            maybe_market_commodities->bounds().upper_right() -
            1_w - exit_block_pixels;
      }
      res = Exit{origin};
      lr_delta =
          ( res->bounds().lower_right() - Delta{1_w, 1_h} ) -
          Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h )
        res = nullopt;
      if( res->bounds().y < 0_y ) res = nullopt;
      if( res->bounds().x < 0_x ) res = nullopt;
    }
    return res;
  }

private:
  Exit() = default;
  Exit( Coord origin ) : origin_( origin ) {}
  Coord origin_{};
};

class Dock {
  static constexpr Scale dock_block_pixels{24};

public:
  Rect bounds() const {
    return Rect::from(
        origin_, Delta{length_in_blocks_ * dock_block_pixels.sx,
                       1_h * dock_block_pixels.sy} );
  }

  void draw( Texture& tx, Delta offset ) const {
    auto bds  = bounds();
    auto grid = bds.to_grid_noalign( dock_block_pixels );
    for( auto rect : range_of_rects( grid ) )
      render_rect( tx, Color::white(),
                   rect.shifted_by( offset ) );
  }

  Dock( Dock&& ) = default;
  Dock& operator=( Dock&& ) = default;

  static Opt<Dock> create(
      Delta const&           size,
      Opt<DockAnchor> const& maybe_dock_anchor,
      Opt<InPortBox> const&  maybe_in_port_box ) {
    Opt<Dock> res;
    if( maybe_dock_anchor && maybe_in_port_box ) {
      auto available = maybe_dock_anchor->bounds().left_edge() -
                       maybe_in_port_box->bounds().right_edge();
      available /= dock_block_pixels.sx;
      auto origin = maybe_dock_anchor->bounds().upper_left() -
                    ( available * dock_block_pixels.sx );
      origin -= 1_h * dock_block_pixels.sy / 2;
      res = Dock{/*origin_=*/origin,
                 /*length_in_blocks_=*/available};
      auto lr_delta =
          ( res->bounds().lower_right() - Delta{1_w, 1_h} ) -
          Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h )
        res = nullopt;
      if( res->bounds().y < 0_y ) res = nullopt;
      if( res->bounds().x < 0_x ) res = nullopt;
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

// Base class for other entities that just consist of a collec-
// tion of units.
class UnitCollection {
public:
  Rect bounds() const {
    auto uni0n = L2( _1.uni0n( _2 ) );
    auto to_rect =
        L( Rect::from( _.pixel_coord, g_tile_delta ) );
    auto maybe_rect = accumulate_monoid(
        units_ | rv::transform( to_rect ), uni0n );
    return maybe_rect.value_or( bounds_when_no_units_ );
  }

  void draw( Texture& tx, Delta offset ) const {
    // auto bds = bounds();
    // render_rect( tx, Color::white(), bds.shifted_by( offset )
    // );
    for( auto const& unit_with_pos : units_ )
      if( g_dragging_object !=
          DraggableObject_t{
              DraggableObject::unit{unit_with_pos.id}} )
        render_unit( tx, unit_with_pos.id,
                     unit_with_pos.pixel_coord + offset,
                     /*with_icon=*/false );
    if( g_selected_unit ) {
      for( auto [id, coord] : units_ ) {
        if( id == *g_selected_unit ) {
          render_rect( tx, Color::green(),
                       Rect::from( coord, g_tile_delta )
                           .shifted_by( offset ) );
          break;
        }
      }
    }
  }

  Opt<UnitId> unit_under_cursor( Coord pos ) const {
    Opt<UnitId> res;
    for( auto [id, coord] : units_ ) {
      if( pos.is_inside( Rect::from( coord, g_tile_delta ) ) ) {
        res = id;
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

class UnitsOnDock : public UnitCollection {
public:
  UnitsOnDock( UnitsOnDock&& ) = default;
  UnitsOnDock& operator=( UnitsOnDock&& ) = default;

  static Opt<UnitsOnDock> create(
      Delta const&           size,
      Opt<DockAnchor> const& maybe_dock_anchor,
      Opt<Dock> const&       maybe_dock ) {
    Opt<UnitsOnDock> res;
    if( maybe_dock_anchor && maybe_dock ) {
      vector<UnitWithPosition> units;
      Coord                    coord =
          maybe_dock->bounds().upper_right() - g_tile_delta;
      for( auto id : europort_units_on_dock() ) {
        units.push_back( {id, coord} );
        coord -= g_tile_delta.w;
        if( coord.x < maybe_dock->bounds().left_edge() )
          coord = Coord{( maybe_dock->bounds().upper_right() -
                          g_tile_delta )
                            .x,
                        coord.y - g_tile_delta.h};
      }
      // populate units...
      res = UnitsOnDock{
          /*bounds_when_no_units_=*/maybe_dock_anchor->bounds(),
          /*units_=*/std::move( units )};
      auto bds = res->bounds();
      auto lr_delta =
          ( bds.lower_right() - Delta{1_w, 1_h} ) - Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h )
        res = nullopt;
      if( bds.y < 0_y ) res = nullopt;
      if( bds.x < 0_x ) res = nullopt;
    }
    return res;
  }

private:
  UnitsOnDock( Rect                       dock_anchor,
               vector<UnitWithPosition>&& units )
    : UnitCollection( dock_anchor, std::move( units ) ) {}
};

class ShipsInPort : public UnitCollection {
public:
  ShipsInPort( ShipsInPort&& ) = default;
  ShipsInPort& operator=( ShipsInPort&& ) = default;

  static Opt<ShipsInPort> create(
      Delta const&          size,
      Opt<InPortBox> const& maybe_in_port_box ) {
    Opt<ShipsInPort> res;
    if( maybe_in_port_box ) {
      vector<UnitWithPosition> units;
      auto  in_port_bds = maybe_in_port_box->bounds();
      Coord coord = in_port_bds.lower_right() - g_tile_delta;
      for( auto id : europort_units_in_port() ) {
        units.push_back( {id, coord} );
        coord -= g_tile_delta.w;
        if( coord.x < in_port_bds.left_edge() )
          coord = Coord{
              ( in_port_bds.upper_right() - g_tile_delta ).x,
              coord.y - g_tile_delta.h};
      }
      // populate units...
      res = ShipsInPort{/*bounds_when_no_units_=*/Rect::from(
                            in_port_bds.lower_right(), Delta{} ),
                        /*units_=*/std::move( units )};
      auto bds = res->bounds();
      auto lr_delta =
          ( bds.lower_right() - Delta{1_w, 1_h} ) - Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h )
        res = nullopt;
      if( bds.y < 0_y ) res = nullopt;
      if( bds.x < 0_x ) res = nullopt;
    }
    return res;
  }

private:
  ShipsInPort( Rect                       dock_anchor,
               vector<UnitWithPosition>&& units )
    : UnitCollection( dock_anchor, std::move( units ) ) {}
};

class ShipsInbound : public UnitCollection {
public:
  ShipsInbound( ShipsInbound&& ) = default;
  ShipsInbound& operator=( ShipsInbound&& ) = default;

  static Opt<ShipsInbound> create(
      Delta const&           size,
      Opt<InboundBox> const& maybe_inbound_box ) {
    Opt<ShipsInbound> res;
    if( maybe_inbound_box ) {
      vector<UnitWithPosition> units;
      auto  frame_bds = maybe_inbound_box->bounds();
      Coord coord     = frame_bds.lower_right() - g_tile_delta;
      for( auto id : europort_units_inbound() ) {
        units.push_back( {id, coord} );
        coord -= g_tile_delta.w;
        if( coord.x < frame_bds.left_edge() )
          coord =
              Coord{( frame_bds.upper_right() - g_tile_delta ).x,
                    coord.y - g_tile_delta.h};
      }
      // populate units...
      res = ShipsInbound{/*bounds_when_no_units_=*/Rect::from(
                             frame_bds.lower_right(), Delta{} ),
                         /*units_=*/std::move( units )};
      auto bds = res->bounds();
      auto lr_delta =
          ( bds.lower_right() - Delta{1_w, 1_h} ) - Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h )
        res = nullopt;
      if( bds.y < 0_y ) res = nullopt;
      if( bds.x < 0_x ) res = nullopt;
    }
    return res;
  }

private:
  ShipsInbound( Rect                       dock_anchor,
                vector<UnitWithPosition>&& units )
    : UnitCollection( dock_anchor, std::move( units ) ) {}
};

class ShipsOutbound : public UnitCollection {
public:
  ShipsOutbound( ShipsOutbound&& ) = default;
  ShipsOutbound& operator=( ShipsOutbound&& ) = default;

  static Opt<ShipsOutbound> create(
      Delta const&            size,
      Opt<OutboundBox> const& maybe_outbound_box ) {
    Opt<ShipsOutbound> res;
    if( maybe_outbound_box ) {
      vector<UnitWithPosition> units;
      auto  frame_bds = maybe_outbound_box->bounds();
      Coord coord     = frame_bds.lower_right() - g_tile_delta;
      for( auto id : europort_units_outbound() ) {
        units.push_back( {id, coord} );
        coord -= g_tile_delta.w;
        if( coord.x < frame_bds.left_edge() )
          coord =
              Coord{( frame_bds.upper_right() - g_tile_delta ).x,
                    coord.y - g_tile_delta.h};
      }
      // populate units...
      res = ShipsOutbound{/*bounds_when_no_units_=*/Rect::from(
                              frame_bds.lower_right(), Delta{} ),
                          /*units_=*/std::move( units )};
      auto bds = res->bounds();
      auto lr_delta =
          ( bds.lower_right() - Delta{1_w, 1_h} ) - Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h )
        res = nullopt;
      if( bds.y < 0_y ) res = nullopt;
      if( bds.x < 0_x ) res = nullopt;
    }
    return res;
  }

private:
  ShipsOutbound( Rect                       dock_anchor,
                 vector<UnitWithPosition>&& units )
    : UnitCollection( dock_anchor, std::move( units ) ) {}
};

class ActiveCargo {
public:
  Rect bounds() const { return bounds_; }

  void draw( Texture& tx, Delta offset ) const {
    auto bds  = bounds();
    auto grid = bds.to_grid_noalign( ActiveCargoBox::box_scale );
    if( maybe_active_unit_ ) {
      auto&       unit = unit_from_id( *maybe_active_unit_ );
      auto const& cargo_slots = unit.cargo().slots();
      for( auto const& [idx, cargo_slot, rect] :
           rv::zip( rv::ints, cargo_slots,
                    range_of_rects( grid ) ) ) {
        if( g_dragging_object.has_value() ) {
          if_v( *g_dragging_object,
                DraggableObject::cargo_commodity, cc ) {
            if( cc->slot._ == idx ) continue;
          }
        }
        auto dst_coord       = rect.upper_left() + offset;
        auto cargo_slot_copy = cargo_slot;
        switch_( cargo_slot_copy ) {
          case_( CargoSlot::empty ) {}
          case_( CargoSlot::overflow ) {}
          case_( CargoSlot::cargo ) {
            switch_( val.contents ) {
              case_( UnitId ) {
                if( g_dragging_object !=
                    DraggableObject_t{
                        DraggableObject::unit{val}} )
                  render_unit( tx, val, dst_coord,
                               /*with_icon=*/false );
              }
              case_( Commodity ) {
                render_commodity_annotated(
                    tx, val,
                    dst_coord + k_rendered_commodity_offset );
              }
              switch_exhaustive;
            }
          }
          switch_exhaustive;
        }
      }
      for( auto [idx, rect] :
           rv::zip( rv::ints, range_of_rects( grid ) ) ) {
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

  static Opt<ActiveCargo> create(
      Delta const&               size,
      Opt<ActiveCargoBox> const& maybe_active_cargo_box,
      Opt<ShipsInPort> const&    maybe_ships_in_port ) {
    Opt<ActiveCargo> res;
    if( maybe_active_cargo_box && maybe_ships_in_port ) {
      res = ActiveCargo{
          /*maybe_active_unit_=*/g_selected_unit,
          /*bounds_=*/maybe_active_cargo_box->bounds()};
      auto lr_delta =
          ( res->bounds().lower_right() - Delta{1_w, 1_h} ) -
          Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h )
        res = nullopt;
      if( res->bounds().y < 0_y ) res = nullopt;
      if( res->bounds().x < 0_x ) res = nullopt;
    }
    return res;
  }

  Opt<CargoSlotIndex> slot_idx_from_coord( Coord coord ) const {
    Opt<CargoSlotIndex> res;
    if( maybe_active_unit_ ) {
      if( coord.is_inside( bounds_ ) ) {
        auto boxes = bounds_.with_new_upper_left( Coord{} ) /
                     ActiveCargoBox::box_scale;
        res = boxes.rasterize(
            coord.with_new_origin( bounds_.upper_left() ) /
            ActiveCargoBox::box_scale );
      }
      auto& unit = unit_from_id( *maybe_active_unit_ );
      if( res && *res >= unit.cargo().slots_total() )
        res = nullopt;
    }
    return res;
  }

  Opt<CRef<CargoSlot_t>> cargo_slot_from_coord(
      Coord coord ) const {
    // Lambda will only be called if a valid index is returned,
    // in which case there is guaranteed to be an active unit.
    return slot_idx_from_coord( coord ) //
           | fmap( LC( unit_from_id( *maybe_active_unit_ )
                           .cargo()[_] ) );
  }

  Opt<UnitId> active_unit() const { return maybe_active_unit_; }

private:
  ActiveCargo() = default;
  ActiveCargo( Opt<UnitId> maybe_active_unit, Rect bounds )
    : maybe_active_unit_( maybe_active_unit ),
      bounds_( bounds ) {}
  Opt<UnitId> maybe_active_unit_;
  Rect        bounds_;
};

} // namespace entity

//- Buttons
//- Message box
//- Stats area (money, tax rate, etc.)

struct Entities {
  Opt<entity::MarketCommodities> market_commodities;
  Opt<entity::ActiveCargoBox>    active_cargo_box;
  Opt<entity::DockAnchor>        dock_anchor;
  Opt<entity::Backdrop>          backdrop;
  Opt<entity::InPortBox>         in_port_box;
  Opt<entity::InboundBox>        inbound_box;
  Opt<entity::OutboundBox>       outbound_box;
  Opt<entity::Exit>              exit_label;
  Opt<entity::Dock>              dock;
  Opt<entity::UnitsOnDock>       units_on_dock;
  Opt<entity::ShipsInPort>       ships_in_port;
  Opt<entity::ShipsInbound>      ships_inbound;
  Opt<entity::ShipsOutbound>     ships_outbound;
  Opt<entity::ActiveCargo>       active_cargo;
};

void create_entities( Entities* entities ) {
  using namespace entity;
  entities->market_commodities = //
      MarketCommodities::create( g_clip );
  entities->active_cargo_box =        //
      ActiveCargoBox::create( g_clip, //
                              entities->market_commodities );
  entities->dock_anchor =                             //
      DockAnchor::create( g_clip,                     //
                          entities->active_cargo_box, //
                          entities->market_commodities );
  entities->backdrop =          //
      Backdrop::create( g_clip, //
                        entities->dock_anchor );
  entities->in_port_box =                            //
      InPortBox::create( g_clip,                     //
                         entities->active_cargo_box, //
                         entities->market_commodities );
  entities->inbound_box =         //
      InboundBox::create( g_clip, //
                          entities->in_port_box );
  entities->outbound_box =         //
      OutboundBox::create( g_clip, //
                           entities->inbound_box );
  entities->exit_label =    //
      Exit::create( g_clip, //
                    entities->market_commodities );
  entities->dock =                         //
      Dock::create( g_clip,                //
                    entities->dock_anchor, //
                    entities->in_port_box );
  entities->units_on_dock =                       //
      UnitsOnDock::create( g_clip,                //
                           entities->dock_anchor, //
                           entities->dock );
  entities->ships_in_port =        //
      ShipsInPort::create( g_clip, //
                           entities->in_port_box );
  entities->ships_inbound =         //
      ShipsInbound::create( g_clip, //
                            entities->inbound_box );
  entities->ships_outbound =         //
      ShipsOutbound::create( g_clip, //
                             entities->outbound_box );
  entities->active_cargo =                             //
      ActiveCargo::create( g_clip,                     //
                           entities->active_cargo_box, //
                           entities->ships_in_port );
}

void draw_entities( Texture& tx, Entities const& entities ) {
  auto offset = clip_rect().upper_left().distance_from_origin();
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
adt_rn_( DragSrc,                      //
         ( dock,                       //
           ( UnitId, id ) ),           //
         ( cargo,                      //
           ( CargoSlotIndex, slot ) ), //
         ( outbound,                   //
           ( UnitId, id ) ),           //
         ( inbound,                    //
           ( UnitId, id ) ),           //
         ( inport,                     //
           ( UnitId, id ) ),           //
         ( market,                     //
           ( e_commodity, type ) )     //
);

adt_rn_( DragDst,                      //
         ( cargo,                      //
           ( CargoSlotIndex, slot ) ), //
         ( dock ),                     //
         ( outbound ),                 //
         ( inbound ),                  //
         ( inport ),                   //
         ( inport_ship,                //
           ( UnitId, id ) ),           //
         ( market )                    //
);

adt_rn_( DragArc,                           //
         ( dock_to_cargo,                   //
           ( DragSrc::dock, src ),          //
           ( DragDst::cargo, dst ) ),       //
         ( cargo_to_dock,                   //
           ( DragSrc::cargo, src ),         //
           ( DragDst::dock, dst ) ),        //
         ( cargo_to_cargo,                  //
           ( DragSrc::cargo, src ),         //
           ( DragDst::cargo, dst ) ),       //
         ( outbound_to_inbound,             //
           ( DragSrc::outbound, src ),      //
           ( DragDst::inbound, dst ) ),     //
         ( outbound_to_inport,              //
           ( DragSrc::outbound, src ),      //
           ( DragDst::inport, dst ) ),      //
         ( inbound_to_outbound,             //
           ( DragSrc::inbound, src ),       //
           ( DragDst::outbound, dst ) ),    //
         ( inport_to_outbound,              //
           ( DragSrc::inport, src ),        //
           ( DragDst::outbound, dst ) ),    //
         ( dock_to_inport_ship,             //
           ( DragSrc::dock, src ),          //
           ( DragDst::inport_ship, dst ) ), //
         ( cargo_to_inport_ship,            //
           ( DragSrc::cargo, src ),         //
           ( DragDst::inport_ship, dst ) ), //
         ( market_to_cargo,                 //
           ( DragSrc::market, src ),        //
           ( DragDst::cargo, dst ) ),       //
         ( market_to_inport_ship,           //
           ( DragSrc::market, src ),        //
           ( DragDst::inport_ship, dst ) ), //
         ( cargo_to_market,                 //
           ( DragSrc::cargo, src ),         //
           ( DragDst::market, dst ) ),      //
);

class EuroViewDragAndDrop
  : public DragAndDrop<EuroViewDragAndDrop, DraggableObject_t,
                       DragSrc_t, DragDst_t, DragArc_t> {
public:
  EuroViewDragAndDrop( Entities const* entities )
    : entities_( entities ) {
    CHECK( entities );
  }

  DraggableObject_t draggable_from_src(
      DragSrc_t const& drag_src ) const {
    return matcher_( drag_src, ->, DraggableObject_t ) {
      case_( DragSrc::dock, id ) {
        return DraggableObject::unit{id};
      }
      case_( DragSrc::cargo, slot ) {
        // Not all cargo slots must have an item in them, but in
        // this case the slot should otherwise the DragSrc object
        // should never have been created.
        ASSIGN_CHECK_OPT( object,
                          draggable_in_cargo_slot( slot ) );
        return object;
      }
      case_( DragSrc::outbound, id ) {
        return DraggableObject::unit{id};
      }
      case_( DragSrc::inbound, id ) {
        return DraggableObject::unit{id};
      }
      case_( DragSrc::inport, id ) {
        return DraggableObject::unit{id};
      }
      case_( DragSrc::market, type ) {
        return DraggableObject::market_commodity{type};
      }
      matcher_exhaustive;
    }
  }

  constexpr static auto const draw_dragged_item =
      L( draw_draggable_object( _ ) );

  // Coord is relative to clip_rect for now.
  Opt<DragSrc_t> drag_src( Coord const& do_not_use ) const {
    using namespace entity;
    auto coord =
        do_not_use.with_new_origin( clip_rect().upper_left() );
    Opt<DragSrc_t> res;
    if( entities_->units_on_dock.has_value() ) {
      if( auto maybe_id =
              entities_->units_on_dock->unit_under_cursor(
                  coord );
          maybe_id )
        res = DragSrc::dock{
            /*id=*/*maybe_id //
        };
    }
    if( entities_->active_cargo.has_value() ) {
      auto const& active_cargo = *entities_->active_cargo;
      if( active_cargo.active_unit()    //
              | fmap( is_unit_in_port ) //
              | maybe_truish_to_bool    //
          &&
          util::just( coord ) //
              | fmap_join( LC(
                    active_cargo.slot_idx_from_coord( _ ) ) ) //
              | fmap_join( draggable_in_cargo_slot ) ) {
        res = DragSrc::cargo{
            /*slot=*/*active_cargo.slot_idx_from_coord(
                coord ) //
        };
      }
    }
    if( entities_->ships_outbound.has_value() ) {
      if( auto maybe_id =
              entities_->ships_outbound->unit_under_cursor(
                  coord );
          maybe_id ) {
        res = DragSrc::outbound{
            /*id=*/*maybe_id //
        };
      }
    }
    if( entities_->ships_inbound.has_value() ) {
      if( auto maybe_id =
              entities_->ships_inbound->unit_under_cursor(
                  coord );
          maybe_id ) {
        res = DragSrc::inbound{
            /*id=*/*maybe_id //
        };
      }
    }
    if( entities_->ships_in_port.has_value() ) {
      if( auto maybe_id =
              entities_->ships_in_port->unit_under_cursor(
                  coord );
          maybe_id ) {
        res = DragSrc::inport{
            /*id=*/*maybe_id //
        };
      }
    }
    if( entities_->market_commodities.has_value() ) {
      if( auto maybe_type =
              entities_->market_commodities
                  ->commodity_under_cursor( coord );
          maybe_type ) {
        res = DragSrc::market{
            /*type=*/*maybe_type //
        };
      }
    }
    return res;
  }

  // Coord is relative to clip_rect for now. Important: in this
  // function we should not return early; we should check all the
  // entities (in order) to allow later ones to override earlier
  // ones.
  Opt<DragDst_t> drag_dst( Coord const& do_not_use ) const {
    using namespace entity;
    auto coord =
        do_not_use.with_new_origin( clip_rect().upper_left() );
    Opt<DragDst_t> res;
    if( entities_->active_cargo.has_value() ) {
      if( auto maybe_slot =
              entities_->active_cargo->slot_idx_from_coord(
                  coord );
          maybe_slot ) {
        res = DragDst::cargo{
            /*slot=*/*maybe_slot //
        };
      }
    }
    if( entities_->dock.has_value() ) {
      if( coord.is_inside( entities_->dock->bounds() ) )
        res = DragDst::dock{};
    }
    if( entities_->units_on_dock.has_value() ) {
      if( coord.is_inside( entities_->units_on_dock->bounds() ) )
        res = DragDst::dock{};
    }
    if( entities_->outbound_box.has_value() ) {
      if( coord.is_inside( entities_->outbound_box->bounds() ) )
        res = DragDst::outbound{};
    }
    if( entities_->inbound_box.has_value() ) {
      if( coord.is_inside( entities_->inbound_box->bounds() ) )
        res = DragDst::inbound{};
    }
    if( entities_->in_port_box.has_value() ) {
      if( coord.is_inside( entities_->in_port_box->bounds() ) )
        res = DragDst::inport{};
    }
    if( entities_->ships_in_port.has_value() ) {
      if( auto maybe_ship =
              entities_->ships_in_port->unit_under_cursor(
                  coord );
          maybe_ship ) {
        res = DragDst::inport_ship{
            /*id=*/*maybe_ship, //
        };
      }
    }
    if( entities_->market_commodities.has_value() ) {
      if( coord.is_inside(
              entities_->market_commodities->bounds() ) )
        return DragDst::market{};
    }
    return res;
  }

  bool can_perform_drag( DragArc_t const& drag_arc ) const {
    return matcher_( drag_arc, ->, bool ) {
      case_( DragArc::dock_to_cargo, src, dst ) {
        ASSIGN_CHECK_OPT(
            ship, entities_->active_cargo |
                      fmap_join( L( _.active_unit() ) ) );
        if( !is_unit_in_port( ship ) ) return false;
        return unit_from_id( ship ).cargo().fits( src.id,
                                                  dst.slot._ );
      }
      case_( DragArc::cargo_to_dock ) {
        return util::holds<DraggableObject::unit>(
            draggable_from_src( val.src ) );
      }
      case_( DragArc::cargo_to_cargo, src, dst ) {
        ASSIGN_CHECK_OPT(
            ship, entities_->active_cargo |
                      fmap_join( L( _.active_unit() ) ) );
        if( !is_unit_in_port( ship ) ) return false;
        if( src.slot == dst.slot ) return true;
        ASSIGN_CHECK_OPT( cargo_object,
                          draggable_to_cargo_object(
                              draggable_from_src( src ) ) );
        return matcher_( cargo_object ) {
          case_( UnitId ) {
            return unit_from_id( ship )
                .cargo()
                .fits_with_item_removed(
                    /*cargo=*/cargo_object,   //
                    /*remove_slot=*/src.slot, //
                    /*insert_slot=*/dst.slot  //
                );
          }
          case_( Commodity ) {
            // If at least one quantity of the commodity can be
            // moved then we will allow (at least a partial
            // transfer) to proceed.
            auto size_one     = val;
            size_one.quantity = 1;
            return unit_from_id( ship ).cargo().fits(
                /*cargo=*/size_one,
                /*slot=*/dst.slot );
          }
          matcher_exhaustive;
        }
      }
      case_( DragArc::outbound_to_inbound ) { return true; }
      case_( DragArc::outbound_to_inport ) {
        ASSIGN_CHECK_OPT(
            info, unit_euro_port_view_info( val.src.id ) );
        ASSIGN_CHECK_V( outbound, info.get(),
                        UnitEuroPortViewState::outbound );
        return outbound.percent == 0.0;
      }
      case_( DragArc::inbound_to_outbound ) { return true; }
      case_( DragArc::inport_to_outbound ) { return true; }
      case_( DragArc::dock_to_inport_ship, src, dst ) {
        return unit_from_id( dst.id ).cargo().fits_somewhere(
            src.id );
      }
      case_( DragArc::cargo_to_inport_ship, src, dst ) {
        auto dst_ship = dst.id;
        ASSIGN_CHECK_OPT( cargo_object,
                          draggable_to_cargo_object(
                              draggable_from_src( src ) ) );
        return matcher_( cargo_object ) {
          case_( UnitId ) {
            if( is_unit_onboard( val ) == dst_ship )
              return false;
            return unit_from_id( dst_ship )
                .cargo()
                .fits_somewhere( val );
          }
          case_( Commodity ) {
            // If even 1 quantity can fit then we can proceed
            // with (at least) a partial transfer.
            auto size_one     = val;
            size_one.quantity = 1;
            return unit_from_id( dst_ship )
                .cargo()
                .fits_somewhere( size_one );
          }
          matcher_exhaustive;
        }
      }
      case_( DragArc::market_to_cargo, src, dst ) {
        ASSIGN_CHECK_OPT(
            ship, entities_->active_cargo |
                      fmap_join( L( _.active_unit() ) ) );
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
      case_( DragArc::market_to_inport_ship, src, dst ) {
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
      case_( DragArc::cargo_to_market ) {
        ASSIGN_CHECK_OPT(
            ship, entities_->active_cargo |
                      fmap_join( L( _.active_unit() ) ) );
        if( !is_unit_in_port( ship ) ) return false;
        return unit_from_id( ship )
            .cargo()
            .template slot_holds_cargo_type<Commodity>(
                val.src.slot._ )
            .has_value();
      }
      matcher_exhaustive;
    }
  }

  void perform_drag( DragArc_t const& drag_arc ) const {
    if( !can_perform_drag( drag_arc ) ) {
      lg.error(
          "Drag was marked as correct, but can_perform_drag "
          "returned false for arc: {}",
          drag_arc );
      DCHECK( false );
      return;
    }
    lg.debug( "performing drag: {}", drag_arc );
    // Beyond this point it is assumed that this drag is compat-
    // ible with game rules.
    switch_( drag_arc ) {
      case_( DragArc::dock_to_cargo, src, dst ) {
        ASSIGN_CHECK_OPT(
            ship, entities_->active_cargo |
                      fmap_join( L( _.active_unit() ) ) );
        ownership_change_to_cargo( ship, src.id, dst.slot._ );
      }
      case_( DragArc::cargo_to_dock ) {
        ASSIGN_CHECK_V( unit, draggable_from_src( val.src ),
                        DraggableObject::unit );
        unit_move_to_europort_dock( unit.id );
      }
      case_( DragArc::cargo_to_cargo, src, dst ) {
        ASSIGN_CHECK_OPT(
            ship, entities_->active_cargo |
                      fmap_join( L( _.active_unit() ) ) );
        ASSIGN_CHECK_OPT( cargo_object,
                          draggable_to_cargo_object(
                              draggable_from_src( src ) ) );
        switch_( cargo_object ) {
          case_( UnitId ) {
            // Will first "disown" unit which will remove it from
            // the cargo.
            ownership_change_to_cargo( ship, val, dst.slot._ );
          }
          case_( Commodity ) {
            move_commodity_as_much_as_possible(
                ship, src.slot._, ship, dst.slot._,
                /*try_other_dst_slots=*/false );
          }
          switch_exhaustive;
        }
      }
      case_( DragArc::outbound_to_inbound ) {
        unit_sail_to_old_world( val.src.id );
      }
      case_( DragArc::outbound_to_inport ) {
        unit_sail_to_old_world( val.src.id );
      }
      case_( DragArc::inbound_to_outbound ) {
        unit_sail_to_new_world( val.src.id );
      }
      case_( DragArc::inport_to_outbound ) {
        unit_sail_to_new_world( val.src.id );
      }
      case_( DragArc::dock_to_inport_ship, src, dst ) {
        ownership_change_to_cargo( dst.id, src.id );
      }
      case_( DragArc::cargo_to_inport_ship, src, dst ) {
        ASSIGN_CHECK_OPT( cargo_object,
                          draggable_to_cargo_object(
                              draggable_from_src( src ) ) );
        switch_( cargo_object ) {
          case_( UnitId ) {
            // Will first "disown" unit which will remove it from
            // the cargo.
            ownership_change_to_cargo( dst.id, val );
          }
          case_( Commodity ) {
            ASSIGN_CHECK_OPT(
                src_ship,
                entities_->active_cargo |
                    fmap_join( L( _.active_unit() ) ) );

            move_commodity_as_much_as_possible(
                src_ship, src.slot._, /*dst_ship=*/dst.id,
                /*dst_slot=*/0,
                /*try_other_dst_slots=*/true );
          }
          switch_exhaustive;
        }
      }
      case_( DragArc::market_to_cargo, src, dst ) {
        ASSIGN_CHECK_OPT(
            ship, entities_->active_cargo |
                      fmap_join( L( _.active_unit() ) ) );
        auto comm = Commodity{
            /*type=*/src.type, //
            /*quantity=*/0     //
        };
        comm.quantity =
            unit_from_id( ship )
                .cargo()
                .max_commodity_quantity_that_fits( src.type );
        CHECK( comm.quantity > 0 );
        // Cap it at 100.
        comm.quantity =
            std::min( k_default_market_quantity, comm.quantity );
        add_commodity_to_cargo( comm, ship,
                                /*slot=*/dst.slot._,
                                /*try_other_slots=*/true );
      }
      case_( DragArc::market_to_inport_ship, src, dst ) {
        auto comm = Commodity{
            /*type=*/src.type, //
            /*quantity=*/0     //
        };
        comm.quantity = std::min(
            k_default_market_quantity,
            unit_from_id( dst.id )
                .cargo()
                .max_commodity_quantity_that_fits( src.type ) );
        CHECK( comm.quantity > 0 );
        add_commodity_to_cargo( comm, dst.id, /*slot=*/0,
                                /*try_other_slots=*/true );
      }
      case_( DragArc::cargo_to_market ) {
        ASSIGN_CHECK_OPT(
            ship, entities_->active_cargo |
                      fmap_join( L( _.active_unit() ) ) );
        rm_commodity_from_cargo( ship, val.src.slot._ );
      }
      switch_exhaustive;
    }
  }

  // This class cannot change the entities, but note that the en-
  // tities will be changed on each frame.
  Entities const* entities_;
};

/****************************************************************
** The Europe Plane
*****************************************************************/
struct EuropePlane : public Plane {
  EuropePlane() = default;
  bool enabled() const override { return true; }
  bool covers_screen() const override { return false; }
  void on_frame_start() override {
    drag_n_drop_.handle_on_frame_start();
    g_dragging_object = drag_n_drop_.obj_being_dragged();
    // Should be last.
    create_entities( &entities_ );
  }
  void draw( Texture& tx ) const override {
    tx.fill( Color::white() );
    draw_entities( tx, entities_ );
    render_rect( tx, rect_color_, clip_rect() );
    // Should be last.
    drag_n_drop_.handle_draw( tx );
  }
  bool input( input::event_t const& event ) override {
    return matcher_( event ) {
      case_( input::unknown_event_t ) result_ false;
      case_( input::quit_event_t ) result_ false;
      case_( input::key_event_t ) result_ false;
      case_( input::mouse_wheel_event_t ) result_ false;
      case_( input::mouse_move_event_t ) {
        if( is_on_clip_rect( val.pos ) )
          this->rect_color_ = Color::blue();
        else
          this->rect_color_ = Color::white();
        result_ true;
      }
      case_( input::mouse_button_event_t ) {
        bool handled         = false;
        auto try_select_unit = [&]( auto const& maybe_entity ) {
          if( maybe_entity ) {
            auto shifted_pos =
                val.pos + ( Coord{} - clip_rect().upper_left() );
            if( auto maybe_id = maybe_entity->unit_under_cursor(
                    shifted_pos );
                maybe_id ) {
              g_selected_unit = maybe_id;
              handled         = true;
            }
          }
        };
        try_select_unit( entities_.ships_in_port );
        try_select_unit( entities_.ships_inbound );
        try_select_unit( entities_.ships_outbound );
        result_ handled;
      }
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

    // Should be last.
    if( button == input::e_mouse_button::l )
      return drag_n_drop_.handle_can_drag( origin );
    return e_accept_drag::no;
  }
  void on_drag( input::e_mouse_button /*button*/, Coord origin,
                Coord prev, Coord current ) override {
    if( drag_n_drop_.is_drag_in_progress() ) {
      drag_n_drop_.handle_on_drag( current );
    } else {
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
  }
  void on_drag_finished( input::e_mouse_button /*button*/,
                         Coord origin, Coord end ) override {
    if( drag_n_drop_.handle_on_drag_finished( origin, end ) )
      return;
  }
  Color               rect_color_{Color::white()};
  Entities            entities_;
  EuroViewDragAndDrop drag_n_drop_{&entities_};
};

EuropePlane g_europe_plane;

/****************************************************************
** Initialization / Cleanup
*****************************************************************/
void init_europort_view() {
  g_clip = main_window_logical_size() - menu_height() +
           Delta{2_w, 2_h};
}

void cleanup_europort_view() {}

REGISTER_INIT_ROUTINE( europort_view );

} // namespace

/****************************************************************
** Public API
*****************************************************************/
Plane* europe_plane() { return &g_europe_plane; }

} // namespace rn
