/****************************************************************
**europort-view.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-06-14.
*
* Description: Implements the Europe port view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Flatbuffers
#include "sg-macros.hpp"

namespace rn {

DECLARE_SAVEGAME_SERIALIZERS( EuroportView );

struct Plane;
Plane* europe_plane();

} // namespace rn
