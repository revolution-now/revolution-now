/****************************************************************
**terrain.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-14.
*
* Description: Save-game state for terrain data.
*
*****************************************************************/
#pragma once

// Rds
#include "gs/terrain.rds.hpp"

// gfx
#include "gfx/coord.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

// base
#include "base/expect.hpp"
#include "base/maybe.hpp"

namespace rn {

struct TerrainState {
  TerrainState();
  bool operator==( TerrainState const& ) const = default;

  // Implement refl::WrapsReflected.
  TerrainState( wrapped::TerrainState&& o );
  wrapped::TerrainState const&      refl() const { return o_; }
  static constexpr std::string_view refl_ns   = "rn";
  static constexpr std::string_view refl_name = "TerrainState";

  Matrix<MapSquare> const& world_map() const;

  Delta world_size_tiles() const;
  Rect  world_rect_tiles() const;

  bool square_exists( Coord coord ) const;

  MapSquare const&              square_at( Coord coord ) const;
  base::maybe<MapSquare const&> maybe_square_at(
      Coord coord ) const;

  int  placement_seed() const { return o_.placement_seed; }
  void set_placement_seed( int seed ) {
    o_.placement_seed = seed;
  }

  // This essentially returns what square_at does, except it also
  // returns valid values for any squares outside of the map, in
  // which case it will return the "proto" squares specified in
  // the terrain data structure. This is very useful for ren-
  // dering for two reasons: 1) the renderer doesn't have to make
  // special cases for tiles on the map edge, and 2) it helps to
  // replicate the behavior of the original game where tiles at
  // the map edge are rendered as if there are more tiles off of
  // the map. The word "total" in the name refers to the fact
  // that, unlike `square_at`, this is a total function.
  MapSquare const& total_square_at( Coord coord ) const;

  // Given a tile coordinate that is off of the map, this will
  // return the proto square direction for it. If the tile is on
  // the map then it returns nothing.
  base::maybe<e_cardinal_direction>
  proto_square_direction_for_tile( Coord tile ) const;

  MapSquare const& proto_square( e_cardinal_direction d ) const;

  // Throws if coord is not on map.
  bool is_land( Coord coord ) const;

  // These should not be called directly; they should only be
  // called by the MapUpdater classes, which is what you should
  // be using whenever you modify terrain in order to ensure that
  // the map gets redrawn accordingly. If you don't want to
  // redraw a map (e.g. you are in unit tests) then just use the
  // non-rendering map updater.
  Matrix<MapSquare>&      mutable_world_map();
  MapSquare&              mutable_square_at( Coord coord );
  base::maybe<MapSquare&> mutable_maybe_square_at( Coord coord );

  // This should only be called by code doing map generation.
  // Once a map is generated, the proto squares should not be
  // modified.
  MapSquare& mutable_proto_square( e_cardinal_direction d );

 private:
  base::valid_or<std::string> validate() const;
  void                        validate_or_die() const;

  // ----- Serializable state.
  wrapped::TerrainState o_;

  // ----- Non-serializable (transient) state.
  // none.
};

using ProtoSquaresMap =
    refl::enum_map<e_cardinal_direction, MapSquare>;

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS( ::rn::TerrainState, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::ProtoSquaresMap, owned_by_cpp ){};
}
