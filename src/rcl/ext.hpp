/****************************************************************
**ext.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-12.
*
* Description: User type extension point for Rcl.
*
*****************************************************************/
#pragma once

// rcl
#include "model.hpp"

// base
#include "base/expect.hpp"
#include "base/fmt.hpp"

// C++ standard library
#include <string>
#include <string_view>

/****************************************************************
** Macros
*****************************************************************/
#define RCL_CONVERT_FIELD( name )                               \
  {                                                             \
    if( !tbl.has_key( TO_STRING( name ) ) )                     \
      return rcl::error( fmt::format(                           \
          "table must have a '{}' field for conversion to {}.", \
          TO_STRING( name ), kTypeName ) );                     \
    {                                                           \
      UNWRAP_RETURN( o, rcl::convert_to<decltype( res.name )>(  \
                            tbl[TO_STRING( name )] ) );         \
      res.name = std::move( o );                                \
    }                                                           \
  }

/****************************************************************
** Extension point.
*****************************************************************/
namespace rcl {

template<typename T>
struct tag {};

struct error {
  explicit error( std::string_view sv )
    : what( std::string( sv ) ) {}

  template<typename... Args>
  error( std::string_view fmt_str, Args&&... args )
    : error( fmt::format( fmt_str,
                          std::forward<Args>( args )... ) ) {}

  bool operator==( error const& ) const = default;

  friend void to_str( error const& o, std::string& out ) {
    out += o.what;
  }

  std::string what;
};

template<typename T>
using convert_err = base::expect<T, error>;

template<typename T>
concept Convertible = requires( T const& o, value const& v ) {
  { convert_to( v, tag<T>{} ) } -> std::same_as<convert_err<T>>;
};

template<Convertible T>
convert_err<T> convert_to( value const& v ) {
  // The function called below should be found via ADL.
  return convert_to( v, tag<T>{} );
}

} // namespace rcl

TOSTR_TO_FMT( rcl::error );
