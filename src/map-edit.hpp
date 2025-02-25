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

struct IEngine;
struct IPlane;
struct Planes;
struct SS;
struct TS;

/****************************************************************
** MapEditPlane
*****************************************************************/
struct MapEditPlane {
  MapEditPlane( IEngine& engine, SS& ss, TS& ts );
  ~MapEditPlane();

  wait<> run_map_editor();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  IPlane& impl();
};

/****************************************************************
** API
*****************************************************************/
wait<> run_map_editor( IEngine& engine, SS& ss, TS& ts );

wait<> run_map_editor_standalone( IEngine& engine,
                                  Planes& planes );

} // namespace rn
