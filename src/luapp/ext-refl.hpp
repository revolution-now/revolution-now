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

// refl
#include "refl/ext.hpp"

namespace lua {

template<refl::ReflectedStruct S>
requires( !PushableViaAdl<S> && !GettableViaAdl<S> )
struct type_traits<S>
  : TraitsForModel<S, e_userdata_ownership_model::owned_by_cpp> {
};

template<refl::WrapsReflected S>
requires( !requires { typename type_traits<S>::type; } )
struct type_traits<S>
  : TraitsForModel<S, e_userdata_ownership_model::owned_by_cpp> {
};

template<refl::ReflectedStruct... Ts>
struct type_traits<base::variant<Ts...>>
  : TraitsForModel<base::variant<Ts...>,
                   e_userdata_ownership_model::owned_by_cpp> {};

template<typename S>
requires requires { typename S::i_am_rds_variant; }
struct type_traits<S>
  : TraitsForModel<S, e_userdata_ownership_model::owned_by_cpp> {
};

} // namespace lua
