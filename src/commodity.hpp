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
#include "fb.hpp"
#include "fmt-helper.hpp"
#include "id.hpp"
#include "tx.hpp"

// Flatbuffers
#include "fb/commodity_generated.h"

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
SERIALIZABLE_ENUM( e_commodity );

// Index refers to the ordering in the enum above, starting at 0.
Opt<e_commodity> commodity_from_index( int index );

// Gets a nice display name; may contain spaces.
char const* commodity_display_name( e_commodity type );

/****************************************************************
** Commodity Labels
*****************************************************************/
adt_rn( CommodityLabel,     //
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
  bool operator==( Commodity const& rhs ) const {
    return type == rhs.type && quantity == rhs.quantity;
  }
  bool operator!=( Commodity const& rhs ) const {
    return !( *this == rhs );
  }

  // clang-format off
  SERIALIZABLE_STRUCT_MEMBERS( Commodity,
    ( e_commodity, type     ),
    ( int,         quantity )
  );
  // clang-format on
};
NOTHROW_MOVE( Commodity );

// These are "low level" functions that should only be called
// after all the right checks have been made that the cargo can
// fit (or exists, as the case may be). If the action cannot be
// carried out then an error will be thrown. This will try to put
// all the cargo in the specified slot, or, if it doesn't fit,
// will try to distribute it to other slots.
void add_commodity_to_cargo( Commodity const& comm,
                             UnitId holder, int slot,
                             bool try_other_slots );

Commodity rm_commodity_from_cargo( UnitId holder, int slot );

// This will take the commodity in (only) the src_slot of the src
// unit and will attempt to move as much of it as possible to the
// dst unit's dst_slot (and/or to other dst slots if "try other
// dst slots" is true) It will return the quantity actually
// moved. Note this function may return zero if nothing can be
// moved (that is ok), but it will return an error if there is no
// commodity at the src's src_slot.
//
// This function will work even if the src and dst units are the
// same (and then even if the src/dst slots are the same).
int move_commodity_as_much_as_possible(
    UnitId src, int src_slot, UnitId dst, int dst_slot,
    Opt<int> max_quantity, bool try_other_dst_slots );

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

// Only call this if you need to create a new small texture.
Texture render_commodity_create( e_commodity type );

} // namespace rn

DEFINE_FORMAT( rn::Commodity, "Commodity{{type={},quantity={}}}",
               o.type, o.quantity );
