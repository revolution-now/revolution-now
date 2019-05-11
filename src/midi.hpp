/****************************************************************
**midi.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-04-20.
*
* Description: Interface for playing midi files.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// C++ standard library.
#include <string>

namespace rn {

// Testing.
void test_midi();
void play_midi_file( std::string const& file );

} // namespace rn
