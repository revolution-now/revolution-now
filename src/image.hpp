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
#include "tx.hpp"

// Rds
#include "image.rds.hpp"

namespace rn {

Texture const& image( e_image which );

struct Plane;
Plane* image_plane();
void   image_plane_set( e_image image );

} // namespace rn
