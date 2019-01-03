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

// C++ standard library
#include <variant>

// Macro to easily extend {fmt} to user-defined types.  This
// macro should be issued in the global namespace.
#define DEFINE_FORMAT_IMPL( use_param, type, ... )            \
  namespace fmt {                                             \
  template<>                                                  \
  struct formatter<type> {                                    \
    template<typename ParseContext>                           \
    constexpr auto parse( ParseContext &ctx ) {               \
      return ctx.begin();                                     \
    }                                                         \
    template<typename FormatContext>                          \
    auto format( const type &o, FormatContext &ctx ) {        \
      use_param return format_to( ctx.begin(), __VA_ARGS__ ); \
    }                                                         \
  };                                                          \
  }

// This is the one to use when the formatting output depends on
// the value of the object (most cases).
#define DEFINE_FORMAT( type, ... ) \
  DEFINE_FORMAT_IMPL(, type, __VA_ARGS__ )
// This is for when the formatting output is independent of the
// value (i.e., only dependent on type); e.g., std::monostate.
#define DEFINE_FORMAT_( type, ... ) \
  DEFINE_FORMAT_IMPL( (void)o;, type, __VA_ARGS__ )

namespace rn {

// This function exists for the purpose of  having  the  compiler
// deduce the Indexes variadic integer arguments that we can then
// use  to  index  the variant; it probably is not useful to call
// this method directly (it is called by to_string).
template<typename Variant, size_t... Indexes>
std::string variant_elems_to_string(
    Variant const &v,
    std::index_sequence<Indexes...> /*unused*/ ) {
  std::string res;
  // Unary right fold of template parameter pack.
  ( ( res += ( Indexes == v.index() )
                 ? ::fmt::format( "{}", std::get<Indexes>( v ) )
                 : "" ),
    ... );
  return res;
}

template<typename... Args>
std::string variant_to_string( std::variant<Args...> const &v ) {
  auto is = std::make_index_sequence<sizeof...( Args )>();
  return variant_elems_to_string( v, is );
}

} // namespace rn

namespace fmt {

// {fmt} formatter for formatting variants whose constituent
// types are all so formattable.
template<typename... Ts>
struct formatter<std::variant<Ts...>> {
  template<typename ParseContext>
  constexpr auto parse( ParseContext &ctx ) {
    return ctx.begin();
  }
  template<typename FormatContext>
  auto format( std::variant<Ts...> const &o,
               FormatContext &            ctx ) {
    return format_to( ctx.begin(),
                      ::rn::variant_to_string( o ) );
  }
};

} // namespace fmt

namespace rn {} // namespace rn
