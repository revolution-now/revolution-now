/****************************************************************
* sound.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-29.
*
* Description: All things sound.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

namespace rn {

ND bool play_music_file( char const* file );

void cleanup_sound();

} // namespace rn

