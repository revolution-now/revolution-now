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

// Rds
#include "land-view.rds.hpp"

// Revolution Now
#include "unit-id.hpp"
#include "wait.hpp"

// ss
#include "ss/nation.rds.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct AnimationSequence;
struct Colony;
struct IEngine;
struct IPlane;
struct SS;
struct TS;
struct ViewportController;

/****************************************************************
** ILandViewPlane
*****************************************************************/
struct ILandViewPlane {
  virtual ~ILandViewPlane() = default;

  virtual void set_visibility( maybe<e_player> player ) = 0;

  virtual wait<> ensure_visible( Coord const& coord ) = 0;

  // Tries to center on the tile (smoothly, if not too far away)
  // as much as is possible. E.g. if the tile is close to the
  // world edge then it won't end up centered.
  virtual wait<> center_on_tile( gfx::point tile ) = 0;

  virtual wait<> ensure_visible_unit( GenericUnitId id ) = 0;

  // Does a brief animation to reveal the underlying ground tiles
  // in a way similar to the OG, without revealing any informa-
  // tion to the player that they wouldn't otherwise know (i.e.
  // will not reveal prime resources under forests or LCRs). This
  // should not be called while there are other animations run-
  // ning, otherwise it might conflict.
  virtual wait<> show_hidden_terrain() = 0;

  // Only needs to be called mid-turn, since the end-of-turn
  // state is also a view mode.
  virtual wait<LandViewPlayerInput> show_view_mode(
      ViewModeOptions options ) = 0;

  virtual wait<LandViewPlayerInput> get_next_input(
      UnitId id ) = 0;

  virtual wait<LandViewPlayerInput> eot_get_next_input() = 0;

  // We use the lifetime-bound attribute here because it is not
  // uncommon for this coroutine to be run in the background
  // (i.e., not immediately awaited upon), and so this will help
  // catch lifetime issues. Technically we should do this to all
  // coroutines, but most aren't used in this way.
  virtual wait<> animate(
      AnimationSequence const& seq ATTR_LIFETIMEBOUND ) = 0;

  // We don't have to do much specifically in the land view when
  // we start a new turn, but there are a couple of small things
  // to do for a polished user experience.
  virtual void start_new_turn() = 0;

  // Zoom out just enough to see the entire map plus a bit of
  // border around it.
  virtual void zoom_out_full() = 0;

  // If there is a unit blinking and asking for commands then
  // this will return it.
  virtual maybe<UnitId> unit_blinking() const = 0;

  // If visible, will be returned. Note that this is distinct
  // from the the API in the white-box module which will always
  // return a value.
  virtual maybe<gfx::point> white_box() const = 0;

  virtual ViewportController& viewport() const = 0;

  virtual IPlane& impl() = 0;
};

/****************************************************************
** LandViewPlane
*****************************************************************/
struct LandViewPlane : ILandViewPlane {
  LandViewPlane( IEngine& engine, SS& ss, TS& ts,
                 maybe<e_player> visibility );

  ~LandViewPlane() override;

 public: // Implement ILandViewPlane.
  void set_visibility( maybe<e_player> player ) override;

  wait<> ensure_visible( Coord const& coord ) override;
  wait<> center_on_tile( gfx::point tile ) override;
  wait<> ensure_visible_unit( GenericUnitId id ) override;

  wait<> show_hidden_terrain() override;

  wait<LandViewPlayerInput> show_view_mode(
      ViewModeOptions options ) override;

  wait<LandViewPlayerInput> get_next_input( UnitId id ) override;

  wait<LandViewPlayerInput> eot_get_next_input() override;

  wait<> animate( AnimationSequence const& seq ) override;

  void start_new_turn() override;

  void zoom_out_full() override;

  maybe<UnitId> unit_blinking() const override;

  maybe<gfx::point> white_box() const override;

  ViewportController& viewport() const override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  IPlane& impl() override;
};

} // namespace rn
