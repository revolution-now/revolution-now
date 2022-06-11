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
#include "unit-id.hpp"
#include "wait.hpp"

namespace rn {

struct IGui;
struct Plane;
struct Planes;
struct Player;
struct UnitsState;
struct TerrainState;

/****************************************************************
** HarborPlane
*****************************************************************/
struct HarborPlane {
  HarborPlane( Player& player, UnitsState& units_state,
               TerrainState const& terrain_state, IGui& gui );

  ~HarborPlane();

  void set_selected_unit( UnitId id );

  wait<> show_harbor_view();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  Plane& impl();
};

/****************************************************************
** API
*****************************************************************/
wait<> show_harbor_view( Planes& planes, Player& player,
                         UnitsState&         units_state,
                         TerrainState const& terrain_state,
                         maybe<UnitId>       selected_unit );

} // namespace rn
