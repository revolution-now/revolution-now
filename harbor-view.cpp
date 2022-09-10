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
#include "dragdrop.hpp"
#include "gui.hpp"
#include "harbor-units.hpp"
#include "image.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "macros.hpp"
#include "market.hpp"
#include "old-world-state.rds.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "render.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "tiles.hpp"
#include "ts.hpp"
#include "ustate.hpp"
#include "variant.hpp"
#include "wait.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// gfx
#include "gfx/coord.hpp"

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
** Harbor View Entities
*****************************************************************/
namespace entity {

class DockAnchor : EntityBase {
  static constexpr H above_active_cargo_box{ 32 };

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

  DockAnchor( DockAnchor&& )            = default;
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
          maybe_market_commodities->bounds().right_edge() - 32;
      auto x_upper_bound = 0 + size.w - 60;
      auto x_lower_bound =
          maybe_active_cargo_box->bounds().right_edge();
      if( x_upper_bound < x_lower_bound ) return res;
      location_x =
          std::clamp( location_x, x_lower_bound, x_upper_bound );
      if( location_y < 0 ) return res;
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
  static constexpr Delta image_distance_from_anchor{ .w = 950,
                                                     .h = 544 };

 public:
  void draw( rr::Renderer& renderer, Delta offset ) const {
    rr::Painter painter = renderer.painter();
    render_sprite_section(
        painter, e_tile::harbor_background, Coord{} + offset,
        Rect::from( upper_left_of_render_rect_, size_ ) );
  }

  Backdrop( Backdrop&& )            = default;
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

class Dock : EntityBase {
  static constexpr Delta dock_block_pixels{ .w = 24, .h = 24 };
  static inline Delta    dock_block_pixels_delta =
      Delta{ .w = 1, .h = 1 } * dock_block_pixels;

 public:
  Rect bounds() const {
    return Rect::from(
        origin_,
        Delta{ .w = length_in_blocks_ * dock_block_pixels.w,
               .h = 1 * dock_block_pixels.h } );
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

  Dock( Dock&& )            = default;
  Dock& operator=( Dock&& ) = default;

  static maybe<Dock> create(
      PS& S, Delta const& size,
      maybe<DockAnchor> const& maybe_dock_anchor,
      maybe<InPortBox> const&  maybe_in_port_box ) {
    maybe<Dock> res;
    if( maybe_dock_anchor && maybe_in_port_box ) {
      auto available = maybe_dock_anchor->bounds().left_edge() -
                       maybe_in_port_box->bounds().right_edge();
      available /= dock_block_pixels.w;
      auto origin =
          maybe_dock_anchor->bounds().upper_left() -
          Delta{ .w = ( available * dock_block_pixels.w ) };
      origin -= Delta{ .h = 1 * dock_block_pixels.h / 2 };
      res           = Dock( S, /*origin_=*/origin,
                            /*length_in_blocks_=*/available );
      auto lr_delta = ( res->bounds().lower_right() -
                        Delta{ .w = 1, .h = 1 } ) -
                      Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h )
        res = nothing;
      if( res->bounds().y < 0 ) res = nothing;
      if( res->bounds().x < 0 ) res = nothing;
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

class UnitsOnDock : public UnitCollection {
 public:
  UnitsOnDock( UnitsOnDock&& )            = default;
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
      for( auto id : harbor_units_on_dock( S.ss_.units,
                                           S.player.nation ) ) {
        units.push_back( { id, coord } );
        coord -= Delta{ .w = g_tile_delta.w };
        if( coord.x < maybe_dock->bounds().left_edge() )
          coord =
              Coord{ .x = ( maybe_dock->bounds().upper_right() -
                            g_tile_delta )
                              .x,
                     .y = coord.y - g_tile_delta.h };
      }
      // populate units...
      res = UnitsOnDock(
          S,
          /*bounds_when_no_units_=*/maybe_dock_anchor->bounds(),
          /*units_=*/std::move( units ) );
      auto bds = res->bounds();
      auto lr_delta =
          ( bds.lower_right() - Delta{ .w = 1, .h = 1 } ) -
          Coord{};
      if( lr_delta.w > size.w || lr_delta.h > size.h )
        res = nothing;
      if( bds.y < 0 ) res = nothing;
      if( bds.x < 0 ) res = nothing;
    }
    return res;
  }

 private:
  UnitsOnDock( PS& S, Rect dock_anchor,
               vector<UnitWithPosition>&& units )
    : UnitCollection( S, dock_anchor, std::move( units ) ) {}
};
NOTHROW_MOVE( UnitsOnDock );

} // namespace entity

//- Buttons
//- Message box
//- Stats area (money, tax rate, etc.)

struct Entities {
  maybe<entity::DockAnchor>  dock_anchor;
  maybe<entity::Backdrop>    backdrop;
  maybe<entity::Dock>        dock;
  maybe<entity::UnitsOnDock> units_on_dock;
};
NOTHROW_MOVE( Entities );

void create_entities( PS& S, Entities* entities ) {
  entities->dock_anchor =                             //
      DockAnchor::create( S, clip,                    //
                          entities->active_cargo_box, //
                          entities->market_commodities );
  entities->backdrop =           //
      Backdrop::create( S, clip, //
                        entities->dock_anchor );
  entities->dock =                         //
      Dock::create( S, clip,               //
                    entities->dock_anchor, //
                    entities->in_port_box );
  entities->units_on_dock =                       //
      UnitsOnDock::create( S, clip,               //
                           entities->dock_anchor, //
                           entities->dock );
}

void draw_entities( rr::Renderer&   renderer,
                    Entities const& entities ) {
  UNWRAP_CHECK(
      normal_area,
      compositor::section( compositor::e_section::normal ) );
  auto offset = normal_area.upper_left().distance_from_origin();
  if( entities.backdrop.has_value() )
    entities.backdrop->draw( renderer, offset );
  if( entities.dock_anchor.has_value() )
    entities.dock_anchor->draw( renderer, offset );
  if( entities.dock.has_value() )
    entities.dock->draw( renderer, offset );
  if( entities.units_on_dock.has_value() )
    entities.units_on_dock->draw( renderer, offset );
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
              return is_unit_in_port( S.ss_.units, id );
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
  return res;
}

// Important: in this function we should not return early; we
// should check all the entities (in order) to allow later ones
// to override earlier ones.
maybe<HarborDragDst_t> drag_dst_from_coord(
    Entities const* entities, Coord const& coord ) {
  using namespace entity;
  maybe<HarborDragDst_t> res;
  if( entities->dock.has_value() ) {
    if( coord.is_inside( entities->dock->bounds() ) )
      res = HarborDragDst::dock{};
  }
  if( entities->units_on_dock.has_value() ) {
    if( coord.is_inside( entities->units_on_dock->bounds() ) )
      res = HarborDragDst::dock{};
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
    if( !is_unit_in_port( S.ss_.units, ship ) ) return false;
    return S.ss_.units.unit_for( ship ).cargo().fits_somewhere(
        S.ss_.units, Cargo::unit{ src.id }, dst.slot );
  }
  bool DRAG_CONNECT_CASE( cargo, dock ) const {
    return holds<HarborDraggableObject::unit>(
               draggable_from_src( S, src ) )
        .has_value();
  }
  bool DRAG_CONNECT_CASE( cargo, cargo ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    if( !is_unit_in_port( S.ss_.units, ship ) ) return false;
    if( src.slot == dst.slot ) return true;
    UNWRAP_CHECK( cargo_object,
                  draggable_to_cargo_object(
                      draggable_from_src( S, src ) ) );
    return overload_visit(
        cargo_object,
        [&]( Cargo::unit ) {
          return S.ss_.units.unit_for( ship )
              .cargo()
              .fits_with_item_removed(
                  S.ss_.units,
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
          return S.ss_.units.unit_for( ship ).cargo().fits(
              S.ss_.units,
              /*cargo=*/Cargo::commodity{ size_one },
              /*slot=*/dst.slot );
        } );
  }
  bool DRAG_CONNECT_CASE( outbound, inbound ) const {
    return true;
  }
  bool DRAG_CONNECT_CASE( outbound, inport ) const {
    UNWRAP_CHECK(
        info, S.ss_.units.maybe_harbor_view_state_of( src.id ) );
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
    return S.ss_.units.unit_for( dst.id ).cargo().fits_somewhere(
        S.ss_.units, Cargo::unit{ src.id } );
  }
  bool DRAG_CONNECT_CASE( cargo, inport_ship ) const {
    auto dst_ship = dst.id;
    UNWRAP_CHECK( cargo_object,
                  draggable_to_cargo_object(
                      draggable_from_src( S, src ) ) );
    return overload_visit(
        cargo_object,
        [&]( Cargo::unit u ) {
          if( is_unit_onboard( S.ss_.units, u.id ) == dst_ship )
            return false;
          return S.ss_.units.unit_for( dst_ship )
              .cargo()
              .fits_somewhere( S.ss_.units, u );
        },
        [&]( Cargo::commodity const& c ) {
          // If even 1 quantity can fit then we can proceed
          // with (at least) a partial transfer.
          auto size_one     = c.obj;
          size_one.quantity = 1;
          return S.ss_.units.unit_for( dst_ship )
              .cargo()
              .fits_somewhere( S.ss_.units,
                               Cargo::commodity{ size_one } );
        } );
  }
  bool DRAG_CONNECT_CASE( market, cargo ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    if( !is_unit_in_port( S.ss_.units, ship ) ) return false;
    auto comm = Commodity{
        /*type=*/src.type, //
        // If the commodity can fit even with just one quan-
        // tity then it is allowed, since we will just insert
        // as much as possible if we can't insert 100.
        /*quantity=*/1 //
    };
    return S.ss_.units.unit_for( ship ).cargo().fits_somewhere(
        S.ss_.units, Cargo::commodity{ comm }, dst.slot );
  }
  bool DRAG_CONNECT_CASE( market, inport_ship ) const {
    auto comm = Commodity{
        /*type=*/src.type, //
        // If the commodity can fit even with just one quan-
        // tity then it is allowed, since we will just insert
        // as much as possible if we can't insert 100.
        /*quantity=*/1 //
    };
    return S.ss_.units.unit_for( dst.id ).cargo().fits_somewhere(
        S.ss_.units, Cargo::commodity{ comm },
        /*starting_slot=*/0 );
  }
  bool DRAG_CONNECT_CASE( cargo, market ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    if( !is_unit_in_port( S.ss_.units, ship ) ) return false;
    return S.ss_.units.unit_for( ship )
        .cargo()
        .template slot_holds_cargo_type<Cargo::commodity>(
            src.slot )
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

    // FIXME: add proper initial value.
    maybe<int> const res = co_await S.ts_.gui.optional_int_input(
        { .msg           = text,
          .initial_value = 0,
          .min           = 0,
          .max           = 100 } );
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
    CHECK( is_unit_in_port( S.ss_.units, ship ) );
    UNWRAP_CHECK(
        commodity_ref,
        S.ss_.units.unit_for( ship )
            .cargo()
            .template slot_holds_cargo_type<Cargo::commodity>(
                src.slot ) );
    src.quantity = co_await ask_for_quantity(
        S, commodity_ref.obj.type, "sell" );
    co_return src.quantity.has_value();
  }
  wait<bool> DRAG_CONFIRM_CASE( cargo, inport_ship ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    CHECK( is_unit_in_port( S.ss_.units, ship ) );
    auto maybe_commodity_ref =
        S.ss_.units.unit_for( ship )
            .cargo()
            .template slot_holds_cargo_type<Cargo::commodity>(
                src.slot );
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
    if( S.ss_.units.unit_for( ship ).cargo().fits(
            S.ss_.units, Cargo::unit{ src.id }, dst.slot ) )
      S.ss_.units.change_to_cargo_somewhere( ship, src.id,
                                             dst.slot );
    else
      S.ss_.units.change_to_cargo_somewhere( ship, src.id );
  }
  void DRAG_PERFORM_CASE( cargo, dock ) const {
    ASSIGN_CHECK_V( unit, draggable_from_src( S, src ),
                    HarborDraggableObject::unit );
    unit_move_to_port( S.ss_.units, unit.id );
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
          S.ss_.units.change_to_cargo_somewhere( ship, u.id,
                                                 dst.slot );
        },
        [&]( Cargo::commodity const& ) {
          move_commodity_as_much_as_possible(
              S.ss_.units, ship, src.slot, ship, dst.slot,
              /*max_quantity=*/nothing,
              /*try_other_dst_slots=*/false );
        } );
  }
  void DRAG_PERFORM_CASE( outbound, inbound ) const {
    unit_sail_to_harbor( S.ss_.terrain, S.ss_.units, S.player,
                         src.id );
  }
  void DRAG_PERFORM_CASE( outbound, inport ) const {
    unit_sail_to_harbor( S.ss_.terrain, S.ss_.units, S.player,
                         src.id );
  }
  void DRAG_PERFORM_CASE( inbound, outbound ) const {
    unit_sail_to_new_world( S.ss_.terrain, S.ss_.units, S.player,
                            src.id );
  }
  void DRAG_PERFORM_CASE( inport, outbound ) const {
    HarborState& hb_state = S.harbor_state();
    unit_sail_to_new_world( S.ss_.terrain, S.ss_.units, S.player,
                            src.id );
    // This is not strictly necessary, but for a nice user expe-
    // rience we will auto-select another unit that is in-port
    // (if any) since that is likely what the user wants to work
    // with, as opposed to keeping the selection on the unit that
    // is now outbound. Or if there are no more units in port,
    // just deselect.
    hb_state.selected_unit = nothing;
    vector<UnitId> units_in_port =
        harbor_units_in_port( S.ss_.units, S.player.nation );
    hb_state.selected_unit = rl::all( units_in_port ).head();
  }
  void DRAG_PERFORM_CASE( dock, inport_ship ) const {
    S.ss_.units.change_to_cargo_somewhere( dst.id, src.id );
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
          S.ss_.units.change_to_cargo_somewhere( dst.id, u.id );
        },
        [&]( Cargo::commodity const& ) {
          UNWRAP_CHECK( src_ship,
                        active_cargo_ship( entities ) );
          move_commodity_as_much_as_possible(
              S.ss_.units, src_ship, src.slot,
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
        S.ss_.units.unit_for( ship )
            .cargo()
            .max_commodity_quantity_that_fits( src.type ) );
    // Cap it.
    comm.quantity =
        std::min( comm.quantity, k_default_market_quantity );
    CHECK( comm.quantity > 0 );
    add_commodity_to_cargo( S.ss_.units, comm, ship,
                            /*slot=*/dst.slot,
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
        S.ss_.units.unit_for( dst.id )
            .cargo()
            .max_commodity_quantity_that_fits( src.type ) );
    // Cap it.
    comm.quantity =
        std::min( comm.quantity, k_default_market_quantity );
    CHECK( comm.quantity > 0 );
    add_commodity_to_cargo( S.ss_.units, comm, dst.id,
                            /*slot=*/0,
                            /*try_other_slots=*/true );
  }
  void DRAG_PERFORM_CASE( cargo, market ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    UNWRAP_CHECK(
        commodity_ref,
        S.ss_.units.unit_for( ship )
            .cargo()
            .template slot_holds_cargo_type<Cargo::commodity>(
                src.slot ) );
    auto quantity_wants_to_sell =
        src.quantity.value_or( commodity_ref.obj.quantity );
    int       amount_to_sell = std::min( quantity_wants_to_sell,
                                         commodity_ref.obj.quantity );
    Commodity new_comm       = commodity_ref.obj;
    new_comm.quantity -= amount_to_sell;
    rm_commodity_from_cargo( S.ss_.units, ship, src.slot );
    if( new_comm.quantity > 0 )
      add_commodity_to_cargo( S.ss_.units, new_comm, ship,
                              /*slot=*/src.slot,
                              /*try_other_slots=*/false );
  }
  void operator()( auto const&, auto const& ) const {
    SHOULD_NOT_BE_HERE;
  }
};

} // namespace
} // namespace rn
