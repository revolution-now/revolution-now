/****************************************************************
**commodity.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-07-24.
*
* Description: Handles the 16 commodities in the game.
*
*****************************************************************/
#include "commodity.hpp"

// Revolution Now
#include "enum.hpp"
#include "lua.hpp"
#include "macros.hpp"
#include "text.hpp"
#include "tiles.hpp"
#include "ustate.hpp"
#include "variant.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/variant.hpp"

// luapp
#include "luapp/rtable.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"

// base-util
#include "base-util/pp.hpp"

using namespace std;

namespace rn {

namespace {

#define TO_TILE( type )   \
  case e_commodity::type: \
    return PP_JOIN( e_tile::commodity_, type );

#define TO_TILES( ... ) EVAL( PP_MAP( TO_TILE, __VA_ARGS__ ) )

e_tile tile_for_commodity( e_commodity c ) {
  switch( c ) {
    TO_TILES( food,        //
              sugar,       //
              tobacco,     //
              cotton,      //
              fur,         //
              lumber,      //
              ore,         //
              silver,      //
              horses,      //
              rum,         //
              cigars,      //
              cloth,       //
              coats,       //
              trade_goods, //
              tools,       //
              muskets      //
    )
  }
}

void render_commodity_label( rr::Renderer& renderer, Coord where,
                             string_view label ) {
  if( label.empty() ) return;
  TextMarkupInfo info{ /*normal=*/gfx::pixel::white(),
                       /*highlight=*/gfx::pixel::green() };
  render_text_markup( renderer, where, font::small(), info,
                      label );
}

void render_commodity_impl( rr::Renderer& renderer, Coord where,
                            e_commodity   type,
                            maybe<string> label ) {
  auto        tile    = tile_for_commodity( type );
  rr::Painter painter = renderer.painter();
  render_sprite( painter, tile, where );
  if( !label ) return;
  // Place text below commodity, but centered horizontally.
  Delta comm_size = sprite_size( tile );

  // vvv FIXME FIXME FIXME FIXME
  // Hack since we can't compute the rendered text size of `la-
  // bel` because it may contain markup chars. Need to find a
  // better way to do this.
  auto const char_size =
      rr::rendered_text_line_size_pixels( "X" );
  // If the length is longer than two then it contains markup.
  gfx::size label_size = char_size;

  label_size.w = ( ( label->find( '@' ) != string::npos )
                       ? 3
                       : label->size() ) *
                 char_size.w;
  // ^^^ FIXME FIXME FIXME FIXME

  auto origin = where + comm_size.h + 2_h -
                ( label_size.w - comm_size.w ) / 2_sx;
  render_commodity_label( renderer, origin, *label );
}

string commodity_number_to_markup( int value ) {
  if( value < 100 ) //
    return fmt::format( "{}", value );
  if( value < 200 )
    return fmt::format( "@[H]{}@[]{:0>2}", value / 100,
                        value % 100 );
  return fmt::format( "@[H]{}@[]{:0>2}", value / 100,
                      value % 100 );
}

} // namespace

/****************************************************************
** Commodity
*****************************************************************/
Commodity with_quantity( Commodity const& in,
                         int              new_quantity ) {
  Commodity res = in;
  res.quantity  = new_quantity;
  return res;
}

base::valid_or<string> Commodity::validate() const {
  REFL_VALIDATE( quantity >= 0 );
  return base::valid;
}

/****************************************************************
** Public API
*****************************************************************/
Delta commodity_tile_size( e_commodity type ) {
  return sprite_size( tile_for_commodity( type ) );
}

maybe<e_commodity> commodity_from_index( int index ) {
  maybe<e_commodity> res;
  if( index >= 0 &&
      index < int( refl::enum_count<e_commodity> ) )
    res = refl::enum_values<e_commodity>[index];
  return res;
}

string_view commodity_display_name( e_commodity type ) {
  return enum_to_display_name( type );
}

void add_commodity_to_cargo( Commodity const& comm,
                             UnitId holder, int slot,
                             bool try_other_slots ) {
  if( try_other_slots ) {
    CHECK( unit_from_id( holder ).cargo().try_add_somewhere(
               Cargo::commodity{ comm }, slot ),
           "failed to add {} starting at slot {}", comm, slot );
  } else {
    CHECK( unit_from_id( holder ).cargo().try_add(
               Cargo::commodity{ comm }, slot ),
           "failed to add {} at slot {}", comm, slot );
  }
}

Commodity rm_commodity_from_cargo( UnitId holder, int slot ) {
  auto& cargo = unit_from_id( holder ).cargo();

  ASSIGN_CHECK_V( cargo_item, cargo[slot], CargoSlot::cargo );
  ASSIGN_CHECK_V( comm, cargo_item.contents, Cargo::commodity );

  Commodity res = std::move( comm.obj );
  cargo[slot]   = CargoSlot_t{ CargoSlot::empty{} };
  cargo.validate_or_die();
  return res;
}

int move_commodity_as_much_as_possible(
    UnitId src, int src_slot, UnitId dst, int dst_slot,
    maybe<int> max_quantity, bool try_other_dst_slots ) {
  auto const& src_cargo = unit_from_id( src ).cargo();
  auto        maybe_src_comm =
      src_cargo.slot_holds_cargo_type<Cargo::commodity>(
          src_slot );
  CHECK( maybe_src_comm.has_value() );

  auto const& dst_cargo = unit_from_id( dst ).cargo();
  auto        maybe_dst_comm =
      dst_cargo.slot_holds_cargo_type<Cargo::commodity>(
          dst_slot );
  if( maybe_dst_comm.has_value() && !try_other_dst_slots ) {
    CHECK(
        maybe_dst_comm->obj.type == maybe_src_comm->obj.type,
        "src and dst have different commodity types: {} vs {}",
        maybe_src_comm->obj, maybe_dst_comm->obj );
  }

  // Need to remove first in case src/dst are the same unit.
  auto removed = rm_commodity_from_cargo( src, src_slot );
  CHECK( removed.quantity > 0 );

  int max_transfer_quantity = 0;

  if( try_other_dst_slots ) {
    max_transfer_quantity =
        std::min( removed.quantity,
                  dst_cargo.max_commodity_quantity_that_fits(
                      removed.type ) );
  } else {
    if( maybe_dst_comm.has_value() ) {
      max_transfer_quantity =
          std::min( removed.quantity,
                    dst_cargo.max_commodity_per_cargo_slot() -
                        maybe_dst_comm->obj.quantity );
    } else {
      CHECK( holds<CargoSlot::empty>( dst_cargo[dst_slot] ) );
      max_transfer_quantity =
          std::min( removed.quantity,
                    dst_cargo.max_commodity_per_cargo_slot() );
    }
  }

  if( max_quantity.has_value() )
    max_transfer_quantity =
        std::min( max_transfer_quantity, *max_quantity );

  CHECK( max_transfer_quantity >= 0 &&
         max_transfer_quantity <=
             dst_cargo.max_commodity_per_cargo_slot() );

  if( max_transfer_quantity > 0 ) {
    auto comm_to_transfer     = removed;
    comm_to_transfer.quantity = max_transfer_quantity;
    add_commodity_to_cargo(
        comm_to_transfer, dst,
        /*slot=*/dst_slot,
        /*try_other_slots=*/try_other_dst_slots );
    removed.quantity -= max_transfer_quantity;
    CHECK( removed.quantity >= 0 );
  }

  if( removed.quantity > 0 )
    add_commodity_to_cargo( removed, src,
                            /*slot=*/src_slot,
                            /*try_other_slots=*/false );

  return max_transfer_quantity;
}

maybe<string> commodity_label_to_markup(
    CommodityLabel_t const& label ) {
  switch( label.to_enum() ) {
    case CommodityLabel::e::none: {
      return nothing;
    }
    case CommodityLabel::e::quantity: {
      auto& [value] = label.get<CommodityLabel::quantity>();
      return fmt::format( "{}",
                          commodity_number_to_markup( value ) );
    }
    case CommodityLabel::e::buy_sell: {
      auto& [sell, buy] = label.get<CommodityLabel::buy_sell>();
      return fmt::format( "{}/{}", sell / 100, buy / 100 );
    }
  };
}

void render_commodity( rr::Renderer& renderer, Coord where,
                       e_commodity type ) {
  render_commodity_impl( renderer, where, type,
                         /*label=*/nothing );
}

void render_commodity_annotated(
    rr::Renderer& renderer, Coord where, e_commodity type,
    CommodityLabel_t const& label ) {
  render_commodity_impl( renderer, where, type,
                         commodity_label_to_markup( label ) );
}

// Will use quantity as label.
void render_commodity_annotated( rr::Renderer&    renderer,
                                 Coord            where,
                                 Commodity const& comm ) {
  render_commodity_annotated(
      renderer, where, comm.type,
      CommodityLabel::quantity{ comm.quantity } );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
LUA_ENUM( commodity )

} // namespace rn
