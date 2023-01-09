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

#include "core-config.hpp"

// Rds
#include "land-view-render.rds.hpp"

// Revolution Now
#include "render.hpp"

// ss
#include "ss/colony-id.hpp"
#include "ss/dwelling-id.hpp"
#include "ss/ref.hpp"
#include "ss/unit-id.hpp"

// gfx
#include "gfx/coord.hpp"

// C++ standard library
#include <unordered_map>
#include <vector>

namespace rr {
struct Renderer;
}

namespace rn {

class SmoothViewport;
struct SSConst;
struct Visibility;

/****************************************************************
** LandViewRenderer
*****************************************************************/
struct LandViewRenderer {
  using UnitAnimationsMap =
      std::unordered_map<GenericUnitId, UnitAnimation_t>;
  using DwellingAnimationsMap =
      std::unordered_map<DwellingId, DwellingAnimation_t>;
  using ColonyAnimationsMap =
      std::unordered_map<ColonyId, ColonyAnimation_t>;

  LandViewRenderer(
      SSConst const& ss, rr::Renderer& renderer_arg,
      UnitAnimationsMap const&     unit_animations,
      DwellingAnimationsMap const& dwelling_animations,
      ColonyAnimationsMap const&   colony_animations,
      Visibility const& viz, maybe<UnitId> last_unit_input,
      Rect                         viewport_rect_pixels,
      maybe<InputOverrunIndicator> input_overrun_indicator,
      SmoothViewport const&        viewport );

  void render_entities() const;

  void render_non_entities() const;

 private:
  void render_units() const;

  void render_native_dwellings() const;

  void render_units_under_colonies() const;

  void render_colonies() const;

  Rect render_rect_for_tile( Coord tile ) const;

  void render_single_unit( Coord where, GenericUnitId id,
                           e_flag_count flag_count ) const;

  void render_units_on_square( Coord tile, bool flags ) const;

  std::vector<std::pair<Coord, GenericUnitId>> units_to_render()
      const;

  void render_units_default() const;

  void render_single_unit_depixelate_to(
      Coord where, GenericUnitId id, bool multiple_units,
      double stage, e_tile target_tile ) const;

  void render_units_impl() const;

  void render_native_dwelling( Dwelling const& dwelling ) const;

  void render_native_dwelling_depixelate(
      Dwelling const& dwelling ) const;

  void render_input_overrun_indicator() const;

  void render_colony( Colony const& colony ) const;

  void render_colony_depixelate( Colony const& colony ) const;

  void render_backdrop() const;

  // Note: SSConst needs to be held by value since otherwise it
  // would be a reference to the temporary created at the call
  // site by converting from SS.
  SSConst const                ss_;
  rr::Renderer&                renderer_;
  rr::Renderer&                renderer; // no _ for macros.
  UnitAnimationsMap const&     unit_animations_;
  DwellingAnimationsMap const& dwelling_animations_;
  ColonyAnimationsMap const&   colony_animations_;
  Rect                         covered_;
  Visibility const&            viz_;
  maybe<UnitId>                last_unit_input_;
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
    SSConst const& ss, Coord tile );

} // namespace rn
