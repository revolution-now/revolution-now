/****************************************************************
**traverse.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-04.
*
* Description: Traversable extension for reflected types.
*
*****************************************************************/
#pragma once

// refl
#include "ext.hpp"

// traverse
#include "traverse/ext.hpp"

// base
#include "base/meta.hpp"

// Normally the traverse implementations doesn't go into the trv
// namespace (they go alongside the type being traversed in the
// same namespace) because they are found via ADL. However, since
// these are general overloads that must apply to any reflected
// type regardless of namespace, we obviously cannot follow that
// usual procedure. So we have to put them in the trv namespace
// so that they are found by ADL via the trv::tag_t parameter.
namespace trv {

/****************************************************************
** Enums
*****************************************************************/
// Handled by virtue of being scalar types.

/****************************************************************
** Structs
*****************************************************************/
template<refl::ReflectedStruct S, typename Fn>
void traverse( S const& o, Fn& fn, trv::tag_t<S> ) {
  using Tr = ::refl::traits<S>;
  static constexpr size_t kNumFields =
      std::tuple_size_v<decltype( Tr::fields )>;
  FOR_CONSTEXPR_IDX( Idx, kNumFields ) {
    auto& field_desc = std::get<Idx>( Tr::fields );
    auto& field_val  = o.*field_desc.accessor;
    fn( field_val, std::string_view{ field_desc.name } );
  };
}

/****************************************************************
** Wrappers
*****************************************************************/
// For types that satisfy WrapsReflected.
template<refl::WrapsReflected T, typename Fn>
void traverse( T const& o, Fn& fn, trv::tag_t<T> ) {
  fn( o.refl(), trv::none );
}

/****************************************************************
** Reflected Variants
*****************************************************************/
// Reflected variants are variants whose member alternatives are
// all reflected structs.
template<refl::ReflectedStruct... Ts, typename Fn>
void traverse( base::variant<Ts...> const& o, Fn& fn,
               trv::tag_t<base::variant<Ts...>> ) {
  auto const visitor = [&]<typename T>( T const& alt ) {
    using Tr = ::refl::traits<T>;
    static constexpr std::string_view kName{ Tr::name };
    fn( alt, kName );
  };
  return std::visit( visitor, o );
}

} // namespace trv
