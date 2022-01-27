/****************************************************************
**ext-builtin.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-27.
*
* Description: Cdr conversions for builtin types.
*
*****************************************************************/
#pragma once

// cdr
#include "ext.hpp"

namespace cdr {

/****************************************************************
** int
*****************************************************************/
value       to_canonical( int o, tag_t<int> );
result<int> from_canonical( value const& v, tag_t<int> );

/****************************************************************
** bool
*****************************************************************/
value        to_canonical( bool o, tag_t<bool> );
result<bool> from_canonical( value const& v, tag_t<bool> );

/****************************************************************
** double
*****************************************************************/
value          to_canonical( double o, tag_t<double> );
result<double> from_canonical( value const& v, tag_t<double> );

} // namespace cdr
