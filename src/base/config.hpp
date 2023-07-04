/****************************************************************
**config.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-15.
*
* Description: Basic config for the base library.
*
*****************************************************************/
#pragma once

#include <cstddef>

using size_t = std::size_t;

#if defined( __clang__ )
#  define COMPILER_CLANG
#elif defined( __GNUC__ ) || defined( __GNUG__ )
#  define COMPILER_GCC
#elif defined( _MSC_VER )
#  define COMPILER_MSVC
#endif