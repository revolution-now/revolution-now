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

struct Plane;
struct Planes;
struct Player;
struct SS;
struct TS;

/****************************************************************
** HarborPlane
*****************************************************************/
struct HarborPlane {
  HarborPlane( SS& ss, TS& ts, Player& player );

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
wait<> show_harbor_view( Planes& planes, SS& ss, TS& ts,
                         Player&       player,
                         maybe<UnitId> selected_unit );

} // namespace rn
