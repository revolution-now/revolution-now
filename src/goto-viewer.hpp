/****************************************************************
**goto-viewer.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-27.
*
* Description: Implementations of IGotoMapViewer.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "igoto-viewer.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct IVisibility;
struct SSConst;
struct Unit;

/****************************************************************
** GotoMapViewer
*****************************************************************/
// NOTE: the unit should not be deleted while this object is
// being used.
struct GotoMapViewer : IGotoMapViewer {
  GotoMapViewer( SSConst const& ss, Unit const& unit );
  ~GotoMapViewer() override;

  bool can_enter_tile( gfx::point tile ) const override;

  e_map_side map_side( gfx::point tile ) const override;

  e_map_side_edge is_on_map_side_edge(
      gfx::point tile ) const override;

  maybe<bool> is_sea_lane( gfx::point tile ) const override;

  maybe<bool> has_lcr( gfx::point tile ) const override;

  MovementPoints movement_points_required(
      gfx::point src, e_direction direction ) const override;

 private:
  SSConst const& ss_;
  Unit const& unit_;
  std::unique_ptr<IVisibility const> const viz_;
  bool const is_ship_ = {};
};

static_assert( !std::is_abstract_v<GotoMapViewer> );

} // namespace rn
