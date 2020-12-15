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
#include "maybe.hpp"

// base
#include "base/source-loc.hpp"

// base-util
#include "base-util/mp.hpp"
#include "base-util/pp.hpp"
#include "base-util/string.hpp"

// {fmt}
#include "fmt/format.h"

// magic_enum
#include "magic_enum.hpp"

// C++ standard library
#include <chrono>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

// The reason that we inherit from std::string is so that we can
// inherit its parser. Without the parser then we would not be
// able format custom types with non-trivial format strings.
using formatter_base = ::fmt::formatter<::std::string>;

/****************************************************************
** Metaprogramming
*****************************************************************/
namespace rn {

template<typename T>
constexpr bool has_fmt =
    ::fmt::has_formatter<T, ::fmt::format_context>();

}

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

// Tag assembler.  Example:
//
//   auto tag = FmtTags<
//     MyTag1,
//     MyTag2
//   >{};
//
//   fmt::format( "{}", tag( var ) );
//
// MyTag2 will be applied first to var, then MyTag1 will be ap-
// plied to the result.
//
template<template<typename U> typename... Tags>
struct FmtTags;

template<template<typename U> typename Tag>
struct FmtTags<Tag> {
  template<typename T>
  auto operator()( T &&o ) {
    return Tag{ std::forward<T>( o ) };
  }
};

template<template<typename U> typename FirstTag,
         template<typename Z> typename... RestTags>
struct FmtTags<FirstTag, RestTags...> {
  template<typename T>
  auto operator()( T &&o ) const {
    return FirstTag{
        FmtTags<RestTags...>{}( std::forward<T>( o ) ) };
  }
};

// In the struct below we capture the argument by value if it is
// a temporary and then point to it with the ref. If not a tempo-
// rary we just point to it with the ref. In either case, the ref
// will always refer to the underlying value.
#define DEFINE_FMT_TAG( name )                              \
  template<typename T>                                      \
  struct name {                                             \
    name( T const &o ) : maybe_o{}, ref( o ) {}             \
    name( T &&o )                                           \
      : maybe_o( std::forward<T>( o ) ), ref( *maybe_o ) {} \
    maybe<T>                        maybe_o;                \
    std::reference_wrapper<T const> ref;                    \
  };                                                        \
  template<typename T>                                      \
  name( T const & ) -> name<T>;

DEFINE_FMT_TAG( FmtRemoveTemplateArgs );
DEFINE_FMT_TAG( FmtRemoveRnNamespace );

template<typename T>
struct FmtJsonStyleList {
  std::reference_wrapper<Vec<T> const> vec;
};

// Deduction guide.
template<typename T>
FmtJsonStyleList( Vec<T> const & ) -> FmtJsonStyleList<T>;

} // namespace rn

/****************************************************************
** Formatters for Type Wrappers
*****************************************************************/
namespace fmt {

// "some_type<x, y, z<a, b, c>>" --> "some_type<...>"
template<typename T>
struct formatter<::rn::FmtRemoveTemplateArgs<T>>
  : formatter_base {
  template<typename FormatContext>
  auto format( ::rn::FmtRemoveTemplateArgs<T> const &o,
               FormatContext &                       ctx ) {
    std::string inner = fmt::format( "{}", o.ref.get() );
    std::string reduced;
    reduced.reserve( inner.size() );
    int angle_bracket_level = 0;
    for( int i = 0; i < int( inner.size() ); ++i ) {
      if( inner[i] == '<' ) {
        if( angle_bracket_level == 0 ) reduced += "<...>";
        angle_bracket_level++;
        continue;
      }
      if( inner[i] == '>' ) {
        angle_bracket_level--;
        continue;
      }
      if( angle_bracket_level == 0 )
        reduced.push_back( inner[i] );
    }
    return formatter_base::format( reduced, ctx );
  }
};

// "rn::xyz" --> "xyz"
// "rn::(anonymous namespace)::xyz" --> "xyz"
template<typename T>
struct formatter<::rn::FmtRemoveRnNamespace<T>>
  : formatter_base {
  template<typename FormatContext>
  auto format( ::rn::FmtRemoveRnNamespace<T> const &o,
               FormatContext &                      ctx ) {
    std::string with_namespaces =
        fmt::format( "{}", o.ref.get() );
    std::string_view sv = with_namespaces;
    if( util::starts_with( sv, "::" ) ) sv.remove_prefix( 2 );
    if( util::starts_with( sv, "rn::" ) ) sv.remove_prefix( 4 );
    if( util::starts_with( sv, "(anonymous namespace)::" ) )
      sv.remove_prefix( 23 );
    return formatter_base::format( std::string( sv ), ctx );
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

} // namespace fmt

/****************************************************************
** Formatters
*****************************************************************/
namespace fmt {

template<>
struct formatter<std::monostate> : formatter_base {
  template<typename FormatContext>
  auto format( std::monostate const &, FormatContext &ctx ) {
    return formatter_base::format( "monostate", ctx );
  }
};

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

// {fmt} formatter for formatting variant-like things whose
// constituent types are also formattable.
template<template<typename...> typename V, typename... Ts>
/* clang-format off */
    requires( std::is_convertible_v<V<Ts...>&,
                std::variant<Ts...>&> &&
              mp::and_v<::rn::has_fmt<Ts>...> )
struct formatter<V<Ts...>>
  /* clang-format on */
  : dynamic_formatter<> {
  using B = dynamic_formatter<>;
  template<typename Context>
  auto format( V<Ts...> const &v, Context &ctx ) {
    return std::visit(
        [&]( auto const &_ ) {
          return B::format( fmt::format( "{}", _ ), ctx );
        },
        static_cast<std::variant<Ts...> const &>( v ) );
  }
};

template<>
struct formatter<base::SourceLoc> : formatter_base {
  template<typename FormatContext>
  auto format( base::SourceLoc const &o, FormatContext &ctx ) {
    return formatter_base::format(
        fmt::format( "{}:{}:{}", o.file_name(), o.line(),
                     o.column() ),
        ctx );
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

// {fmt} formatter for formatting maybes whose contained
// type is formattable.
template<typename T>
struct formatter<base::maybe<T>> : formatter_base {
  template<typename FormatContext>
  auto format( base::maybe<T> const &o, FormatContext &ctx ) {
    static const std::string nothing_str( "nothing" );
    return formatter_base::format(
        o.has_value() ? fmt::format( "{}", *o ) : nothing_str,
        ctx );
  }
};

// {fmt} formatter for formatting nothing_t.
template<>
struct formatter<base::nothing_t> : formatter_base {
  template<typename FormatContext>
  auto format( base::nothing_t const &, FormatContext &ctx ) {
    static const std::string nothing_str( "nothing" );
    return formatter_base::format( nothing_str, ctx );
  }
};

// {fmt} formatter for formatting vectors whose contained
// type is formattable.
template<typename T>
struct formatter<std::vector<T>> : formatter_base {
  template<typename FormatContext>
  auto format( std::vector<T> const &o, FormatContext &ctx ) {
    return formatter_base::format(
        fmt::format( "{}", ::rn::FmtJsonStyleList{ o } ), ctx );
  }
};

// {fmt} formatter for formatting reference wrappers whose
// referenced type is formattable.
template<typename T>
struct formatter<std::reference_wrapper<T>> : formatter_base {
  template<typename FormatContext>
  auto format( std::reference_wrapper<T> const &o,
               FormatContext &                  ctx ) {
    return formatter_base::format( fmt::format( "{}", o.get() ),
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

  bool has_hour_or_minute = false;
  if( d > 1h ) {
    auto hrs = duration_cast<hours>( d );
    res += fmt::format( "{:0>2}", hrs.count() );
    res += ':';
    d -= hrs;
    has_hour_or_minute = true;
  }
  if( d > 1min ) {
    auto mins = duration_cast<minutes>( d );
    res += fmt::format( "{:0>2}", mins.count() );
    res += ':';
    d -= mins;
    has_hour_or_minute = true;
  }
  auto secs = duration_cast<seconds>( d );
  res += fmt::format( "{:0>2}", secs.count() );
  if( !has_hour_or_minute ) res += 's';
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

// Specialization for standard enums using magic_enum.
template<typename T>
struct formatter<T, char, std::enable_if_t<std::is_enum_v<T>>>
  : formatter_base {
  template<typename FormatContext>
  auto format( T const &o, FormatContext &ctx ) {
    return formatter_base::format(
        fmt::format( "{}", magic_enum::enum_name( o ) ), ctx );
  }
};

} // namespace fmt
