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
#include "aliases.hpp"
#include "coord.hpp"
#include "enum.hpp"
#include "sdl-util.hpp" // FIXME: get rid of this.

// C++ standard library
#include <string_view>

namespace rn {

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

struct Commodity {
  e_commodity type;
  int         quantity;

  bool operator==( Commodity const& rhs ) const {
    return type == rhs.type && quantity == rhs.quantity;
  }
};

// The optional label may contain markup characters.
void render_commodity( Texture const& tx, e_commodity type,
                       Coord            pixel_coord,
                       std::string_view label = "" );

} // namespace rn
