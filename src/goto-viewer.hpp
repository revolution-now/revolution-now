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
struct GotoMapViewer : IGotoMapViewer {
  GotoMapViewer( SSConst const& ss, Unit const& unit );
  ~GotoMapViewer() override;

  bool can_enter_tile( gfx::point p ) const override;

 private:
  std::unique_ptr<IVisibility const> const viz_;
  bool const is_ship_ = {};
};

} // namespace rn
