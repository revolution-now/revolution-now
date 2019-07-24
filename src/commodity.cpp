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

// base-util
#include "base-util/pp.hpp"

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

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void render_commodity( Texture const& tx, e_commodity type,
                       Coord pixel_coord, string_view label ) {
  auto tile = tile_for_commodity( type );
  render_sprite( tx, tile, pixel_coord );
  if( !label.empty() ) {
    TextMarkupInfo info{/*normal=*/Color::white(),
                        /*highlight=*/Color::green()};
    auto           text =
        render_text_markup( fonts::small(), info, label );
    auto comm_rect =
        Rect::from( pixel_coord, lookup_sprite( tile ).size() );
    auto origin = centered( text.size(), comm_rect );
    // Place text at the bottom, but centered horizontally.
    origin.y = comm_rect.bottom_edge() - text.size().h;
    copy_texture( text, tx, origin );
  }
}

} // namespace rn
