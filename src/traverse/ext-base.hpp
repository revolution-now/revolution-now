/****************************************************************
**ext-base.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-03.
*
* Description: Traversable implementation for base library types.
*
*****************************************************************/
#pragma once

// traverse
#include "ext.hpp"

// base
#include "base/heap-value.hpp"
#include "base/maybe.hpp"
#include "base/variant.hpp"

namespace trv {

/****************************************************************
** base::maybe
*****************************************************************/
template<typename T, typename Fn>
void traverse( base::maybe<T> const& o, Fn& fn,
               tag_t<base::maybe<T>> ) {
  if( o.has_value() ) fn( *o, none );
}

/****************************************************************
** base::heap_value
*****************************************************************/
template<typename T, typename Fn>
void traverse( base::heap_value<T> const& o, Fn& fn,
               tag_t<base::heap_value<T>> ) {
  fn( *o, none );
}

/****************************************************************
** base::variant
*****************************************************************/
// Hold off on handling a simple variant for which no reflection
// information is known, since then we would not be able to
// supply anything meaningful for the key. This is disabled for
// cdr as well for similar reasons. Maybe we'll come back to this
// in the future. There will be a more constrained overload in
// the reflection library that will provide these overloads in
// the case that the variant alternatives all consist of unique
// reflected structs, and hopefully that one will be selected in
// the relevant cases since it is more constrained.
template<typename... Ts, typename Fn>
void traverse( base::variant<Ts...> const& o, Fn& fn,
               tag_t<base::variant<Ts...>> ) = delete;

/****************************************************************
** Structs derived from base::variant.
*****************************************************************/
template<typename T, typename Fn>
requires requires { typename T::i_am_rds_variant; }
void traverse( T const& o, Fn& fn, tag_t<T> ) {
  fn( o.as_base(), none );
}

} // namespace trv