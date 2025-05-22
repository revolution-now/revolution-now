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
struct MapRevealed;
struct MapSquare;
struct PlayerSquare;
struct PlayerTerrain;
struct SS;
struct SSConst;
struct SSConst;
struct TS;
struct TerrainState;

enum class e_natural_resource;

/****************************************************************
** IVisibility
*****************************************************************/
// This allows asking for the contents and visibility status of a
// map square in a generic way that works when rendering either a
// player-specific map or an all-visible map.
struct IVisibility {
  IVisibility( SSConst const& ss );

  virtual ~IVisibility() = default;

  // Are we viewing from the perspective of a player or not.
  virtual base::maybe<e_player> player() const = 0;

  // Returns if the tile is visible in this rendering. If the
  // tile if off-map then they are always hidden (proto square).
  virtual e_tile_visibility visible( gfx::point tile ) const = 0;

  virtual maybe<Colony const&> colony_at(
      gfx::point tile ) const = 0;

  virtual maybe<Dwelling const&> dwelling_at(
      gfx::point tile ) const = 0;

  // For rendering purposes, this should be called to determine
  // whether there is a prime resource actually visible on the
  // square, since there is some logic involved in doing that be-
  // yond just looking at the MapSquare object.
  virtual maybe<e_natural_resource> resource_at(
      gfx::point tile ) const final;

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
  virtual MapSquare const& square_at(
      gfx::point tile ) const = 0;

  // For convenience.
  Rect rect_tiles() const;

  // For convenience. Is the tile on the map.
  bool on_map( gfx::point tile ) const;

 private:
  // There are a couple of situations where we don't want to
  // render a prime resource on the map, either due to the other
  // properties of the map square or entities on top of the
  // square. This allows us to keep that logic in this module in-
  // stead of the rendering module; better that the latter is
  // kept as dumb as possible for unit testing purposes. The ren-
  // derer will call the resource_at() method, which in turn will
  // call this one.
  bool is_resource_suppressed( gfx::point tile ) const;

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
  base::maybe<e_player> player() const override {
    return base::nothing;
  };

  e_tile_visibility visible( gfx::point ) const override;

  maybe<Colony const&> colony_at(
      gfx::point tile ) const override;

  maybe<Dwelling const&> dwelling_at(
      gfx::point tile ) const override;

  MapSquare const& square_at( gfx::point tile ) const override;

 private:
  SSConst const& ss_;
};

/****************************************************************
** VisibilityForPlayer
*****************************************************************/
// For when we are viewing the map from a player's perspective.
struct VisibilityForPlayer : IVisibility {
  VisibilityForPlayer( SSConst const& ss, e_player player );

 public: // Implement IVisibility.
  base::maybe<e_player> player() const override {
    return player_;
  };

  e_tile_visibility visible( gfx::point tile ) const override;

  maybe<Colony const&> colony_at(
      gfx::point tile ) const override;

  maybe<Dwelling const&> dwelling_at(
      gfx::point tile ) const override;

  MapSquare const& square_at( gfx::point tile ) const override;

 private:
  maybe<PlayerSquare const&> player_square_at(
      gfx::point tile ) const;

  SSConst const& ss_;
  VisibilityEntire entire_;
  e_player const player_                     = {};
  PlayerTerrain const* const player_terrain_ = nullptr;
};

/****************************************************************
** VisibilityWithOverrides
*****************************************************************/
// Delegates to a provided IVisibility object except for a cer-
// tain set of tiles whose values will be overridden.
struct VisibilityWithOverrides : IVisibility {
  VisibilityWithOverrides(
      SSConst const& ss, IVisibility const& underlying,
      VisibilityOverrides const& overrides ATTR_LIFETIMEBOUND );

 public: // Implement IVisibility.
  base::maybe<e_player> player() const override {
    return underlying_.player();
  };

  e_tile_visibility visible( gfx::point tile ) const override;

  maybe<Colony const&> colony_at(
      gfx::point tile ) const override;

  maybe<Dwelling const&> dwelling_at(
      gfx::point tile ) const override;

  MapSquare const& square_at( gfx::point tile ) const override;

 private:
  IVisibility const& underlying_;
  VisibilityOverrides const& overrides_;
};

/****************************************************************
** Public API
*****************************************************************/
// Selects the standard instance usable given the player status.
std::unique_ptr<IVisibility const> create_visibility_for(
    SSConst const& ss, maybe<e_player> player );

// Returns true if, were that nation's view to be currently ren-
// dered, the square would be visible and clear. This could mean
// that it is actively visible by some unit now, or it could mean
// that it was explored earlier in the turn and is still consid-
// ered visible and clear.
bool does_player_have_fog_removed_on_square( SSConst const& ss,
                                             e_player player,
                                             gfx::point tile );

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
                                         e_player player,
                                         e_unit_type type,
                                         gfx::point tile );

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
void recompute_fog_for_player( SS& ss, TS& ts, e_player player );

// This will update map visibility to be front the perspective of
// the given player (or the entire map visible if the `nation`
// parameter is nothing), This process may include redrawing the
// map if necessary.
void update_map_visibility( TS& ts, maybe<e_player> player );

// Used to determine if a unit move should be animated, which
// happens if either the source or destination tiles of the move
// (or attack) is visible and clear.
bool should_animate_move( IVisibility const& viz, gfx::point src,
                          gfx::point dst );

/****************************************************************
** ScopedMapViewer
*****************************************************************/
// This is used when moving some AI-controlled unit on the map in
// a way that interacts with a european unit. If that european
// unit is human then this will temporarily change the active
// viewer (including redraw the map) if needed so that the map is
// being viewed from that player's perspective during the inter-
// action. An example of this would be that we have two human
// players, english and french, and the native units are moving.
// Since the english comes before the french and since the eng-
// lish are human, they will be the default viewers while the na-
// tives are moving. However, if a native unit moves to attack a
// french player, then since the french player is human, we need
// to temporarily change the map viewer to be from the french
// perspective while the interaction is happening, then change it
// back to whatever it was. Note that if the entire map is vis-
// ible from the start, then no change will be done.
struct [[nodiscard]] ScopedMapViewer {
  ScopedMapViewer( SS& ss, TS& ts, e_player const player );
  ~ScopedMapViewer();

  ScopedMapViewer( ScopedMapViewer&& ) = delete;

 private:
  bool needs_change() const;

  SS& ss_;
  TS& ts_;
  maybe<e_player> const old_player_;
  // unique_ptr so that we can fwd declare.
  std::unique_ptr<MapRevealed const> const old_map_revealed_;
  e_player const new_player_;
};

} // namespace rn
