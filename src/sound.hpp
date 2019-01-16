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

// Revolution Now
#include "enum.hpp"

namespace rn {

ND bool play_music_file( char const* file );

void cleanup_sound();

enum class e_( sfx, move, attacker_lost, attacker_won );

void load_all_sfx();

void play_sound_effect( e_sfx sound );

} // namespace rn
