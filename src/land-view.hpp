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

struct Colony;
struct Plane;
struct Planes;
struct SS;
struct TS;

enum class e_depixelate_anim { death, demote };

/****************************************************************
** ILandViewPlane
*****************************************************************/
struct ILandViewPlane {
  virtual ~ILandViewPlane() = default;

  virtual wait<> landview_ensure_visible(
      Coord const& coord ) = 0;

  virtual wait<> landview_ensure_visible_unit( UnitId id ) = 0;

  virtual wait<LandViewPlayerInput_t> landview_get_next_input(
      UnitId id ) = 0;

  virtual wait<LandViewPlayerInput_t>
  landview_eot_get_next_input() = 0;

  virtual wait<> landview_animate_move(
      UnitId id, e_direction direction ) = 0;

  // This happens not when a colony is attacked or captured, but
  // when it is either abandoned or starved.
  virtual wait<> landview_animate_colony_depixelation(
      Colony const& colony ) = 0;

  virtual wait<> landview_animate_attack(
      UnitId attacker, UnitId defender, bool attacker_wins,
      e_depixelate_anim dp_anim ) = 0;

  virtual wait<> landview_animate_colony_capture(
      UnitId attacker_id, UnitId defender_id,
      ColonyId colony_id ) = 0;

  // Clear any buffer input.
  virtual void landview_reset_input_buffers() = 0;

  // We don't have to do much specifically in the land view when
  // we start a new turn, but there are a couple of small things
  // to do for a polished user experience.
  virtual void landview_start_new_turn() = 0;

  // Zoom out just enough to see the entire map plus a bit of
  // border around it.
  virtual void zoom_out_full() = 0;

  virtual Plane& impl() = 0;
};

/****************************************************************
** LandViewPlane
*****************************************************************/
struct LandViewPlane : ILandViewPlane {
  LandViewPlane( Planes& planes, SS& ss, TS& ts );

  ~LandViewPlane() override;

  wait<> landview_ensure_visible( Coord const& coord ) override;
  wait<> landview_ensure_visible_unit( UnitId id ) override;

  wait<LandViewPlayerInput_t> landview_get_next_input(
      UnitId id ) override;

  wait<LandViewPlayerInput_t> landview_eot_get_next_input()
      override;

  wait<> landview_animate_move( UnitId      id,
                                e_direction direction ) override;

  wait<> landview_animate_colony_depixelation(
      Colony const& colony ) override;

  wait<> landview_animate_attack(
      UnitId attacker, UnitId defender, bool attacker_wins,
      e_depixelate_anim dp_anim ) override;

  wait<> landview_animate_colony_capture(
      UnitId attacker_id, UnitId defender_id,
      ColonyId colony_id ) override;

  void landview_reset_input_buffers() override;

  void landview_start_new_turn() override;

  void zoom_out_full() override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  Plane& impl() override;
};

} // namespace rn
