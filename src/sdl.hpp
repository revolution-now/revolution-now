/****************************************************************
**sdl.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-09.
*
* Description: Single point where SDL.h is included.
*
*****************************************************************/
#pragma once

#ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Weverything"
#endif
#include "SDL.h"
#ifdef __clang__
#  pragma clang diagnostic pop
#endif