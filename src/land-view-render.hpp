/****************************************************************
**land-view-render.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-01-09.
*
* Description: Handles rendering for the land view.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "land-view-anim.rds.hpp"
#include "render.hpp"
#include "time.hpp"

// ss
#include "ss/colony-id.hpp"
#include "ss/ref.hpp"
#include "ss/unit-id.hpp"

// gfx
#include "gfx/coord.hpp"

// C++ standard library
#include <vector>

namespace rr {
struct Renderer;
enum class e_render_buffer;
}

namespace rn {

class SmoothViewport;

struct DwellingAnimationState;
struct LandViewAnimator;
struct SSConst;
struct IVisibility;
struct UnitFlagOptions;
struct VisibilityOverrides;

// A fading hourglass icon will be drawn over a unit to signal to
// the player that the movement command just entered will be
// thrown out in order to avoid inadvertantly giving the new unit
// an order intended for the old unit.
struct InputOverrunIndicator {
  UnitId unit_id    = {};
  Time_t start_time = {};
};

/****************************************************************
** LandViewRenderer
*****************************************************************/
struct LandViewRenderer {
  LandViewRenderer(
      SSConst const& ss, rr::Renderer& renderer_arg,
      LandViewAnimator const&                   lv_animator,
      std::unique_ptr<IVisibility const> const& viz,
      maybe<UnitId> last_unit_input, Rect viewport_rect_pixels,
      maybe<InputOverrunIndicator> input_overrun_indicator,
      SmoothViewport const&        viewport );

  // Units, colonies, dwellings.
  void render_entities() const;

  // Landscape, backdrop.
  void render_non_entities() const;

 private:
  void render_units() const;

  void render_dwellings() const;

  void render_landscape_anim_buffers() const;

  void render_landscape_anim_buffer(
      maybe<LandscapeAnimReplacementState const&> state ) const;

  void render_landscape_anim_buffer(
      maybe<LandscapeAnimEnpixelationState const&> state ) const;

  void render_landscape_anim_buffer_impl(
      rr::e_render_buffer        buffer,
      std::vector<Coord> const&  redrawn,
      VisibilityOverrides const& overrides ) const;

  void render_units_underneath() const;

  void render_colonies() const;

  Rect render_rect_for_tile( Coord tile ) const;

  void render_single_unit(
      Coord where, GenericUnitId id,
      maybe<UnitFlagOptions> flag_count ) const;

  void render_units_on_square( Coord tile, bool flags ) const;

  std::vector<std::pair<Coord, GenericUnitId>> units_to_render()
      const;

  void render_units_default() const;

  void render_single_unit_depixelate_to(
      Coord where, UnitId id, bool multiple_units, double stage,
      e_unit_type target_type ) const;

  void render_single_native_unit_depixelate_to(
      Coord where, NativeUnitId id, bool multiple_units,
      double stage, e_native_unit_type target_type ) const;

  void render_units_impl() const;

  void render_dwelling( Dwelling const& dwelling,
                        Coord           tile ) const;

  void render_dwelling_depixelate(
      DwellingAnimationState const& anim,
      Dwelling const& dwelling, Coord tile ) const;

  Coord dwelling_pixel_coord_from_tile( Coord tile ) const;

  void render_input_overrun_indicator() const;

  void render_colony( Colony const& colony ) const;

  void render_colony_depixelate( Colony const& colony ) const;

  Coord colony_pixel_coord_from_tile( Coord tile ) const;

  void render_backdrop() const;

  SSConst const&          ss_;
  rr::Renderer&           renderer_;
  rr::Renderer&           renderer; // no _ for macros.
  LandViewAnimator const& lv_animator_;
  Rect                    covered_;
  // We use the unique_ptr here because we need to know when the
  // source instance changed its value.
  std::unique_ptr<IVisibility const> const& viz_;
  maybe<UnitId>                             last_unit_input_;
  Rect                         viewport_rect_pixels_;
  maybe<InputOverrunIndicator> input_overrun_indicator_;
  SmoothViewport const&        viewport_;
};

/****************************************************************
** Free Functions
*****************************************************************/
// Given a tile, this will return the ordered unit stack on that
// tile (if any units are present), in the order that they would
// be rendered. The top-most unit is first.
std::vector<GenericUnitId> land_view_unit_stack(
    SSConst const& ss, Coord tile,
    maybe<UnitId> last_unit_input );

} // namespace rn
