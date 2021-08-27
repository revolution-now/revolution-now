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
** Extension point.
*****************************************************************/
namespace rcl {

template<typename T>
struct tag {};

struct error {
  explicit error( std::string_view sv )
    : what( std::string( sv ) ) {}
  auto operator<=>( error const& ) const = default;

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
