/****************************************************************
**harbor-view.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-08.
*
* Description: The european harbor view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "unit-id.hpp"
#include "wait.hpp"

namespace rn {

struct IPlane;
struct Player;
struct SS;
struct TS;

/****************************************************************
** ColonyPlane
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
  IPlane& impl();
};

/****************************************************************
** API
*****************************************************************/
wait<> show_harbor_view( SS& ss, TS& ts, Player& player,
                         maybe<UnitId> selected_unit );

} // namespace rn
