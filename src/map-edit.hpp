/****************************************************************
**map-edit.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-04-03.
*
* Description: Map Editor.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "wait.hpp"

namespace rn {

struct Plane;
struct SS;
struct TS;

/****************************************************************
** MapEditPlane
*****************************************************************/
struct MapEditPlane {
  MapEditPlane( SS& ss, TS& ts );
  ~MapEditPlane();

  wait<> run_map_editor();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  Plane& impl();
};

/****************************************************************
** API
*****************************************************************/
wait<> run_map_editor( SS& ss, TS& ts, bool standalone_mode );

} // namespace rn
