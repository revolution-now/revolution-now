/****************************************************************
**attributes.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-08.
*
* Description: Macros for compiler-specific attributes.
*
*****************************************************************/
#pragma once

#if defined( __clang__ )
#  define ATTR_LIFETIMEBOUND [[clang::lifetimebound]]
#else
#  define ATTR_LIFETIMEBOUND
#endif