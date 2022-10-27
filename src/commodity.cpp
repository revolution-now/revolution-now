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
#include "macros.hpp"
#include "text.hpp"
#include "tiles.hpp"
#include "ustate.hpp"
#include "variant.hpp"

// config
#include "config/commodity.rds.hpp"

// ss
#include "ss/units.hpp"

// render
#include "render/renderer.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/string.hpp"
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

void render_commodity_label_impl(
    rr::Renderer& renderer, Coord where, string_view label,
    TextMarkupInfo const& markup_info ) {
  if( label.empty() ) return;
  render_text_markup( renderer, where, font::small(),
                      markup_info, label );
}

void render_commodity_label(
    rr::Renderer& renderer, Coord where, string_view label,
    e_commodity_label_render_colors colors ) {
  TextMarkupInfo info;
  switch( colors ) {
    case e_commodity_label_render_colors::standard:
      info = { /*normal=*/gfx::pixel{
                   .r = 0x00, .g = 0x00, .b = 0x00, .a = 255 },
               /*highlight=*/gfx::pixel::green() };
      break;
    case e_commodity_label_render_colors::over_limit:
      info = { /*normal=*/gfx::pixel{
                   .r = 0xaa, .g = 0x00, .b = 0x00, .a = 255 },
               /*highlight=*/gfx::pixel::red() };
      break;
    case e_commodity_label_render_colors::custom_house_selling:
      info = { /*normal=*/gfx::pixel{
                   .r = 0x00, .g = 0xff, .b = 0x00, .a = 255 },
               /*highlight=*/gfx::pixel::yellow() };
      break;
  }
  render_commodity_label_impl( renderer, where, label, info );
}

void render_commodity_impl(
    rr::Renderer& renderer, Coord where, e_commodity type,
    maybe<string>                   label,
    e_commodity_label_render_colors colors ) {
  auto        tile    = tile_for_commodity( type );
  rr::Painter painter = renderer.painter();
  render_sprite( painter, tile, where );
  if( !label ) return;
  // Place text below commodity, but centered horizontally.
  Delta comm_size = sprite_size( tile );
  Delta label_size =
      rendered_text_size( /*reflow_info=*/{}, *label );
  auto origin =
      where + Delta{ .w = -( label_size.w - comm_size.w ) / 2,
                     .h = comm_size.h + 2 };
  render_commodity_label( renderer, origin, *label, colors );
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

string lowercase_commodity_display_name( e_commodity type ) {
  return config_commodity.types[type].lowercase_display_name;
}

string uppercase_commodity_display_name( e_commodity type ) {
  return base::capitalize_initials(
      lowercase_commodity_display_name( type ) );
}

void add_commodity_to_cargo( UnitsState&      units_state,
                             Commodity const& comm,
                             UnitId holder, int slot,
                             bool try_other_slots ) {
  if( try_other_slots ) {
    CHECK(
        units_state.unit_for( holder ).cargo().try_add_somewhere(
            units_state, Cargo::commodity{ comm }, slot ),
        "failed to add {} starting at slot {}", comm, slot );
  } else {
    CHECK( units_state.unit_for( holder ).cargo().try_add(
               units_state, Cargo::commodity{ comm }, slot ),
           "failed to add {} at slot {}", comm, slot );
  }
}

Commodity rm_commodity_from_cargo( UnitsState& units_state,
                                   UnitId holder, int slot ) {
  auto& cargo = units_state.unit_for( holder ).cargo();

  ASSIGN_CHECK_V( cargo_item, cargo[slot], CargoSlot::cargo );
  ASSIGN_CHECK_V( comm, cargo_item.contents, Cargo::commodity );

  Commodity res = std::move( comm.obj );
  cargo[slot]   = CargoSlot_t{ CargoSlot::empty{} };
  cargo.validate_or_die( units_state );
  return res;
}

int move_commodity_as_much_as_possible(
    UnitsState& units_state, UnitId src, int src_slot,
    UnitId dst, int dst_slot, maybe<int> max_quantity,
    bool try_other_dst_slots ) {
  auto const& src_cargo = units_state.unit_for( src ).cargo();
  auto        maybe_src_comm =
      src_cargo.slot_holds_cargo_type<Cargo::commodity>(
          src_slot );
  CHECK( maybe_src_comm.has_value() );

  auto const& dst_cargo = units_state.unit_for( dst ).cargo();
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
  auto removed =
      rm_commodity_from_cargo( units_state, src, src_slot );
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
        units_state, comm_to_transfer, dst,
        /*slot=*/dst_slot,
        /*try_other_slots=*/try_other_dst_slots );
    removed.quantity -= max_transfer_quantity;
    CHECK( removed.quantity >= 0 );
  }

  if( removed.quantity > 0 )
    add_commodity_to_cargo( units_state, removed, src,
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
      auto& [value, colors] =
          label.get<CommodityLabel::quantity>();
      return fmt::format( "{}",
                          commodity_number_to_markup( value ) );
    }
    case CommodityLabel::e::buy_sell: {
      auto& [bid, ask] = label.get<CommodityLabel::buy_sell>();
      return fmt::format( "{}/{}", bid, ask );
    }
  };
}

void render_commodity( rr::Renderer& renderer, Coord where,
                       e_commodity type ) {
  render_commodity_impl( renderer, where, type,
                         /*label=*/nothing, /*colors=*/{} );
}

void render_commodity_annotated(
    rr::Renderer& renderer, Coord where, e_commodity type,
    CommodityLabel_t const& label ) {
  e_commodity_label_render_colors const colors =
      label.get_if<CommodityLabel::quantity>()
          .member( &CommodityLabel::quantity::colors )
          .value_or( e_commodity_label_render_colors::standard );
  render_commodity_impl( renderer, where, type,
                         commodity_label_to_markup( label ),
                         colors );
}

// Will use quantity as label.
void render_commodity_annotated( rr::Renderer&    renderer,
                                 Coord            where,
                                 Commodity const& comm ) {
  render_commodity_annotated(
      renderer, where, comm.type,
      CommodityLabel::quantity{ .value  = comm.quantity,
                                .colors = {} } );
}

} // namespace rn
