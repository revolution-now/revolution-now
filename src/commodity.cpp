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
#include "fmt-helper.hpp"
#include "macros.hpp"
#include "text.hpp"
#include "tiles.hpp"

// base-util
#include "base-util/pp.hpp"
#include "base-util/variant.hpp"

using namespace std;

namespace rn {

namespace {

#define TO_TILE( type )    \
  case +e_commodity::type: \
    return PP_JOIN( g_tile::commodity_, type );

#define TO_TILES( ... ) EVAL( PP_MAP( TO_TILE, __VA_ARGS__ ) )

g_tile tile_for_commodity( e_commodity c ) {
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
  UNREACHABLE_LOCATION;
}

Opt<CRef<Texture>> render_commodity_label( string_view label ) {
  Opt<CRef<Texture>> res;
  if( !label.empty() ) {
    TextMarkupInfo info{/*normal=*/Color::white(),
                        /*highlight=*/Color::green()};
    res = render_text_markup( font::small(), info, label );
  }
  return res;
}

void render_commodity_impl( Texture& tx, e_commodity type,
                            Coord              pixel_coord,
                            Opt<CRef<Texture>> label ) {
  auto tile = tile_for_commodity( type );
  render_sprite( tx, tile, pixel_coord );
  if( label ) {
    // Place text below commodity, but centered horizontally.
    auto comm_size  = lookup_sprite( tile ).size();
    auto label_size = label->get().size();
    auto origin     = pixel_coord + comm_size.h + 2_h -
                  ( label_size.w - comm_size.w ) / 2_sx;
    copy_texture( label->get(), tx, origin );
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
** Public API
*****************************************************************/
Opt<string> commodity_label_to_markup(
    CommodityLabel_t const& label ) {
  return matcher_( label, ->, Opt<string> ) {
    case_( CommodityLabel::none ) { //
      return nullopt;
    }
    case_( CommodityLabel::quantity, value ) { //
      return fmt::format( "{}",
                          commodity_number_to_markup( value ) );
    }
    case_( CommodityLabel::buy_sell, sell, buy ) { //
      return fmt::format( "{}/{}", sell / 100, buy / 100 );
    }
    matcher_exhaustive;
  };
}

Opt<CRef<Texture>> render_commodity_label(
    CommodityLabel_t const& label ) {
  Opt<CRef<Texture>> res;
  auto maybe_text = commodity_label_to_markup( label );
  if( maybe_text ) res = render_commodity_label( *maybe_text );
  return res;
}

void render_commodity( Texture& tx, e_commodity type,
                       Coord pixel_coord ) {
  render_commodity_impl( tx, type, pixel_coord,
                         /*label=*/nullopt );
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
      CommodityLabel::quantity{comm.quantity} );
}

} // namespace rn
