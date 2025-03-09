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
#include "text.hpp"
#include "tiles.hpp"
#include "unit-mgr.hpp"

// config
#include "config/commodity.rds.hpp"
#include "config/tile-enum.rds.hpp"
#include "config/ui.rds.hpp"

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

using ::gfx::pixel;

e_tile tile_for_commodity_16( e_commodity const c ) {
  switch( c ) {
    case e_commodity::food:
      return e_tile::commodity_food_16;
    case e_commodity::sugar:
      return e_tile::commodity_sugar_16;
    case e_commodity::tobacco:
      return e_tile::commodity_tobacco_16;
    case e_commodity::cotton:
      return e_tile::commodity_cotton_16;
    case e_commodity::furs:
      return e_tile::commodity_furs_16;
    case e_commodity::lumber:
      return e_tile::commodity_lumber_16;
    case e_commodity::ore:
      return e_tile::commodity_ore_16;
    case e_commodity::silver:
      return e_tile::commodity_silver_16;
    case e_commodity::horses:
      return e_tile::commodity_horses_16;
    case e_commodity::rum:
      return e_tile::commodity_rum_16;
    case e_commodity::cigars:
      return e_tile::commodity_cigars_16;
    case e_commodity::cloth:
      return e_tile::commodity_cloth_16;
    case e_commodity::coats:
      return e_tile::commodity_coats_16;
    case e_commodity::trade_goods:
      return e_tile::commodity_trade_goods_16;
    case e_commodity::tools:
      return e_tile::commodity_tools_16;
    case e_commodity::muskets:
      return e_tile::commodity_muskets_16;
  }
}

e_tile tile_for_commodity_20( e_commodity const c ) {
  switch( c ) {
    case e_commodity::food:
      return e_tile::commodity_food_20;
    case e_commodity::sugar:
      return e_tile::commodity_sugar_20;
    case e_commodity::tobacco:
      return e_tile::commodity_tobacco_20;
    case e_commodity::cotton:
      return e_tile::commodity_cotton_20;
    case e_commodity::furs:
      return e_tile::commodity_furs_20;
    case e_commodity::lumber:
      return e_tile::commodity_lumber_20;
    case e_commodity::ore:
      return e_tile::commodity_ore_20;
    case e_commodity::silver:
      return e_tile::commodity_silver_20;
    case e_commodity::horses:
      return e_tile::commodity_horses_20;
    case e_commodity::rum:
      return e_tile::commodity_rum_20;
    case e_commodity::cigars:
      return e_tile::commodity_cigars_20;
    case e_commodity::cloth:
      return e_tile::commodity_cloth_20;
    case e_commodity::coats:
      return e_tile::commodity_coats_20;
    case e_commodity::trade_goods:
      return e_tile::commodity_trade_goods_20;
    case e_commodity::tools:
      return e_tile::commodity_tools_20;
    case e_commodity::muskets:
      return e_tile::commodity_muskets_20;
  }
}

void render_commodity_label_impl(
    rr::Renderer& renderer, Coord where, string_view label,
    rr::TextLayout const& text_layout,
    TextMarkupInfo const& markup_info ) {
  if( label.empty() ) return;
  render_text_markup( renderer, where, font::small(),
                      text_layout, markup_info, label );
}

void render_commodity_label(
    rr::Renderer& renderer, Coord where, string_view label,
    rr::TextLayout const& text_layout,
    e_commodity_label_render_colors colors ) {
  TextMarkupInfo info;
  switch( colors ) {
    case e_commodity_label_render_colors::standard:
      info = {
        .normal =
            pixel{ .r = 0x00, .g = 0x00, .b = 0x00, .a = 255 },
        .highlight = pixel::green() };
      break;
    case e_commodity_label_render_colors::over_limit:
      info = {
        .normal =
            pixel{ .r = 0xaa, .g = 0x00, .b = 0x00, .a = 255 },
        .highlight = pixel::red() };
      break;
    case e_commodity_label_render_colors::custom_house_selling:
      info = {
        .normal =
            pixel{ .r = 0x00, .g = 0xff, .b = 0x00, .a = 255 },
        .highlight = pixel::yellow() };
      break;
    case e_commodity_label_render_colors::harbor_cargo: {
      static pixel const color =
          config_ui.harbor.cargo_label_color;
      info = { .normal = color, .highlight = color };
      break;
    }
    case e_commodity_label_render_colors::harbor_cargo_100: {
      static pixel const color =
          config_ui.harbor.cargo_label_color.highlighted(
              config_ui.harbor
                  .cargo_label_color_full_highlight_intensity );
      info = { .normal = color, .highlight = color };
      break;
    }
  }
  render_commodity_label_impl( renderer, where, label,
                               text_layout, info );
}

void render_commodity_impl(
    rr::Renderer& renderer, Coord where, e_tile tile,
    maybe<string> label, rr::TextLayout const& text_layout,
    e_commodity_label_render_colors colors, bool dulled ) {
  render_sprite_dulled( renderer, tile, where, dulled );
  if( !label ) return;
  // Place text below commodity, but centered horizontally.
  Delta comm_size = sprite_size( tile );
  static rr::TextLayout const kTextLayout;
  Delta label_size = rendered_text_size(
      renderer.typer().textometer(), kTextLayout,
      /*reflow_info=*/{}, *label );
  auto origin =
      where + Delta{ .w = -( label_size.w - comm_size.w ) / 2,
                     .h = comm_size.h + 2 };
  render_commodity_label( renderer, origin, *label, text_layout,
                          colors );
}

void render_commodity_16_impl(
    rr::Renderer& renderer, Coord where, e_commodity type,
    maybe<string> label, rr::TextLayout const& text_layout,
    e_commodity_label_render_colors colors, bool dulled ) {
  render_commodity_impl( renderer, where,
                         tile_for_commodity_16( type ), label,
                         text_layout, colors, dulled );
}

void render_commodity_20_impl(
    rr::Renderer& renderer, Coord where, e_commodity type,
    maybe<string> label, rr::TextLayout const& text_layout,
    e_commodity_label_render_colors colors, bool dulled ) {
  render_commodity_impl( renderer, where,
                         tile_for_commodity_20( type ), label,
                         text_layout, colors, dulled );
}

string commodity_number_to_markup( int value ) {
  if( value < 100 ) //
    return fmt::format( "{}", value );
  if( value < 200 )
    return fmt::format( "[{}]{:0>2}", value / 100, value % 100 );
  return fmt::format( "[{}]{:0>2}", value / 100, value % 100 );
}

} // namespace

/****************************************************************
** Commodity
*****************************************************************/
Commodity with_quantity( Commodity const& in,
                         int new_quantity ) {
  Commodity res = in;
  res.quantity  = new_quantity;
  return res;
}

/****************************************************************
** Public API
*****************************************************************/
Delta commodity_tile_size_16( e_commodity ) {
  return { .w = 16, .h = 16 };
}

Delta commodity_tile_size_20( e_commodity ) {
  return { .w = 20, .h = 20 };
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

void add_commodity_to_cargo( UnitsState const& units,
                             Commodity const& comm,
                             CargoHold& cargo, int const slot,
                             bool const try_other_slots ) {
  if( try_other_slots ) {
    CHECK( cargo.try_add_somewhere(
               units, Cargo::commodity{ comm }, slot ),
           "failed to add {} starting at slot {}", comm, slot );
  } else {
    CHECK(
        cargo.try_add( units, Cargo::commodity{ comm }, slot ),
        "failed to add {} at slot {}", comm, slot );
  }
}

Commodity rm_commodity_from_cargo( UnitsState const& units,
                                   CargoHold& cargo,
                                   int const slot ) {
  ASSIGN_CHECK_V( cargo_item, cargo[slot], CargoSlot::cargo );
  ASSIGN_CHECK_V( comm, cargo_item.contents, Cargo::commodity );

  Commodity res = std::move( comm.obj );
  cargo[slot]   = CargoSlot{ CargoSlot::empty{} };
  cargo.validate_or_die( units );
  return res;
}

int move_commodity_as_much_as_possible(
    UnitsState const& units, CargoHold& src_cargo, int src_slot,
    CargoHold& dst_cargo, int dst_slot, maybe<int> max_quantity,
    bool try_other_dst_slots ) {
  auto maybe_src_comm =
      src_cargo.slot_holds_cargo_type<Cargo::commodity>(
          src_slot );
  CHECK( maybe_src_comm.has_value() );

  auto maybe_dst_comm =
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
      rm_commodity_from_cargo( units, src_cargo, src_slot );
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
      CHECK( as_const( dst_cargo )[dst_slot]
                 .holds<CargoSlot::empty>() );
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
        units, comm_to_transfer, dst_cargo,
        /*slot=*/dst_slot,
        /*try_other_slots=*/try_other_dst_slots );
    removed.quantity -= max_transfer_quantity;
    CHECK( removed.quantity >= 0 );
  }

  if( removed.quantity > 0 )
    add_commodity_to_cargo( units, removed, src_cargo,
                            /*slot=*/src_slot,
                            /*try_other_slots=*/false );

  return max_transfer_quantity;
}

maybe<string> commodity_label_to_markup(
    CommodityLabel const& label ) {
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

void render_commodity_16( rr::Renderer& renderer, Coord where,
                          e_commodity type ) {
  render_commodity_16_impl( renderer, where, type,
                            /*label=*/nothing, rr::TextLayout{},
                            /*colors=*/{},
                            /*dulled=*/false );
}

void render_commodity_20( rr::Renderer& renderer, Coord where,
                          e_commodity type ) {
  render_commodity_20_impl( renderer, where, type,
                            /*label=*/nothing, rr::TextLayout{},
                            /*colors=*/{},
                            /*dulled=*/false );
}

void render_commodity_20_outline( rr::Renderer& renderer,
                                  gfx::point const where,
                                  e_commodity const type,
                                  pixel const outline_color ) {
  render_sprite_outline( renderer, where,
                         tile_for_commodity_20( type ),
                         outline_color );
}

void render_commodity_annotated_16(
    rr::Renderer& renderer, Coord where, e_commodity type,
    CommodityRenderStyle const& style ) {
  e_commodity_label_render_colors const colors =
      style.label.get_if<CommodityLabel::quantity>()
          .member( &CommodityLabel::quantity::colors )
          .value_or( e_commodity_label_render_colors::standard );
  render_commodity_16_impl(
      renderer, where, type,
      commodity_label_to_markup( style.label ), rr::TextLayout{},
      colors, style.dulled );
}

void render_commodity_annotated_20(
    rr::Renderer& renderer, Coord where, e_commodity type,
    CommodityRenderStyle const& style ) {
  e_commodity_label_render_colors const colors =
      style.label.get_if<CommodityLabel::quantity>()
          .member( &CommodityLabel::quantity::colors )
          .value_or( e_commodity_label_render_colors::standard );
  render_commodity_20_impl(
      renderer, where, type,
      commodity_label_to_markup( style.label ), rr::TextLayout{},
      colors, style.dulled );
}

// Will use quantity as label.
void render_commodity_annotated_16( rr::Renderer& renderer,
                                    Coord where,
                                    Commodity const& comm ) {
  bool const dulled = ( comm.quantity < 100 );
  render_commodity_annotated_16(
      renderer, where, comm.type,
      CommodityRenderStyle{
        .label =
            CommodityLabel::quantity{ .value  = comm.quantity,
                                      .colors = {} },
        .dulled = dulled } );
}

void render_commodity_annotated_20( rr::Renderer& renderer,
                                    Coord where,
                                    Commodity const& comm ) {
  bool const dulled = ( comm.quantity < 100 );
  render_commodity_annotated_20(
      renderer, where, comm.type,
      CommodityRenderStyle{
        .label =
            CommodityLabel::quantity{ .value  = comm.quantity,
                                      .colors = {} },
        .dulled = dulled } );
}

} // namespace rn
