/****************************************************************
**image.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-16.
*
* Description: Handles loading and displaying fullscreen images.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "enum.hpp"
#include "plane.hpp"

namespace rn {

enum class e_( image, old_world );

// This will cause all images to be loaded into memory but the
// resulting textures will not be owned by this module, so there
// is no need for a corresponding `release` function.
void load_all_images();

Plane* image_plane();
void   image_plane_enable( bool enable );
void   image_plane_set( e_image image );

} // namespace rn
