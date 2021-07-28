/****************************************************************
**sound.hpp
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

// Rds
#include "rds/sound.hpp"

namespace rn {

void play_sound_effect( e_sfx sound );

void linker_dont_discard_module_sound();

} // namespace rn
