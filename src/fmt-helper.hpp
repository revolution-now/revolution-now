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

// base-util
#include "base-util/misc.hpp"
#include "base-util/string.hpp"

// C++ standard library
#include <chrono>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>

// The reason that we inherit from std::string is so that we can
// inherit its parser. Without the parser then we would not be
// able format custom types with non-trivial format strings.
using formatter_base = ::fmt::formatter<::std::string>;

// Macro to easily extend {fmt} to user-defined types. This macro
// should be issued in the global namespace.
#define DEFINE_FORMAT_IMPL( use_param, type, ... )     \
  template<>                                           \
  struct fmt::formatter<type> : formatter_base {       \
    template<typename FormatContext>                   \
    auto format( const type &o, FormatContext &ctx ) { \
      use_param return formatter_base::format(         \
          fmt::format( __VA_ARGS__ ), ctx );           \
    }                                                  \
  };

// This is the one to use when the formatting output depends on
// the value of the object (most cases).
#define DEFINE_FORMAT( type, ... ) \
  DEFINE_FORMAT_IMPL(, type, __VA_ARGS__ )
// This is for when the formatting output is independent of the
// value (i.e., only dependent on type); e.g., std::monostate.
#define DEFINE_FORMAT_( type, ... ) \
  DEFINE_FORMAT_IMPL( (void)o;, type, __VA_ARGS__ )

namespace fmt {

template<typename... Ts>
struct formatter<std::chrono::time_point<Ts...>>
  : formatter_base {
  template<typename FormatContext>
  auto format( std::chrono::time_point<Ts...> const &o,
               FormatContext &                       ctx ) {
    auto str = "\"" + util::to_string( o ) + "\"";
    return formatter_base::format( str, ctx );
  }
};

// {fmt} formatter for formatting variants whose constituent
// types are all so formattable.
template<typename... Ts>
struct formatter<std::variant<Ts...>> : dynamic_formatter<> {
  using V = std::variant<Ts...>;
  using B = dynamic_formatter<>;
  template<typename Context>
  auto format( V const &v, Context &ctx ) {
    return std::visit( LC( B::format( _, ctx ) ), v );
  }
};

// {fmt} formatter for formatting optionals whose contained
// type is formattable.
template<typename T>
struct formatter<std::optional<T>> : formatter_base {
  template<typename FormatContext>
  auto format( std::optional<T> const &o, FormatContext &ctx ) {
    return formatter_base::format(
        fmt::format( o.has_value() ? fmt::format( "{}", *o )
                                   : "nullopt" ),
        ctx );
  }
};

// This is a specialization (via SFINAE) for enums, though it
// will actually only work for the smart enums that can be
// converted to strings (and, in particular, the ones that come
// from the better-enums library and have the ::_enumerated and
// ::_to_string() members).
//
// The {fmt} library should find this automatically.
//
// The SFINAE is done just by requiring that the type have an
// _enumerated member, which in practice means that it is a
// better-enum type. However, it could also have been done like
// so, for example:
//
//   template<typename T>
//   struct formatter<
//     T,
//     char,
//     std::enable_if_t<
//       std::is_enum_v<
//         typename T::_enumerated
//       >
//     >
//   > {
//
// though that is probably overkill. In any case, note that the
// last type in the argument list must be the SFINAE and must
// have the type void (as it is if we use void_t or enable_if)
// because:
//
//   1) The third template parameter in the base template
//      declaration of formatter (in fmt/core.h) has a default
//      value of void, and
//   2) https://stackoverflow.com/questions/18700558/
//            default-template-parameter-partial-specialization
//
// Unfortunately, the fact that the below compiles and gets
// selected (or not selected) at the desired times may depend on
// implementation details of the fmt library, such as the precise
// template arguments in the base template declaration of
// formatter. So this could break at some point.
//
template<typename T>
struct formatter<T, char, std::void_t<typename T::_enumerated>>
  : formatter_base {
  template<typename FormatContext>
  auto format( T const &o, FormatContext &ctx ) {
    return formatter_base::format(
        fmt::format( "{}", o._to_string() ), ctx );
  }
};

} // namespace fmt

namespace rn {} // namespace rn
