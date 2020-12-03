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
#include "fmt-helper.hpp"
#include "gfx.hpp"
#include "lua.hpp"
#include "macros.hpp"
#include "text.hpp"
#include "tiles.hpp"
#include "ustate.hpp"
#include "variant.hpp"

// base-util
#include "base-util/pp.hpp"

// magic enum
#include "magic_enum.hpp"

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

OptCRef<Texture> render_commodity_label( string_view label ) {
  if( !label.empty() ) {
    TextMarkupInfo info{ /*normal=*/Color::white(),
                         /*highlight=*/Color::green() };
    return render_text_markup( font::small(), info, label );
  }
  return nothing;
}

void render_commodity_impl( Texture& tx, e_commodity type,
                            Coord            pixel_coord,
                            OptCRef<Texture> label ) {
  auto tile = tile_for_commodity( type );
  render_sprite( tx, tile, pixel_coord );
  if( label ) {
    // Place text below commodity, but centered horizontally.
    auto comm_size  = lookup_sprite( tile ).size();
    auto label_size = label->size();
    auto origin     = pixel_coord + comm_size.h + 2_h -
                  ( label_size.w - comm_size.w ) / 2_sx;
    copy_texture( *label, tx, origin );
  }
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
** Commodity Cargo
*****************************************************************/
expect<> Commodity::check_invariants_safe() const {
  if( quantity <= 0 )
    return UNEXPECTED( "Commodity quantity <= 0 ({})",
                       quantity );
  return xp_success_t{};
}

/****************************************************************
** Public API
*****************************************************************/
Opt<e_commodity> commodity_from_index( int index ) {
  Opt<e_commodity> res;
  if( index >= 0 &&
      index < int( magic_enum::enum_count<e_commodity>() ) )
    res = magic_enum::enum_values<e_commodity>()[index];
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
               comm, slot ),
           "failed to add {} starting at slot {}", comm, slot );
  } else {
    CHECK( unit_from_id( holder ).cargo().try_add( comm, slot ),
           "failed to add {} at slot {}", comm, slot );
  }
}

Commodity rm_commodity_from_cargo( UnitId holder, int slot ) {
  auto& cargo = unit_from_id( holder ).cargo();

  ASSIGN_CHECK_V( cargo_item, cargo[slot], CargoSlot::cargo );
  ASSIGN_CHECK_V( comm, cargo_item.contents, Commodity );

  Commodity res = std::move( comm );
  cargo[slot]   = CargoSlot::empty{};
  cargo.check_invariants();
  return res;
}

int move_commodity_as_much_as_possible(
    UnitId src, int src_slot, UnitId dst, int dst_slot,
    Opt<int> max_quantity, bool try_other_dst_slots ) {
  auto const& src_cargo = unit_from_id( src ).cargo();
  auto        maybe_src_comm =
      src_cargo.slot_holds_cargo_type<Commodity>( src_slot );
  CHECK( maybe_src_comm.has_value() );

  auto const& dst_cargo = unit_from_id( dst ).cargo();
  auto        maybe_dst_comm =
      dst_cargo.slot_holds_cargo_type<Commodity>( dst_slot );
  if( maybe_dst_comm.has_value() && !try_other_dst_slots ) {
    CHECK(
        maybe_dst_comm->type == maybe_src_comm->type,
        "src and dst have different commodity types: {} vs {}",
        maybe_src_comm, maybe_dst_comm );
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
                        maybe_dst_comm->quantity );
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

Opt<string> commodity_label_to_markup(
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

OptCRef<Texture> render_commodity_label(
    CommodityLabel_t const& label ) {
  auto maybe_text = commodity_label_to_markup( label );
  if( maybe_text ) return render_commodity_label( *maybe_text );
  return nothing;
}

void render_commodity( Texture& tx, e_commodity type,
                       Coord pixel_coord ) {
  render_commodity_impl( tx, type, pixel_coord,
                         /*label=*/nothing );
}

void render_commodity_annotated(
    Texture& tx, e_commodity type, Coord pixel_coord,
    CommodityLabel_t const& label ) {
  render_commodity_impl( tx, type, pixel_coord,
                         render_commodity_label( label ) );
}

// Will use quantity as label.
void render_commodity_annotated( Texture&         tx,
                                 Commodity const& comm,
                                 Coord            pixel_coord ) {
  render_commodity_annotated(
      tx, comm.type, pixel_coord,
      CommodityLabel::quantity{ comm.quantity } );
}

Texture render_commodity_create( e_commodity type ) {
  auto tx = create_texture_transparent(
      lookup_sprite( tile_for_commodity( type ) ).size() );
  render_commodity( tx, type, Coord{} );
  return tx;
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_ENUM( commodity )

}

} // namespace rn
