/****************************************************************
**camera.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-19.
*
* Description: Controls the camera on the land view.
*
*****************************************************************/
#pragma once

// rds
#include "camera.rds.hpp"

// gfx
#include "gfx/cartesian.hpp"

namespace rn {

// TODO: The plan is to slowly migrate and rewrite the stuff in
// the viewport module to this one, bringing testing and sanity.

// NOTE: until it is fully migrated, we need to remember to call
// the ViewportController's fix_invariants method after calling
// any methods on this class.

/****************************************************************
** Fwd Decls.
*****************************************************************/
struct IUserConfig;
struct Viewport; // TODO: rename.

/****************************************************************
** Camera.
*****************************************************************/
struct Camera {
  Camera( IUserConfig const& user_config, Viewport& camera,
          gfx::size map_size_tiles );

 public: // zoom control.
  ZoomChanged zoom_out();
  ZoomChanged zoom_in();

  void center_on_tile( gfx::point tile );

  // For convenience.
  gfx::size map_dimensions_tiles() const;

 private:
  static auto const& static_config();
  auto const& user_config();

  void fix_broken_invariants();

  gfx::size const map_size_tiles_;
  IUserConfig const& user_config_;
  Viewport& camera_;
};

} // namespace rn
