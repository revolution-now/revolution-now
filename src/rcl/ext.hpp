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
#include "base/valid.hpp"

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

#define RCL_CHECK( a, ... )                         \
  {                                                 \
    if( !( a ) ) {                                  \
      auto loc = ::base::SourceLoc::current();      \
      return rcl::error( fmt::format(               \
          "{}:{}: {}", loc.file_name(), loc.line(), \
          FMT_SAFE( "" __VA_ARGS__ ) ) );           \
    }                                               \
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

using convert_valid = base::valid_or<error>;

template<typename T>
concept Convertible = requires( T const& o, value const& v ) {
  { convert_to( v, tag<T>{} ) } -> std::same_as<convert_err<T>>;
};

// An rcl-convertible type can optionally provide a validation
// method which, if detected, will be called after conversion to
// validate the object. Such a method must be called
// `rcl_validate` and it can be either a const member function OR
// a free-standing (or friend) function (the latter allows pro-
// viding validation functions for e.g. std types).
template<typename T>
concept ValidatableViaMember = Convertible<T> &&
    requires( T const& o ) {
  { o.rcl_validate() } -> std::same_as<convert_valid>;
};

template<typename T>
concept ValidatableViaFreeMethod = Convertible<T> &&
    requires( T const& o ) {
  { rcl_validate( o ) } -> std::same_as<convert_valid>;
};

template<Convertible T>
convert_err<T> convert_to( value const& v ) {
  // The function called below should be found via ADL.
  convert_err<T> res = convert_to( v, tag<T>{} );
  if( !res.has_value() ) return res;
  static_assert(
      !(ValidatableViaMember<T> && ValidatableViaFreeMethod<T>));
  if constexpr( ValidatableViaMember<T> ) {
    HAS_VALUE_OR_RET( res->rcl_validate() );
  } else if constexpr( ValidatableViaFreeMethod<T> ) {
    // The function called below should be found via ADL.
    HAS_VALUE_OR_RET( rcl_validate( *res ) );
  }
  return res;
}

} // namespace rcl

TOSTR_TO_FMT( rcl::error );
