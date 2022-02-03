/****************************************************************
**ext-base.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-02.
*
* Description: Cdr conversions for std types.
*
*****************************************************************/
#pragma once

// cdr
#include "converter.hpp"
#include "ext.hpp"

// base
#include "base/maybe.hpp"

namespace cdr {

/****************************************************************
** base::maybe
*****************************************************************/
template<ToCanonical T>
value to_canonical( converter& conv, base::maybe<T> const& o,
                    tag_t<base::maybe<T>> ) {
  if( !o.has_value() ) return null;
  return conv.to( *o );
}

template<FromCanonical T>
result<base::maybe<T>> from_canonical( converter&   conv,
                                       value const& v,
                                       tag_t<base::maybe<T>> ) {
  if( v == null ) return base::nothing;
  UNWRAP_RETURN( res, conv.from<T>( v ) );
  return base::maybe<T>{ std::move( res ) };
}

} // namespace cdr
