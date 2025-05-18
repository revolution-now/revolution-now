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

// Revolution Now
#include "unit-id.hpp"
#include "wait.hpp"

namespace rn {

struct IEngine;
struct IPlane;
struct Player;
struct SS;
struct TS;

/****************************************************************
** IHarborViewer
*****************************************************************/
struct IHarborViewer {
  virtual ~IHarborViewer() = default;

  virtual void set_selected_unit( UnitId id ) = 0;

  virtual wait<> show() = 0;
};

/****************************************************************
** HarborViewer
*****************************************************************/
struct HarborViewer : public IHarborViewer {
  HarborViewer( IEngine& engine, SS& ss, TS& ts,
                Player& player );

  void set_selected_unit( UnitId id ) override;

  wait<> show() override;

 private:
  IEngine& engine_;
  SS& ss_;
  TS& ts_;
  Player& player_;
};

} // namespace rn
