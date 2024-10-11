/****************************************************************
**to-str.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-01-07.
*
* Description: to_str framework.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "adl-tag.hpp"
#include "fmt.hpp"

// C++ standard library
#include <concepts>
#include <string>
#include <string_view>

namespace std {
void to_str( string const& o, string& out, base::tag<string> );
}

namespace base {

/****************************************************************
** Primitive Types.
*****************************************************************/
void to_str( bool o, std::string& out, tag<bool> );
void to_str( char o, std::string& out, tag<char> );
void to_str( int8_t o, std::string& out, tag<int8_t> );
void to_str( uint8_t o, std::string& out, tag<uint8_t> );
void to_str( int o, std::string& out, tag<int> );
void to_str( int16_t o, std::string& out, tag<int16_t> );
void to_str( uint16_t o, std::string& out, tag<uint16_t> );
void to_str( uint32_t o, std::string& out, tag<uint32_t> );
void to_str( size_t o, std::string& out, tag<size_t> );
void to_str( long o, std::string& out, tag<long> );
void to_str( float o, std::string& out, tag<float> );
void to_str( double o, std::string& out, tag<double> );

template<size_t N> void to_str( char const ( &o )[N],
                                std::string& out,
                                tag<char[N]> ) {
  for( char const c : o ) {
    if( !c ) break;
    to_str( c, out, tag<char>{} );
  }
}

/****************************************************************
** Concept.
*****************************************************************/
template<typename T>
concept Show = requires( T const& o, std::string s ) {
  {
    to_str( o, s, ::base::tag<std::remove_cvref_t<T>>{} )
  } -> std::same_as<void>;
};

/****************************************************************
** API methods.
*****************************************************************/
// This version is faster since it reuses an existing string ob-
// ject and appends to it. This one should be used when imple-
// menting to_str methods in terms of other to_str methods.
template<Show T>
void to_str( T const& o, std::string& out ) {
  to_str( o, out, tag<T>{} );
}

// Only use this one when you know that you're only converting a
// single value to a string. Otherwise, prefer the to_str variant
// with an output argument because it reuses the same string ob-
// ject for efficiency.
template<Show T>
std::string to_str( T const& o ) {
  std::string res;
  to_str( o, res );
  return res;
}

} // namespace base

namespace fmt {

namespace detail {

// Need to exclude these because they would conflict with the
// ones in fmt.
template<typename T>
concept ExcludeFromFmtPromotion = requires {
  // clang-format off
  requires(
      std::is_same_v<T, std::string>             ||
     (std::is_scalar_v<T> && !std::is_enum_v<T>) ||
      std::is_array_v<T>                         ||
      std::is_same_v<T, int>                     ||
      std::is_same_v<T, char*>                   ||
      std::is_same_v<T, char const*>             ||
      std::is_same_v<T, std::string_view>
  );
  // clang-format on
};

} // namespace detail

// This enables formatting with fmt anything that has a to_str.
//
// The reason that we inherit from the std::string formatter is
// so that we can reuse its parser. Without the parser then we
// would not be able format custom types with non-trivial format
// strings.
template<base::Show S>
// clang-format off
requires( !detail::ExcludeFromFmtPromotion<std::remove_cvref_t<S>> )
struct formatter<S> : formatter<std::string> {
  // clang-format on
  template<typename FormatContext>
  auto format( S const& o, FormatContext& ctx ) const {
    return formatter<std::string>::format( base::to_str( o ),
                                           ctx );
  }
};

} // namespace fmt