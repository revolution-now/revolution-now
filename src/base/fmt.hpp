/****************************************************************
**fmt.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-09.
*
* Description: Some helper utilities for using {fmt}.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base-util
#include "base-util/pp.hpp"

// {fmt}
#include "fmt/format.h"

// C++ standard library
#include <string>

namespace base {

// The reason that we inherit from std::string is so that we can
// inherit its parser. Without the parser then we would not be
// able format custom types with non-trivial format strings.
using formatter_base = ::fmt::formatter<::std::string>;

} // namespace base

/****************************************************************
** Metaprogramming
*****************************************************************/
namespace base {

// This partially works I think...
template<typename T>
constexpr bool has_fmt =
    ::fmt::has_formatter<T, ::fmt::format_context>();

} // namespace base

/****************************************************************
** Macros
*****************************************************************/
// Macro to generate an fmt formatter for a type that has a
// to_str overload.
#define TOSTR_TO_FMT( type )                           \
  template<>                                           \
  struct fmt::formatter<type> : base::formatter_base { \
    template<typename FormatContext>                   \
    auto format( type const& o, FormatContext& ctx ) { \
      std::string res;                                 \
      to_str( o, res );                                \
      return base::formatter_base::format(             \
          fmt::format( "{}", res ), ctx );             \
    }                                                  \
  };

// Macro to easily extend {fmt} to user-defined types. This macro
// should be issued in the global namespace.
#define DEFINE_FORMAT_IMPL( use_param, type, ... )     \
  template<>                                           \
  struct fmt::formatter<type> : base::formatter_base { \
    template<typename FormatContext>                   \
    auto format( type const& o, FormatContext& ctx ) { \
      use_param return base::formatter_base::format(   \
          fmt::format( __VA_ARGS__ ), ctx );           \
    }                                                  \
  };

// When the type is templated. May need to be surrounded by EVAL.
#define DEFINE_FORMAT_T_IMPL( use_param, t_args, type, ... )  \
  template<PP_MAP_COMMAS( PP_ADD_TYPENAME, EXPAND t_args )>   \
  struct fmt::formatter<EXPAND type> : base::formatter_base { \
    template<typename FormatContext>                          \
    auto format( EXPAND type const& o, FormatContext& ctx ) { \
      use_param return base::formatter_base::format(          \
          fmt::format( __VA_ARGS__ ), ctx );                  \
    }                                                         \
  };

// This is the one to use when the formatting output depends on
// the value of the object (most cases).
#define DEFINE_FORMAT( type, ... ) \
  DEFINE_FORMAT_IMPL(, type, __VA_ARGS__ )
// This is for when the formatting output is independent of the
// value (i.e., only dependent on type); e.g., std::monostate.
#define DEFINE_FORMAT_( type, ... ) \
  DEFINE_FORMAT_IMPL( (void)o;, type, __VA_ARGS__ )
// For when the type is templated. Important: the `type` argument
// must be surrounded in parenthesis.
#define DEFINE_FORMAT_T( t_args, type, ... ) \
  DEFINE_FORMAT_T_IMPL(, t_args, type, __VA_ARGS__ )
#define DEFINE_FORMAT_T_( t_args, type, ... ) \
  DEFINE_FORMAT_T_IMPL( (void)o;, t_args, type, __VA_ARGS__ )

namespace base {} // namespace base
