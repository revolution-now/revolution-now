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

// base
#include "base/vocab.hpp"

namespace rn {

struct Colony;
struct Plane;
struct Planes;
struct Player;
struct SS;
struct TS;

/****************************************************************
** ColonyPlane
*****************************************************************/
struct ColonyPlane {
  ColonyPlane( Planes& planes, SS& ss, TS& ts, Colony& colony );

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
enum class e_colony_abandoned { no, yes };

// Returns whether the colony was abandoned by the player while
// open. The caller needs to know about that so that it can take
// the appropriate measures to not access it after that, since it
// will no longer exist.
wait<base::NoDiscard<e_colony_abandoned>> show_colony_view(
    Planes& planes, SS& ss, TS& ts, Colony& colony );

} // namespace rn
