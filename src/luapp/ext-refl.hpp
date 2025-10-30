/****************************************************************
**ext-refl.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-26.
*
* Description: Lua type traits for reflected types.
*
*****************************************************************/
#pragma once

// luapp
#include "ext-userdata.hpp"
#include "ext-usertype.hpp"
#include "state.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/query-struct.hpp"

// traverse
#include "traverse/ext.hpp"

// base
#include "base/to-str.hpp"

namespace lua {

/****************************************************************
** ReflectedStruct
*****************************************************************/
template<refl::ReflectedStruct S>
requires( !PushableViaAdl<S> && !GettableViaAdl<S> )
struct type_traits<S>
  : TraitsForModel<S, e_userdata_ownership_model::owned_by_cpp> {
};

template<refl::ReflectedStruct S>
requires( !PushableViaAdl<S> && !GettableViaAdl<S> )
void define_usertype_for( state& st, tag<S> ) {
  auto u = st.usertype.create<S>();
  trv::traverse(
      refl::traits<S>::fields,
      // Here the `name` passed into this lambda is e.g. "<0>",
      // "<1>", because we are traversing the tuple. So we need
      // field.name to get the real field name.
      [&]( auto& field, std::string_view const /*name*/ ) {
        using FieldType =
            std::remove_cvref_t<decltype( field )>::type;
        if constexpr( !HasValueUserdataOwnershipModel<
                          FieldType> )
          u[field.name] = field.accessor;
      } );
}

/****************************************************************
** WrapsReflected
*****************************************************************/
template<refl::WrapsReflected S>
requires( !requires { typename type_traits<S>::type; } )
struct type_traits<S>
  : TraitsForModel<S, e_userdata_ownership_model::owned_by_cpp> {
};

// NOTE: no default specialization for refl::WrapsReflected;
// those have to be provided for each type specially since we
// have no way of knowing what's C++ API is.

/****************************************************************
** Reflected Variant
*****************************************************************/
template<refl::ReflectedStruct... Ts>
struct type_traits<base::variant<Ts...>>
  : TraitsForModel<base::variant<Ts...>,
                   e_userdata_ownership_model::owned_by_cpp> {};

template<typename S>
requires requires { typename S::i_am_rds_variant; }
struct type_traits<S>
  : TraitsForModel<S, e_userdata_ownership_model::owned_by_cpp> {
};

// Lua API for sumtypes:
//
//   local s = ...sumtype...
//
//   local a1 = s:select_red()
//   a1.xyz = ...
//
//   s:select_green()
//   s.green.xyz = 5
//
//   print( s.green )
//
template<typename U, refl::ReflectedStruct... Ts>
void define_usertype_rds_variant_impl(
    base::variant<Ts...> const*, auto& u ) {
  using V                   = base::variant<Ts...>;
  static auto const& kNames = refl::alternative_names<V>();

  table mt           = u[metatable_key];
  auto const __index = mt["__index"].template as<rfunction>();

  // TBD: Add some members here.

  mt["__index"] = [__index](
                      U& o, any const key ) -> base::maybe<any> {
    base::maybe<any> res;
    if( auto const member = __index( o, key ); member != nil ) {
      res = member.template as<any>();
      return res;
    }
    auto const maybe_key = safe_as<std::string>( key );
    if( !maybe_key.has_value() ) return res;
    auto const L  = __index.this_cthread();
    auto const st = state::view( L );

    if( std::string_view select_str = *maybe_key;
        select_str.starts_with( "select_" ) ) {
      select_str.remove_prefix( 7 );
      std::string const variant_str{ select_str };
      auto const f = [L, variant_str, &o] -> any {
        auto const st = state::view( L );
        any res       = as<any>( L, nil );
        [&]<size_t... I>( std::index_sequence<I...> ) {
          auto const fn =
              [&]<size_t Idx>(
                  std::integral_constant<size_t, Idx> ) {
                static_assert( Idx < std::variant_size_v<V> );
                if( kNames[Idx] == variant_str ) {
                  auto& val = o.template emplace<Idx>();
                  res       = as<any>( L, val );
                }
              };
          ( fn( std::integral_constant<size_t, I>{} ), ... );
        }( std::make_index_sequence<sizeof...( Ts )>() );
        LUA_CHECK( st, res != nil, "unknown variant: {}",
                   variant_str );
        return res;
      };
      return as<any>( L, std::move( f ) );
    }
    [&]<size_t... I>( std::index_sequence<I...> ) {
      auto const fn = [&]<size_t Idx>(
                          std::integral_constant<size_t, Idx> ) {
        static_assert( Idx < std::variant_size_v<V> );
        if( o.index() == Idx ) {
          if( *maybe_key == kNames[Idx] )
            res = as<any>( L, std::get<Idx>( o ) );
        }
      };
      ( fn( std::integral_constant<size_t, I>{} ), ... );
    }( std::make_index_sequence<sizeof...( Ts )>() );
    return res;
  };

  // NOTE: there is no __newindex here since the entire API hap-
  // pens to be handled in the __index.
}

// Delegates to the one above.
template<refl::ReflectedStruct... Ts>
void define_usertype_for( state& st,
                          tag<base::variant<Ts...>> ) {
  using U = base::variant<Ts...>;
  auto u  = st.usertype.create<U>();
  define_usertype_rds_variant_impl<U>( (U*){}, u );
}

// Delegates to the one above.
template<typename S>
requires requires { typename S::i_am_rds_variant; }
void define_usertype_for( state& st, tag<S> ) {
  // NOTE: we must create the usertype here and call the impl
  // method as opposed to delgating to the above variant method
  // because we need to ensure that the usertype creating code
  // runs only once per type, and this type S is distinct from
  // the variant of structs type. The reason that the usertype
  // creation code must only run once is because it chains
  // __index methods.
  auto u = st.usertype.create<S>();
  define_usertype_rds_variant_impl<S>( (typename S::Base*){},
                                       u );
}

} // namespace lua
