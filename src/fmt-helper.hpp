/****************************************************************
**fmt-helper.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-26.
*
* Description: Some helper utilities for using {fmt}.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// {fmt}
#include "fmt/format.h"
#include "fmt/ostream.h"

// Macro to easily extend {fmt} to user-defined types.  This
// macro should be issued in the global namespace.
#define DEFINE_FORMAT( type, ... )                     \
  namespace fmt {                                      \
  template<>                                           \
  struct formatter<type> {                             \
    template<typename ParseContext>                    \
    constexpr auto parse( ParseContext &ctx ) {        \
      return ctx.begin();                              \
    }                                                  \
    template<typename FormatContext>                   \
    auto format( const type &o, FormatContext &ctx ) { \
      return format_to( ctx.begin(), __VA_ARGS__ );    \
    }                                                  \
  };                                                   \
  }

namespace rn {} // namespace rn
