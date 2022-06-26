/****************************************************************
**land-view.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-29.
*
* Description: Handles the main game view of the land.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "colony-id.hpp"
#include "unit-id.hpp"
#include "wait.hpp"

// Rds
#include "land-view.rds.hpp"
#include "orders.rds.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct IGui;
struct IMapUpdater;
struct MenuPlane;
struct LandViewState;
struct Plane;
struct SettingsState;
struct TerrainState;
struct WindowPlane;

enum class e_depixelate_anim { death, demote };

/****************************************************************
** LandViewPlane
*****************************************************************/
struct LandViewPlane {
  LandViewPlane( MenuPlane&          menu_plane,
                 WindowPlane&        window_plane,
                 LandViewState&      land_view_state,
                 TerrainState const& terrain_state,
                 IMapUpdater& map_updater, IGui& gui );

  ~LandViewPlane();

  wait<> landview_ensure_visible( Coord const& coord );
  wait<> landview_ensure_visible( UnitId id );

  wait<LandViewPlayerInput_t> landview_get_next_input(
      UnitId id );

  wait<LandViewPlayerInput_t> landview_eot_get_next_input();

  wait<> landview_animate_move(
      TerrainState const&  terrain_state,
      SettingsState const& settings, UnitId id,
      e_direction direction );

  wait<> landview_animate_attack( SettingsState const& settings,
                                  UnitId               attacker,
                                  UnitId               defender,
                                  bool attacker_wins,
                                  e_depixelate_anim dp_anim );

  wait<> landview_animate_colony_capture(
      TerrainState const&  terrain_state,
      SettingsState const& settings, UnitId attacker_id,
      UnitId defender_id, ColonyId colony_id );

  // Clear any buffer input.
  void landview_reset_input_buffers();

  // We don't have to do much specifically in the land view when
  // we start a new turn, but there are a couple of small things
  // to do for a polished user experience.
  void landview_start_new_turn();

  // Zoom out just enough to see the entire map plus a bit of
  // border around it.
  void zoom_out_full();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  Plane& impl();
};

} // namespace rn
