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

// rds
#include "visibility.rds.hpp"

// Revolution Now
#include "maybe.hpp"

// ss
#include "ss/nation.rds.hpp"
#include "ss/unit-type.rds.hpp"

// gfx
#include "gfx/coord.hpp"

// refl
#include "refl/enum-map.hpp"

// base
#include "base/attributes.hpp"

namespace rn {

struct Colony;
struct Dwelling;
struct FrozenSquare;
struct MapSquare;
struct PlayerSquare;
struct PlayerTerrain;
struct SS;
struct SSConst;
struct SSConst;
struct TS;
struct TerrainState;

/****************************************************************
** IVisibility
*****************************************************************/
// This allows asking for the contents and visibility status of a
// map square in a generic way that works when rendering either a
// player-specific map or an all-visible map.
struct IVisibility {
  IVisibility( SSConst const& ss );

  virtual ~IVisibility() = default;

  // Are we viewing from the perspective of a nation or not.
  virtual base::maybe<e_nation> nation() const = 0;

  // Returns if the tile is visible in this rendering. If the
  // tile if off-map then they are always hidden (proto square).
  virtual e_tile_visibility visible( Coord tile ) const = 0;

  virtual maybe<Colony const&> colony_at( Coord tile ) const = 0;

  virtual maybe<Dwelling const&> dwelling_at(
      Coord tile ) const = 0;

  // In general we're rendering the terrain from the point of
  // view of a player and so it may have only partial visibility.
  // Our square_at function thus will try the player's first,
  // then fall back to the real map. Even when rendering for a
  // player we still need access to the real map in some cases
  // because to render a (visible) tile we need to know the ter-
  // rain type of the adjacent tiles to render the spill-over
  // part. If the tile is not on the map, it will return one of
  // the proto squares (which are always considered to be "visi-
  // ble" to the player). Finally, if the tile is not entirely
  // hidden to the player, this will return either the real map
  // square (if visible and clear) or the player's version of it
  // (if fogged).
  virtual MapSquare const& square_at( Coord tile ) const = 0;

  // For convenience.
  Rect rect_tiles() const;

  // For convenience. Is the tile on the map.
  bool on_map( Coord tile ) const;

 protected:
  TerrainState const& terrain_;
};

/****************************************************************
** VisibilityEntire
*****************************************************************/
// For when we are viewing the entire map, not from any player's
// perspective; trivial implementation.
struct VisibilityEntire : IVisibility {
  VisibilityEntire( SSConst const& ss );

 public: // Implement IVisibility.
  base::maybe<e_nation> nation() const override {
    return base::nothing;
  };

  e_tile_visibility visible( Coord ) const override;

  virtual maybe<Colony const&> colony_at(
      Coord tile ) const override;

  virtual maybe<Dwelling const&> dwelling_at(
      Coord tile ) const override;

  MapSquare const& square_at( Coord tile ) const override;

 private:
  SSConst const& ss_;
};

/****************************************************************
** VisibilityForNation
*****************************************************************/
// For when we are viewing the map from a player's perspective.
struct VisibilityForNation : IVisibility {
  VisibilityForNation( SSConst const& ss, e_nation nation );

 public: // Implement IVisibility.
  base::maybe<e_nation> nation() const override {
    return nation_;
  };

  e_tile_visibility visible( Coord tile ) const override;

  virtual maybe<Colony const&> colony_at(
      Coord tile ) const override;

  virtual maybe<Dwelling const&> dwelling_at(
      Coord tile ) const override;

  MapSquare const& square_at( Coord tile ) const override;

 private:
  maybe<PlayerSquare const&> player_square_at(
      Coord tile ) const;

  SSConst const&             ss_;
  VisibilityEntire           entire_;
  e_nation const             nation_         = {};
  PlayerTerrain const* const player_terrain_ = nullptr;
};

/****************************************************************
** VisibilityWithOverrides
*****************************************************************/
// Delegates to a provided IVisibility object except for a cer-
// tain set of tiles whose values will be overridden.
struct VisibilityWithOverrides : IVisibility {
  VisibilityWithOverrides( SSConst const&     ss,
                           IVisibility const& underlying,
                           std::map<Coord, MapSquare> const&
                               overrides ATTR_LIFETIMEBOUND );

 public: // Implement IVisibility.
  base::maybe<e_nation> nation() const override {
    return underlying_.nation();
  };

  e_tile_visibility visible( Coord tile ) const override;

  virtual maybe<Colony const&> colony_at(
      Coord tile ) const override;

  virtual maybe<Dwelling const&> dwelling_at(
      Coord tile ) const override;

  // Implement IVisibility.
  MapSquare const& square_at( Coord tile ) const override;

 private:
  IVisibility const&                underlying_;
  std::map<Coord, MapSquare> const& overrides_;
};

/****************************************************************
** Public API
*****************************************************************/
// Selects the standard instance usable given the nation status.
std::unique_ptr<IVisibility const> create_visibility_for(
    SSConst const& ss, maybe<e_nation> nation );

// Returns true if, were that nation's view to be currently ren-
// dered, the square would be visible and clear. This could mean
// that it is actively visible by some unit now, or it could mean
// that it was explored earlier in the turn and is still consid-
// ered visible and clear.
bool does_nation_have_fog_removed_on_square( SSConst const& ss,
                                             e_nation nation,
                                             Coord    tile );

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

// Will restore all visible tiles' fog state to "fog enabled" un-
// less the square is within the sighting radius of a friendly
// unit or colony on the map. This is done once at the end of
// each player's turn as part of the fog-of-war mechanism in the
// game, which consists of allowing the player to remove fog as
// they move around the map during a single turn (without ever
// regenerating it) and then it gets regenerated at the end of
// their turn. This is done because a) it is much simpler and
// less tricky to implement correctly than a scheme that involves
// regeneration of fog as units move, and b) theoretically it is
// unnecessary anyway to regenerate fog during the turn since,
// even if we did, it would not hide anything that player hasn't
// already seen that turn; hiding something requires something to
// change on a tile that is not adjacent to a unit, but that can
// only happen after the player's turn ends and other players
// start moving.
void recompute_fog_for_nation( SS& ss, TS& ts, e_nation nation );

// This will update map visibility to be front the perspective of
// the given nation (or the entire map visible if the `nation`
// parameter is nothing), This process may include redrawing the
// map if necessary.
void update_map_visibility( TS& ts, maybe<e_nation> nation );

// Used to determine if a unit move should be animated, which
// happens if either the source or destination tiles of the move
// (or attack) is visible and clear.
bool should_animate_move( IVisibility const& viz, Coord src,
                          Coord dst );

} // namespace rn
