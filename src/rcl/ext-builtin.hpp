/****************************************************************
**ext-builtin.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-25.
*
* Description: Rcl extensions for builtin types.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "rcl/ext.hpp"

namespace rcl {

convert_err<int>    convert_to( value const& v, tag<int> );
convert_err<bool>   convert_to( value const& v, tag<bool> );
convert_err<double> convert_to( value const& v, tag<double> );

} // namespace rcl
