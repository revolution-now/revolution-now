/****************************************************************
**rcl.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-26.
*
* Description: Rcl helpers for Rds types.
*
*****************************************************************/
#pragma once

// Rcl
#include "rcl/ext.hpp"

// C++ standard library
#include <type_traits>

namespace rcl {

// Allows deserializing reflected enums from Rcl config files.
template<rn::ReflectedEnum Enum>
rcl::convert_err<Enum> convert_to( rcl::value const& v,
                                   rcl::tag<Enum> ) {
  base::maybe<std::string const&> s = v.get_if<std::string>();
  if( !s )
    return rcl::error(
        fmt::format( "cannot produce an {} enum value from type "
                     "{}. String required.",
                     rn::enum_traits<Enum>::type_name,
                     rcl::name_of( rcl::type_of( v ) ) ) );
  base::maybe<Enum> res =
      rn::enum_traits<Enum>::from_string( *s );
  if( !res.has_value() )
    return rcl::error( fmt::format(
        "failed to parse string `{}' into valid {} enum value.",
        *s, rn::enum_traits<Enum>::type_name ) );
  return *res;
}

} // namespace rcl
