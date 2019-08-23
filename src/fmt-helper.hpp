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

// Revolution Now
#include "aliases.hpp"

// {fmt}
#include "fmt/format.h"

// base-util
#include "base-util/misc.hpp"
#include "base-util/pp.hpp"
#include "base-util/string.hpp"

// C++ standard library
#include <chrono>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

// The reason that we inherit from std::string is so that we can
// inherit its parser. Without the parser then we would not be
// able format custom types with non-trivial format strings.
using formatter_base = ::fmt::formatter<::std::string>;

/****************************************************************
** Macros
*****************************************************************/
// Macro to easily extend {fmt} to user-defined types. This macro
// should be issued in the global namespace.
#define DEFINE_FORMAT_IMPL( use_param, type, ... )     \
  template<>                                           \
  struct fmt::formatter<type> : formatter_base {       \
    template<typename FormatContext>                   \
    auto format( type const &o, FormatContext &ctx ) { \
      use_param return formatter_base::format(         \
          fmt::format( __VA_ARGS__ ), ctx );           \
    }                                                  \
  };

// When the type is templated. May need to be surrounded by EVAL.
#define DEFINE_FORMAT_T_IMPL( use_param, t_args, type, ... )  \
  template<PP_MAP_COMMAS( PP_ADD_TYPENAME, EXPAND t_args )>   \
  struct fmt::formatter<EXPAND type> : formatter_base {       \
    template<typename FormatContext>                          \
    auto format( EXPAND type const &o, FormatContext &ctx ) { \
      use_param return formatter_base::format(                \
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

/****************************************************************
** Type Wrappers
*****************************************************************/
namespace rn {

template<typename T>
struct FmtJsonStyleList {
  CRef<Vec<T>> vec;
};

// Deduction guide.
template<typename T>
FmtJsonStyleList( Vec<T> const & )->FmtJsonStyleList<T>;

} // namespace rn

/****************************************************************
** Formatters
*****************************************************************/
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

template<>
struct formatter<fs::path> : formatter_base {
  template<typename FormatContext>
  auto format( fs::path const &o, FormatContext &ctx ) {
    return formatter_base::format( fmt::format( o.string() ),
                                   ctx );
  }
};

// {fmt} formatter for vectors whose contained type is format-
// table, in a JSON-like notation: [3,4,8,3]. However note that
// it is not real json since e.g. strings will not have quotes
// around them.
template<typename T>
struct formatter<::rn::FmtJsonStyleList<T>> : formatter_base {
  template<typename FormatContext>
  auto format( ::rn::FmtJsonStyleList<T> const &o,
               FormatContext &                  ctx ) {
    std::vector<T> const &   vec = o.vec.get();
    std::vector<std::string> items;
    items.reserve( vec.size() );
    for( auto const &item : vec )
      items.push_back( fmt::format( "{}", item ) );
    return formatter_base::format(
        std::string( "[" ) + util::join( items, "," ) + "]",
        ctx );
  }
};

// {fmt} formatter for formatting optionals whose contained
// type is formattable.
template<typename T>
struct formatter<std::optional<T>> : formatter_base {
  template<typename FormatContext>
  auto format( std::optional<T> const &o, FormatContext &ctx ) {
    static const std::string nullopt_str( "nullopt" );
    return formatter_base::format(
        o.has_value() ? fmt::format( "{}", *o ) : nullopt_str,
        ctx );
  }
};

// {fmt} formatter for formatting pairs whose contained types are
// formattable.
template<typename T, typename U>
struct formatter<std::pair<T, U>> : formatter_base {
  template<typename FormatContext>
  auto format( std::pair<T, U> const &o, FormatContext &ctx ) {
    return formatter_base::format(
        fmt::format( "({},{})", o.first, o.second ), ctx );
  }
};

// FIXME: move this somewhere else and improve it.
template<class Rep, class Period>
auto to_string_colons(
    std::chrono::duration<Rep, Period> const &duration ) {
  using namespace std::chrono;
  using namespace std::literals::chrono_literals;
  std::string res; // should use small-string optimization.
  auto        d = duration;
  if( d > 1h ) {
    auto hrs = duration_cast<hours>( d );
    res += fmt::format( "{:0>2}", hrs.count() );
    res += ':';
    d -= hrs;
  }
  if( d > 1min ) {
    auto mins = duration_cast<minutes>( d );
    res += fmt::format( "{:0>2}", mins.count() );
    res += ':';
    d -= mins;
  }
  auto secs = duration_cast<seconds>( d );
  res += fmt::format( "{:0>2}", secs.count() );
  return res;
}

// {fmt} formatter for formatting duration.
template<class Rep, class Period>
struct formatter<std::chrono::duration<Rep, Period>>
  : formatter_base {
  template<typename FormatContext>
  auto format( std::chrono::duration<Rep, Period> const &o,
               FormatContext &                           ctx ) {
    return formatter_base::format(
        fmt::format( "{}", to_string_colons( o ) ), ctx );
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
