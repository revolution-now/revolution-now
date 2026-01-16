/****************************************************************
**to.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-01-15.
*
* Description: Helpers to easily convert data structures to rcl.
*
*****************************************************************/
#pragma once

// rcl
#include "emit.hpp"

// cdr
#include "cdr/converter.hpp"

namespace rcl {

template<cdr::ToCanonical T>
std::string to_rcl( T const& o ) {
  static cdr::converter::options const options{
    .write_fields_with_default_value = true };
  return emit( cdr::run_conversion_to_canonical( o, options ) );
}

template<cdr::ToCanonical T>
std::string to_json( T const& o ) {
  static cdr::converter::options const options{
    .write_fields_with_default_value = true };
  return emit_json(
      cdr::run_conversion_to_canonical( o, options ) );
}

} // namespace rcl
