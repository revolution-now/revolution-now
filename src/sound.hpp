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

namespace rn {

enum class e_sfx { move, attacker_lost, attacker_won };

void play_sound_effect( e_sfx sound );

void linker_dont_discard_module_sound();

} // namespace rn
