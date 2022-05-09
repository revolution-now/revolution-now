/****************************************************************
**gs-terrain.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-14.
*
* Description: Save-game state for terrain data.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "coord.hpp"
#include "expect.hpp"

// Rds
#include "gs-terrain.rds.hpp"

namespace rn {

struct TerrainState {
  TerrainState();
  bool operator==( TerrainState const& ) const = default;

  TerrainState( TerrainState&& ) = default;
  TerrainState& operator=( TerrainState&& ) = default;

  // Implement refl::WrapsReflected.
  TerrainState( wrapped::TerrainState&& o );
  wrapped::TerrainState const&      refl() const { return o_; }
  static constexpr std::string_view refl_ns   = "rn";
  static constexpr std::string_view refl_name = "TerrainState";

  Matrix<MapSquare> const& world_map() const;

  Delta world_size_tiles() const;
  Rect  world_rect_tiles() const;

  bool square_exists( Coord coord ) const;

  MapSquare const&        square_at( Coord coord ) const;
  maybe<MapSquare const&> maybe_square_at( Coord coord ) const;

  // This essentially returns what square_at does, except it also
  // returns valid values for any squares outside of the map, in
  // which case it will return arctic squares for the top and
  // bottom and sea lane squares for the left and right sides.
  // This is very useful for rendering for two reasons: 1) the
  // renderer doesn't have to make special cases for tiles on the
  // map edge, and 2) it helps to replicate the behavior of the
  // original game where tiles at the map edge are rendered as if
  // there are more tiles off of the map.
  //
  // The word "total" in the name refers to the fact that, unlike
  // `square_at`, this is a total function.
  MapSquare const& total_square_at( Coord coord ) const;

  // Throws if coord is not on map.
  bool is_land( Coord coord ) const;

  // Note: if you call these to mutate the map, be sure to
  // re-render the changed parts (or the entire thing).
  Matrix<MapSquare>& mutable_world_map();
  MapSquare&         mutable_square_at( Coord coord );
  maybe<MapSquare&>  mutable_maybe_square_at( Coord coord );

 private:
  valid_or<std::string> validate() const;
  void                  validate_or_die() const;

  // ----- Serializable state.
  wrapped::TerrainState o_;

  // ----- Non-serializable (transient) state.
  // none.
};

} // namespace rn
