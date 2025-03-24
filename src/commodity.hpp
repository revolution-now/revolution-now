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

// Rds
#include "commodity.rds.hpp"

// Revolution Now
#include "maybe.hpp"
#include "unit-id.hpp"

// ss
#include "ss/commodity.rds.hpp"

// gfx
#include "gfx/cartesian.hpp"
#include "gfx/coord.hpp"
#include "gfx/pixel.hpp"

// refl
#include "refl/query-enum.hpp"

// C++ standard library
#include <string>

namespace rr {
struct Renderer;
}

namespace rn {

struct UnitsState;
struct CargoHold;

enum class e_tile;

constexpr Delta const kCommodityInCargoHoldRenderingOffset{
  .w = 8, .h = 3 };

constexpr Delta kCommodityTileSize = Delta{ .w = 16, .h = 16 };
constexpr Delta kCommodityTileLargeSize =
    Delta{ .w = 20, .h = 20 };

// Keep this here because it is needed in multiple places.
gfx::size const kLabeledCommodity20CargoRenderOffset{ .w = 6,
                                                      .h = 2 };

/****************************************************************
** Commodity List
*****************************************************************/
constexpr int kNumCommodityTypes = refl::enum_count<e_commodity>;

// Index refers to the ordering in the enum above, starting at 0.
maybe<e_commodity> commodity_from_index( int index );

// Gets a nice display name; may contain spaces.
std::string lowercase_commodity_display_name( e_commodity type );
std::string uppercase_commodity_display_name( e_commodity type );

/****************************************************************
** Commodity Labels
*****************************************************************/
// Returns markup text representing the label.
maybe<std::string> commodity_label_to_markup(
    CommodityLabel const& label );

/****************************************************************
** Commodity
*****************************************************************/
Commodity with_quantity( Commodity const& in, int new_quantity );

// These are "low level" functions that should only be called
// after all the right checks have been made that the cargo can
// fit (or exists, as the case may be). If the action cannot be
// carried out then an error will be thrown. This will try to put
// all the cargo in the specified slot, or, if it doesn't fit,
// will try to distribute it to other slots.
void add_commodity_to_cargo( UnitsState const& units,
                             Commodity const& comm,
                             CargoHold& cargo, int slot,
                             bool try_other_slots );

Commodity rm_commodity_from_cargo( UnitsState const& units,
                                   CargoHold& cargo, int slot );

// This will take the commodity in (only) the src_slot of the src
// unit and will attempt to move as much of it as possible to the
// dst unit's dst_slot (and/or to other dst slots if "try other
// dst slots" is true) It will return the quantity actually
// moved. Note this function may return zero if nothing can be
// moved (that is ok), but it will check fail if there is no com-
// modity at the src's src_slot.
//
// This function will work even if the src and dst units are the
// same (and then even if the src/dst slots are the same).
int move_commodity_as_much_as_possible(
    UnitsState const& units, CargoHold& src_cargo, int src_slot,
    CargoHold& dst_cargo, int dst_slot, maybe<int> max_quantity,
    bool try_other_dst_slots );

/****************************************************************
** Commodity Renderers
*****************************************************************/
e_tile tile_for_commodity_16( e_commodity const c );
e_tile tile_for_commodity_20( e_commodity const c );

Delta commodity_tile_size_16( e_commodity type );
Delta commodity_tile_size_20( e_commodity type );

void render_commodity_16( rr::Renderer& renderer, Coord where,
                          e_commodity type );

void render_commodity_20( rr::Renderer& renderer, Coord where,
                          e_commodity type );

void render_commodity_20_outline( rr::Renderer& renderer,
                                  gfx::point where,
                                  e_commodity type,
                                  gfx::pixel outline_color );

// The "annotated" functions will render the label just below the
// commodity sprite and both will have their centers aligned hor-
// izontally. Note that the coordinate is the upper left corner
// of the commodity sprite.

void render_commodity_annotated_16(
    rr::Renderer& renderer, Coord where, e_commodity type,
    CommodityRenderStyle const& style );
void render_commodity_annotated_20(
    rr::Renderer& renderer, Coord where, e_commodity type,
    CommodityRenderStyle const& style );

// Will use quantity as label and will render the icon greyed if
// the quantity is less than 100.
void render_commodity_annotated_16( rr::Renderer& renderer,
                                    Coord where,
                                    Commodity const& comm );
void render_commodity_annotated_20( rr::Renderer& renderer,
                                    Coord where,
                                    Commodity const& comm );

} // namespace rn
