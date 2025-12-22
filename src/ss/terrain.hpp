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
#include "ss/terrain.rds.hpp"

// gfx
#include "gfx/coord.hpp"

// luapp
#include "luapp/ext-usertype.hpp"

// base
#include "base/expect.hpp"
#include "base/function-ref.hpp"
#include "base/maybe.hpp"

namespace rn {

struct TerrainState {
  TerrainState();
  bool operator==( TerrainState const& ) const = default;

  // Implement refl::WrapsReflected.
  TerrainState( wrapped::TerrainState&& o );
  wrapped::TerrainState const& refl() const { return o_; }
  static constexpr std::string_view refl_ns   = "rn";
  static constexpr std::string_view refl_name = "TerrainState";

  gfx::Matrix<MapSquare> const& world_map() const;
  RealTerrain const& real_terrain() const;

  Delta world_size_tiles() const;
  Rect world_rect_tiles() const;

  bool square_exists( Coord coord ) const;
  bool square_exists( gfx::point tile ) const;

  MapSquare const& square_at( Coord coord ) const;
  MapSquare const& square_at( gfx::point tile ) const;
  base::maybe<MapSquare const&> maybe_square_at(
      Coord coord ) const;
  base::maybe<MapSquare const&> maybe_square_at(
      gfx::point tile ) const;

  int placement_seed() const { return o_.placement_seed; }
  void set_placement_seed( int seed ) {
    o_.placement_seed = seed;
  }

  base::maybe<PlayerTerrain const&> player_terrain(
      e_player player ) const;

  // This add (or overwrite) the nation's terrain object and, if
  // visible=true, will copy all the terrain squares over to it,
  // making them visible (otherwise all squares will be
  // non-visible).
  void initialize_player_terrain( e_player player,
                                  bool visible );

  void testing_reset_player_terrain( e_player player );

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
  MapSquare const& total_square_at( gfx::point tile ) const;

  // Given a tile coordinate that is off of the map, this will
  // return the proto square direction for it. If the tile is on
  // the map then it returns nothing.
  base::maybe<e_cardinal_direction>
  proto_square_direction_for_tile( Coord tile ) const;
  base::maybe<e_cardinal_direction>
  proto_square_direction_for_tile( gfx::point tile ) const;

  MapSquare const& proto_square( e_cardinal_direction d ) const;

  // Throws if coord is not on map.
  bool is_land( Coord coord ) const;

  std::vector<int> const& pacific_ocean_endpoints() const {
    return o_.pacific_ocean_endpoints;
  }

  // This one is OK to call from normal game code since it
  // doesn't require any redrawing of the map, since pacific
  // ocean tiles aren't rendered specially. That said, normal
  // game code should never really need to change pacific ocean
  // states of the map.
  std::vector<int>& pacific_ocean_endpoints() {
    return o_.pacific_ocean_endpoints;
  }

  bool is_pacific_ocean( Coord coord ) const;

  // These below should not be called directly; they should only
  // be called by the MapUpdater classes, which is what you
  // should be using whenever you modify terrain in order to en-
  // sure that the map gets redrawn accordingly. If you don't
  // want to redraw a map (e.g. you are in unit tests) then just
  // use the non-rendering map updater.

  MapSquare& mutable_square_at( Coord coord );
  base::maybe<MapSquare&> mutable_maybe_square_at( Coord coord );
  MapSquare& mutable_square_at( gfx::point tile );
  base::maybe<MapSquare&> mutable_maybe_square_at(
      gfx::point tile );
  PlayerTerrain& mutable_player_terrain( e_player player );

  // Whenever the map matrix is modified as a whole (which could
  // involve changing its dimensions) it must always be done via
  // this method so that invariants of this class will be upheld.
  void modify_entire_map(
      base::function_ref<void( RealTerrain& )> mutator );

  // This should only be called by code doing map generation.
  // Once a map is generated, the proto squares should not be
  // modified.
  MapSquare& mutable_proto_square( e_cardinal_direction d );

  // Lua bindings.
  friend void define_usertype_for( lua::state& st,
                                   lua::tag<TerrainState> );

 private:
  base::valid_or<std::string> validate() const;
  void validate_or_die() const;

  // ----- Serializable state.
  wrapped::TerrainState o_;

  // ----- Non-serializable (transient) state.
  // none.
};

} // namespace rn
