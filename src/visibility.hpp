/****************************************************************
**visibility.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-30.
*
* Description: Things related to hidden tiles and fog-of-war.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "maybe.hpp"

// ss
#include "ss/land-view.rds.hpp"
#include "ss/nation.rds.hpp"
#include "ss/unit-type.rds.hpp"

// gfx
#include "gfx/coord.hpp"

// refl
#include "refl/enum-map.hpp"

namespace rn {

struct MapSquare;
struct Planes;
struct PlayerTerrain;
struct SS;
struct SSConst;
struct TerrainState;
struct TS;

/****************************************************************
** Visibility
*****************************************************************/
// This allows asking for the contents and visibility status of a
// map square in a generic way that works when rendering either a
// player-specific map or an all-visible map. Specifically, when
// asking for a map square, it will always yield a value, but it
// will prefer the player's version of the square if there is a
// player view and if the player has seen that square. Otherwise
// it will retrieve the real map square. That said, there is also
// a method for querying whether a tile is visible. Note that all
// methods in this class are total, because they will return
// proto squares for tiles that are off-map.
struct Visibility {
  static Visibility create( SSConst const&        ss,
                            base::maybe<e_nation> nation );

  // Returns if the tile is visible in this rendering. If the
  // tile if off-map then false is returned (proto square).
  bool visible( Coord tile ) const;

  // In general we're rendering the terrain from the point of
  // view of a player and so it may have only partial visibility.
  // Our square_at function thus will try the player's first,
  // then fall back to the real map. Even when rendering for a
  // plyaer we still need access to the real map in some cases
  // because to render a (visible) tile we need to know the ter-
  // rain type of the adjacent tiles to render the spill-over
  // part. If the tile is not on the map, it will return one of
  // the proto squares (which are always considered to be "visi-
  // ble" to the player).
  MapSquare const& square_at( Coord tile ) const;

  // For convenience.
  Rect rect_tiles() const;

  // Is the tile on the map.
  bool on_map( Coord tile ) const;

 private:
  Visibility( TerrainState const&               terrain,
              base::maybe<PlayerTerrain const&> player_terrain )
    : terrain_( &terrain ),
      player_terrain_( player_terrain.fmap(
          []( auto& arg ) { return &arg; } ) ) {}

  // These are pointers instead of references so that the class
  // can be assigned.

  TerrainState const* terrain_;

  // If this does not have a value then we are rendering in the
  // mode where all tiles are fully visible.
  base::maybe<PlayerTerrain const*> player_terrain_;
};

/****************************************************************
** Public API
*****************************************************************/
// Compute which nations have at least one unit that currently
// has visibility on the given tile. To qualify, the nation must
// be able to currently see the tile.
refl::enum_map<e_nation, bool> nations_with_visibility_of_square(
    SSConst const& ss, Coord tile );

// This will look up the unit type's sighting radius, and then
// compute each (existing) square that the unit could see as-
// suming it is on the given tile, and will do so according to
// the OG's rules. This means that a land (sea) unit can see ALL
// squares in its immediate vicinity, plus all LAND (SEA) squares
// beyond that (and <= to its sighting radius). This looks a bit
// odd at first but is probably to prevent the ships with the ex-
// tended sighting radius from being able to reveal too many land
// tiles too easily, or a scout from being able to see on adja-
// cent islands (which would be easily possible after getting De
// Soto, in which case it would have 7x7 site).
std::vector<Coord> unit_visible_squares( SSConst const& ss,
                                         e_nation       nation,
                                         e_unit_type    type,
                                         Coord          tile );

// This is the function that should be used to determine the per-
// spective from which the land map should be rendered. It can be
// rendered from the perspective of a particular player, no
// player (entire map), or current player. This is used at the
// start of each player's turn (if they are human) to set the map
// to their view, and is also used by the cheat function "reveal
// map." The default_nation is the nation that we will use if
// `revealed` is nothing; if default_nation in turn is nothing
// then it means "entire map."
void set_map_visibility( Planes& planes, SS& ss, TS& ts,
                         maybe<MapRevealed_t const&> revealed,
                         maybe<e_nation> default_nation );

} // namespace rn
