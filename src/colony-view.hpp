/****************************************************************
**colony-view.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-05.
*
* Description: The view that appears when clicking on a colony.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "colony-id.hpp"
#include "wait.hpp"

namespace rn {

struct Colony;
struct Plane;
struct Player;
struct SS;
struct TS;

/****************************************************************
** ColonyPlane
*****************************************************************/
struct ColonyPlane {
  ColonyPlane( SS& ss, TS& ts, Colony& colony, Player& player );

  ~ColonyPlane();

  wait<> show_colony_view() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  Plane& impl();
};

/****************************************************************
** API
*****************************************************************/
wait<> show_colony_view( SS& ss, TS& ts, Colony& colony );

} // namespace rn
