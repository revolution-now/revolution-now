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
#include "base/heap-value.hpp"
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

/****************************************************************
** base::heap_value
*****************************************************************/
template<ToCanonical T>
value to_canonical( converter&                 conv,
                    base::heap_value<T> const& o,
                    tag_t<base::heap_value<T>> ) {
  return conv.to( *o );
}

template<FromCanonical T>
result<base::heap_value<T>> from_canonical(
    converter& conv, value const& v,
    tag_t<base::heap_value<T>> ) {
  UNWRAP_RETURN( res, conv.from<T>( v ) );
  return base::heap_value<T>( std::move( res ) );
}

/****************************************************************
** base::variant
*****************************************************************/
// We don't want to attempt to convert a simple variant for which
// no other reflection information is known, since then we would
// be basically forced (if we want efficiency) to use an approach
// consisting of recording the active alternative index, which is
// not very robust serialization-wise or readability-wise. There
// will be a more constrained overload in the reflection library
// that will provide these overloads in the case that the variant
// alternatives all consist of unique reflected structs, and
// hopefully that one will be selected in the relevant cases
// since it is more constrained.
template<typename... Ts>
value to_canonical( converter&                  conv,
                    base::variant<Ts...> const& o,
                    tag_t<base::variant<Ts...>> ) = delete;

template<typename... Ts>
result<base::variant<Ts...>> from_canonical(
    converter& conv, value const& v,
    tag_t<base::variant<Ts...>> ) = delete;

} // namespace cdr