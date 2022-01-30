/****************************************************************
**converter.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-29.
*
* Description: Helper for implementing from_canonical.
*
*****************************************************************/
#pragma once

// cdr
#include "error.hpp"
#include "ext.hpp"

namespace cdr {

/****************************************************************
** converter
*****************************************************************/
// This is a helper object that is used to implement the
// from_canonical functions. It is used to create error messages
// and to recursively convert nested fields. It is used so that,
// in the event of an error, it can accumulate a back trace to
// help pinpoint the location of the field that caused the error.
struct converter {
  consteval converter( std::string_view target_type_name )
    : target_type_name_( target_type_name ) {}

  template<typename... Args>
  error err( std::string_view fmt_str, Args&&... args ) const {
    auto res = error( fmt_str, std::forward<Args>( args )... );
    res.frames.push_back(
        error_frame{ .type_name = target_type_name_ } );
    return res;
  }

  // This is basically like the top-level `from_canonical`.
  template<FromCanonical T>
  result<std::remove_const_t<T>> from( value const& v ) {
    // The function called below should be found via ADL.
    auto res = from_canonical( v, tag<std::remove_const_t<T>> );
    if( !res.has_value() )
      res.error().frames.push_back(
          error_frame{ .type_name = target_type_name_ } );
    return res;
  }

  std::string_view const target_type_name_;
};

} // namespace cdr
