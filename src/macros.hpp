/****************************************************************
**macros.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: Preprocessor macros.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#include <iostream>

// This is obviously a no-op but is an attempt to suppress some
// compiler warnings about parenthesis around macro parameters
// in inconsistent ways by different compilers.
#define ID_( a ) a

// Use like this:
//
//   WARNING() << "some warning message " << xyz;
//
// It will append a newline for you.
#define WARNING() \
  std::cerr << "WARNING:" << __FILE__ << ":" << __LINE__ << ": "

#define LOG( a )                                             \
  std::cerr << "LOG:" << __FILE__ << ":" << __LINE__ << ": " \
            << a /* NOLINT(bugprone-macro-parentheses) */    \
            << "\n"
