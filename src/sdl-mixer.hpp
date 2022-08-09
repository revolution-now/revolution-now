/****************************************************************
**sdl-mixer.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-09.
*
* Description: Single point where SDL_mixer.h is included.
*
*****************************************************************/
#pragma once

#ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Weverything"
#endif
#include "SDL_mixer.h"
#ifdef __clang__
#  pragma clang diagnostic pop
#endif
