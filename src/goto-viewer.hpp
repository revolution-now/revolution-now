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
enum class e_player;
enum class e_unit_type;

struct IVisibility;
struct SSConst;
struct Unit;

/****************************************************************
** GotoMapViewer
*****************************************************************/
// NOTE: the unit should not be deleted while this object is
// being used.
struct GotoMapViewer : IGotoMapViewer {
  GotoMapViewer( SSConst const& ss, IVisibility const& viz,
                 e_player player_type, e_unit_type unit_type );
  ~GotoMapViewer() override = default;

  bool can_enter_tile( gfx::point tile ) const override;

  e_map_side map_side( gfx::point tile ) const override;

  e_map_side_edge is_on_map_side_edge(
      gfx::point tile ) const override;

  maybe<bool> is_sea_lane( gfx::point tile ) const override;

  maybe<bool> has_lcr( gfx::point tile ) const override;

  maybe<MovementPoints> movement_points_required(
      gfx::point src, e_direction direction ) const override;

  MovementPoints minimum_heuristic_tile_cost() const override;

 private:
  SSConst const& ss_;
  IVisibility const& viz_;
  // Note that this player is the player associated with the unit
  // that is moving, it does not determine the visibility, which
  // is instead determined by the viz_ member.
  e_player const player_type_  = {};
  e_unit_type const unit_type_ = {};
  bool const is_ship_          = {};
};

static_assert( !std::is_abstract_v<GotoMapViewer> );

} // namespace rn
