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

// Rds
#include "land-view.rds.hpp"

// Revolution Now
#include "colony-id.hpp"
#include "land-view-anim.rds.hpp"
#include "unit-id.hpp"
#include "wait.hpp"

// ss
#include "ss/nation.rds.hpp"
#include "ss/unit-type.rds.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct Colony;
struct Plane;
struct Planes;
struct SS;
struct TS;

/****************************************************************
** ILandViewPlane
*****************************************************************/
struct ILandViewPlane {
  virtual ~ILandViewPlane() = default;

  virtual void set_visibility( maybe<e_nation> nation ) = 0;

  virtual wait<> ensure_visible( Coord const& coord ) = 0;

  // Tries to center on the tile (smoothly, if not too far away)
  // as much as is possible. E.g. if the tile is close to the
  // world edge then it won't end up centered.
  virtual wait<> center_on_tile( Coord coord ) = 0;

  virtual wait<> ensure_visible_unit( GenericUnitId id ) = 0;

  virtual wait<LandViewPlayerInput_t> get_next_input(
      UnitId id ) = 0;

  virtual wait<LandViewPlayerInput_t> eot_get_next_input() = 0;

  virtual wait<> animate_move( UnitId      id,
                               e_direction direction ) = 0;

  // This happens not when a colony is attacked or captured, but
  // when it is either abandoned or starved.
  virtual wait<> animate_colony_depixelation(
      Colony const& colony ) = 0;

  // Just pixelates a unit that is on the map.
  virtual wait<> animate_unit_pixelation(
      PixelationAnimation_t const& what ) = 0;

  virtual wait<> animate_attack(
      GenericUnitId attacker, GenericUnitId defender,
      std::vector<PixelationAnimation_t> const& animations,
      bool attacker_wins ) = 0;

  virtual wait<> animate_colony_capture(
      UnitId attacker_id, UnitId defender_id,
      std::vector<PixelationAnimation_t> const& animations,
      ColonyId                                  colony_id ) = 0;

  // Clear any buffer input.
  virtual void reset_input_buffers() = 0;

  // We don't have to do much specifically in the land view when
  // we start a new turn, but there are a couple of small things
  // to do for a polished user experience.
  virtual void start_new_turn() = 0;

  // Zoom out just enough to see the entire map plus a bit of
  // border around it.
  virtual void zoom_out_full() = 0;

  // If there is a unit blinking and asking for orders then this
  // will return it.
  virtual maybe<UnitId> unit_blinking() = 0;

  virtual Plane& impl() = 0;
};

/****************************************************************
** LandViewPlane
*****************************************************************/
struct LandViewPlane : ILandViewPlane {
  LandViewPlane( Planes& planes, SS& ss, TS& ts,
                 maybe<e_nation> visibility );

  ~LandViewPlane() override;

  void set_visibility( maybe<e_nation> nation ) override;

  wait<> ensure_visible( Coord const& coord ) override;
  wait<> center_on_tile( Coord coord ) override;
  wait<> ensure_visible_unit( GenericUnitId id ) override;

  wait<LandViewPlayerInput_t> get_next_input(
      UnitId id ) override;

  wait<LandViewPlayerInput_t> eot_get_next_input() override;

  wait<> animate_move( UnitId      id,
                       e_direction direction ) override;

  wait<> animate_colony_depixelation(
      Colony const& colony ) override;

  wait<> animate_unit_pixelation(
      PixelationAnimation_t const& what ) override;

  wait<> animate_attack(
      GenericUnitId attacker, GenericUnitId defender,
      std::vector<PixelationAnimation_t> const& depixelations,
      bool attacker_wins ) override;

  wait<> animate_colony_capture(
      UnitId attacker_id, UnitId defender_id,
      std::vector<PixelationAnimation_t> const& animations,
      ColonyId colony_id ) override;

  void reset_input_buffers() override;

  void start_new_turn() override;

  void zoom_out_full() override;

  maybe<UnitId> unit_blinking() override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  Plane& impl() override;
};

} // namespace rn
