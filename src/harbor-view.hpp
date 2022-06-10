/****************************************************************
**harbor-view.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-06-14.
*
* Description: Implements the harbor view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "plane-stack.hpp"
#include "unit-id.hpp"
#include "wait.hpp"

namespace rn {

struct IGui;
struct Planes;
struct Player;
struct UnitsState;
struct TerrainState;

/****************************************************************
** HarborPlane
*****************************************************************/
struct HarborPlane {
  HarborPlane( Planes& planes, e_plane_stack where,
               Player& player, UnitsState& units_state,
               TerrainState const& terrain_state, IGui& gui );

  ~HarborPlane() noexcept;

  void set_selected_unit( UnitId id );

  wait<> show_harbor_view();

 private:
  Planes&             planes_;
  e_plane_stack const where_;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace rn
