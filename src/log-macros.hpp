/****************************************************************
**log-macros.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-27.
*
* Description: Macros that are useful for logging.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "logger.hpp"

// C++ standard library
#include <type_traits>

/****************************************************************
** Macros
*****************************************************************/
// Will log a variable whenever it changes. This involves
// copying, so should probably only be used while debugging.
#define LOG_WHEN_CHANGE( level, var )                    \
  {                                                      \
    static std::remove_cv_t<decltype( var )> __to_log{}; \
    if( __to_log != var ) {                              \
      lg.level( "{} changed: {}", #var, var );           \
      __to_log = var;                                    \
    }                                                    \
  }

// Will call a callable each time the variable changes. This
// involves copying, so should probably only be used while
// debugging.
#define WHEN_CHANGE_DO( var, ... )                       \
  {                                                      \
    static std::remove_cv_t<decltype( var )> __cached{}; \
    if( __cached != var ) {                              \
      __VA_ARGS__();                                     \
      __cached = var;                                    \
    }                                                    \
  }

// Logs the variable "v: <value of v>".
#define LOG_VAR( level, v ) lg.level( #v ": {}", v )
