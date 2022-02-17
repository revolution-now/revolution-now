/****************************************************************
**cdr.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-03.
*
* Description: Cdr extension for reflected types.
*
*****************************************************************/
#pragma once

// refl
#include "ext.hpp"

// cdr
#include "cdr/converter.hpp"
#include "cdr/ext.hpp"

// base
#include "base/meta.hpp"

// C++ standard library
#include <string>
#include <unordered_set>

namespace refl {

namespace detail {

// This concept checks if the method exists with any signature at
// all, then the ValidatableStruct concept will check if it has
// the right signature. That allows us to catch validate methods
// that have the wrong signature, which would otherwise silently
// prevent it from being called, which is not what we want.
template<typename T>
concept HasValidateMethod = requires( T& o ) {
  // Having `o` be non-const allows us to catch when the method
  // is not const, since this concept will be satisfied but not
  // ValidatableStruct.
  { o.validate() };
};

}

} // namespace refl

// Normally the to_canonical and from_canonical don't go into the
// cdr namespace (they go alongside the type being converted in
// the same namespace) because they are found via ADL. However,
// since these are general overloads that must apply to any re-
// flected type regardless of namespace, we obviously cannot
// follow that usual procedure. So we have to put them in the cdr
// namespace so that they are found by ADL (because there are a
// couple of arguments in the cdr namespace in these functions).
namespace cdr {

/****************************************************************
** Enums
*****************************************************************/
// For types that satisfy refl::ReflectedEnum.
template<refl::ReflectedEnum E>
value to_canonical( converter&, E const& o, tag_t<E> ) {
  return std::string{ refl::traits<E>::value_names[static_cast<
      std::underlying_type_t<E>>( o )] };
}

template<refl::ReflectedEnum E>
result<E> from_canonical( converter& conv, value const& v,
                          tag_t<E> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static auto const& names = refl::traits<E>::value_names;
  for( size_t i = 0; i < names.size(); ++i )
    if( names[i] == str ) //
      return static_cast<E>( i );
  return conv.err( "unrecognized value for enum {}: \"{}\"",
                   refl::traits<E>::name, str );
}

/****************************************************************
** Structs
*****************************************************************/
// For types that satisfy refl::ReflectedStruct. Note that con-
// version configuration options (such as whether to write fields
// with default values or what to do with missing fields) are
// handled inside the converter; the implementation of these
// functions are unchanged regardless of those options.
template<refl::ReflectedStruct S>
value to_canonical( converter& conv, S const& o, tag_t<S> ) {
  using Tr = refl::traits<S>;
  static constexpr size_t kNumFields =
      std::tuple_size_v<decltype( Tr::fields )>;
  table tbl;
  FOR_CONSTEXPR_IDX( Idx, kNumFields ) {
    auto& field_desc = std::get<Idx>( Tr::fields );
    auto& field_val  = o.*field_desc.accessor;
    conv.to_field( tbl, std::string( field_desc.name ),
                   field_val );
  };
  return tbl;
}

template<refl::ReflectedStruct S>
result<S> from_canonical( converter& conv, value const& v,
                          tag_t<S> ) {
  using Tr = refl::traits<S>;
  static constexpr size_t kNumFields =
      std::tuple_size_v<decltype( Tr::fields )>;
  S res{};
  UNWRAP_RETURN( tbl, conv.ensure_type<table>( v ) );
  std::unordered_set<std::string> used_keys;
  base::maybe<error>              err;
  FOR_CONSTEXPR_IDX( Idx, kNumFields ) {
    CHECK( !err.has_value() );
    auto& field_desc = std::get<Idx>( Tr::fields );
    using field_type = typename std::remove_cvref_t<
        decltype( field_desc )>::type;
    auto field_val = conv.from_field<field_type>(
        tbl, std::string( field_desc.name ), used_keys );
    if( !field_val.has_value() ) {
      err = std::move( field_val.error() );
      return true; // stop iterating.
    }
    res.*field_desc.accessor = std::move( *field_val );
    return false; // keep going.
  };
  if( err.has_value() ) return *err;
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  if constexpr( refl::detail::HasValidateMethod<S> ) {
    static_assert( refl::ValidatableStruct<S>,
                   "validate method has incorrect signature." );
    if( auto is_valid = res.validate(); !is_valid )
      return conv.err( is_valid.error() );
  }
  return res;
}

/****************************************************************
** Wrappers
*****************************************************************/
// For types that satisfy refl::WrapsReflected.
template<refl::WrapsReflected T>
value to_canonical( converter& conv, T const& o, tag_t<T> ) {
  return conv.to( o.refl() );
}

template<refl::WrapsReflected T>
result<T> from_canonical( converter& conv, value const& v,
                          tag_t<T> ) {
  UNWRAP_RETURN( wrapped,
                 conv.from<refl::wrapped_refltype_t<T>>( v ) );
  return T( std::move( wrapped ) );
}

/****************************************************************
** Reflected Variants
*****************************************************************/
// Reflected variants are variants whose member alternatives are
// all reflected structs.
template<refl::ReflectedStruct... Ts>
value to_canonical( converter&                  conv,
                    base::variant<Ts...> const& o,
                    tag_t<base::variant<Ts...>> ) {
  auto visitor = [&]<typename T>( T const& alt ) {
    using Tr = refl::traits<T>;
    static const std::string kName{ Tr::name };
    table                    res;
    if( o.index() == 0 ) {
      // If we're in the first alternative then let the converter
      // policy decide whether to write the field depending on
      // whether it has its default value or not, so use
      // to_field.
      conv.to_field( res, kName, alt );
      DCHECK( !res.contains( kName ) ||
              res[kName].template is<table>() );
      return res;
    } else {
      // For alternatives beyond the first, do not use to_field,
      // otherwise alternatives that have their default values
      // will not be written (under certain converter options),
      // which is not what we want, since alternatives beyond the
      // first always needs to get written so that we at least
      // know which alternative is selected (if nothing is
      // written then we assume the first). Subfields within the
      // alternative, however, can still be omitted if they as-
      // sume their default values.
      res[kName] = conv.to( alt );
      DCHECK( res.contains( kName ) &&
              res[kName].template is<table>() );
      return res;
    }
  };
  return std::visit( visitor, o );
}

// clang-format off
template<refl::ReflectedStruct... Ts>
  requires( sizeof...( Ts ) > 0 )
result<base::variant<Ts...>> from_canonical(
    converter& conv, value const& v,
    tag_t<base::variant<Ts...>> ) {
  // clang-format on
  UNWRAP_RETURN( tbl, conv.ensure_type<table>( v ) );
  if( tbl.empty() ) {
    // If the table is empty then we have two options:
    //
    //   1. Fail since there is normally supposed to be precisely
    //      one field indicating the active variant.
    //   2. Assume that we should just default-construct the
    //      first variant alternative, i.e., that the first al-
    //      ternative was active but was not written because the
    //      process that wrote it was running under a policy of
    //      not writing values that are equal to a
    //      default-construction.
    //
    // However, we don't decide what to do here, we allow the
    // converter to decide based on how it is configured, so we
    // therefore just call it with a dummy name; whatever name we
    // give it, it won't find it, so it will force it to decide
    // what to do regarding default construction.
    auto res =
        conv.from_field_no_tracking<mp::head_t<mp::list<Ts...>>>(
            tbl, /*key=*/"<dummy>" );
    if( res.has_value() )
      return base::variant<Ts...>{ std::move( *res ) };

    // !! Fall-through to table size check.
  }
  // Make sure that there is precisely one key in the table.
  HAS_VALUE_OR_RET(
      conv.ensure_table_size( tbl, /*expected_size=*/1 ) );
  std::string const&                key = tbl.begin()->first;
  base::maybe<error>                err;
  base::maybe<base::variant<Ts...>> res;
  FOR_CONSTEXPR_IDX( Idx, sizeof...( Ts ) ) {
    using alt_t =
        std::variant_alternative_t<Idx, base::variant<Ts...>>;
    using Tr = refl::traits<alt_t>;
    if( Tr::name != key ) return false; // keep iterating.
    // We found an alternative whose reflected name matches the
    // field name in the table.
    result<alt_t> try_alt =
        conv.from_field_no_tracking<alt_t>( tbl, key );
    if( !try_alt.has_value() )
      err = std::move( try_alt.error() );
    else
      res.emplace( std::move( *try_alt ) );
    return true; // stop iterating.
  };
  if( err.has_value() ) return *err;
  if( !res.has_value() )
    return conv.err( "unrecognized variant alternative '{}'.",
                     key );
  return std::move( *res );
}

} // namespace cdr
