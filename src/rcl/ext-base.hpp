/****************************************************************
**ext-base.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-26.
*
* Description: Rcl extensions for base library types.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "rcl/ext.hpp"

// base
#include "base/error.hpp"
#include "base/maybe.hpp"

namespace base {

// base::maybe<T>
template<rcl::Convertible T>
rcl::convert_err<maybe<T>> convert_to( rcl::value const& v,
                                       rcl::tag<maybe<T>> ) {
  if( v.holds<rcl::null_t>() ) return nothing;
  UNWRAP_RETURN( res, rcl::convert_to<T>( v ) );
  return std::move( res );
}

} // namespace base
