/****************************************************************
**to-str.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-09.
*
* Description: to_str extension for reflected types.
*
*****************************************************************/
#pragma once

// refl
#include "ext.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/error.hpp"
#include "base/meta.hpp"
#include "base/to-str.hpp"

/****************************************************************
** Helpers
*****************************************************************/
namespace refl {
namespace detail {

template<typename... Types>
std::string type_list_tuple_to_names( std::tuple<Types...>* ) {
  return base::type_list_to_names<Types...>();
}

// tmpl_params is only non-empty for templated sumtype alterna-
// tives where the "namespace" is actually a templated class.
std::string build_ns_prefix_from_refl_ns(
    std::string_view ns, std::string_view tmpl_params );

}
} // namespace refl

// Normally the to_str goes into the same namespace as the type
// in question because they are found via ADL. However, since
// these are general overloads that must apply to any reflected
// type regardless of namespace, we obviously cannot follow that
// usual procedure. So we have to put them in the base namespace
// so that they are found by ADL (because there is an argument in
// the base namespace in these functions).
namespace base {

/****************************************************************
** Enums
*****************************************************************/
// For types that satisfy refl::ReflectedEnum.
template<refl::ReflectedEnum E>
void to_str( E const& o, std::string& out, ADL_t ) {
  auto  idx   = static_cast<int>( o );
  auto& names = refl::traits<E>::value_names;
  DCHECK( idx >= 0 );
  DCHECK( idx < int( names.size() ) );
  out += names[idx];
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
void to_str( S const& o, std::string& out, ADL_t tag ) {
  using Tr = refl::traits<S>;
  static constexpr size_t kNumFields =
      std::tuple_size_v<decltype( Tr::fields )>;
  static std::string const tmpl_params = [] {
    std::string res;
    using tuple_ptr = typename Tr::template_types*;
    if constexpr( std::tuple_size_v<
                      typename Tr::template_types> == 0 )
      return res;
    res += "<";
    res += refl::detail::type_list_tuple_to_names(
        tuple_ptr{ nullptr } );
    res += '>';
    return res;
  }();
  static std::string const ns =
      refl::detail::build_ns_prefix_from_refl_ns(
          Tr::ns,
          Tr::is_sumtype_alternative ? tmpl_params : "" );
  out += ns;
  out += Tr::name;
  if constexpr( !Tr::is_sumtype_alternative ) out += tmpl_params;
  if( kNumFields == 0 ) return;
  out += "{";
  FOR_CONSTEXPR_IDX( Idx, kNumFields ) {
    auto& field_desc = std::get<Idx>( Tr::fields );
    auto& field_val  = o.*field_desc.accessor;
    out += field_desc.name;
    out += '=';
    to_str( field_val, out, tag );
    out += ',';
  };
  if constexpr( kNumFields > 0 ) out.pop_back();
  out += "}";
}

/****************************************************************
** Wrappers
*****************************************************************/
// For types that satisfy refl::WrapsReflected.
template<refl::WrapsReflected T>
void to_str( T const& o, std::string& out, ADL_t tag ) {
  static std::string const ns =
      refl::detail::build_ns_prefix_from_refl_ns(
          T::refl_ns, /*tmpl_params=*/"" );
  out += ns;
  out += T::refl_name;

  std::string refl_out;
  to_str( o.refl(), refl_out, tag );
  auto pos = refl_out.find_first_of( '{' );
  if( pos == std::string::npos ) return;
  out += std::string( refl_out.begin() + pos, refl_out.end() );
}

} // namespace base
