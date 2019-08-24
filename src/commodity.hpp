/****************************************************************
**commodity.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-07-24.
*
* Description: Handles the 16 commodities in the game.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "adt.hpp"
#include "aliases.hpp"
#include "coord.hpp"
#include "enum.hpp"
#include "fmt-helper.hpp"
#include "tx.hpp"

// C++ standard library
#include <string>

namespace rn {

/****************************************************************
** Commodity List
*****************************************************************/
// Important: the ordering here matters, as it determines the
// order in which the commodities are displayed in an array and
// the order in which they are processed.
// clang-format off
enum class e_(commodity,
  food,
  sugar,
  tobacco,
  cotton,
  fur,
  lumber,
  ore,
  silver,
  horses,
  rum,
  cigars,
  cloth,
  coats,
  trade_goods,
  tools,
  muskets
);
// clang-format on

/****************************************************************
** Commodity Labels
*****************************************************************/
ADT_RN( CommodityLabel,     //
        ( none ),           //
        ( quantity,         //
          ( int, value ) ), //
        ( buy_sell,         //
          ( int, sell ),    //
          ( int, buy ) )    //
);

// Returns markup text representing the label.
Opt<std::string> commodity_label_to_markup(
    CommodityLabel_t const& label );

// Will be rendered as a one-line text string with transparent
// background. Could return nullopt if label is `none`.
Opt<CRef<Texture>> render_commodity_label(
    CommodityLabel_t const& label );

/****************************************************************
** Commodity Cargo
*****************************************************************/
// This is the object that gets held as cargo either in a unit's
// cargo or in a colony.
struct Commodity {
  e_commodity type;
  int         quantity;

  bool operator==( Commodity const& rhs ) const {
    return type == rhs.type && quantity == rhs.quantity;
  }
  bool operator!=( Commodity const& rhs ) const {
    return !( *this == rhs );
  }
};

/****************************************************************
** Commodity Renderers
*****************************************************************/
void render_commodity( Texture& tx, e_commodity type,
                       Coord pixel_coord );

// The "annotated" functions will render the label just below the
// commodity sprite and both will have their centers aligned hor-
// izontally. Note that the pixel coordinate is the upper left
// corner of the commodity sprite.

void render_commodity_annotated( Texture& tx, e_commodity type,
                                 Coord pixel_coord,
                                 CommodityLabel_t const& label );

// Will use quantity as label.
void render_commodity_annotated( Texture&         tx,
                                 Commodity const& comm,
                                 Coord            pixel_coord );

} // namespace rn

DEFINE_FORMAT( rn::Commodity, "Commodity{{type={},quantity={}}}",
               o.type, o.quantity );
