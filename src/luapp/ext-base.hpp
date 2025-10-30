/****************************************************************
**ext-base.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-16.
*
* Description: Lua push/get extensions for base library types.
*
*****************************************************************/
#pragma once

// luapp
#include "ext-userdata.hpp"
#include "ext-usertype.hpp"
#include "ext.hpp"
#include "state.hpp"
#include "types.hpp"

// base
#include "base/maybe.hpp"

namespace lua {

/****************************************************************
** maybe
*****************************************************************/
// NOTE: This first specialization applies not only to simple
// types like int or string, but also applies when T is a refer-
// ence, even to a userdata. So one way to view this when it
// comes to userdata is that the specializations further below
// for userdata with ownership models applies to pushing and pop-
// ping maybe<T> where T is a non-reference userdata type, while
// this specialization can be used to push and pop maybe<T&>,
// since it will push either T& or nil.
template<typename T>
struct type_traits<base::maybe<T>> {
  using M = base::maybe<T>;

  static constexpr int nvalues = nvalues_for<T>();

  // Need an extra template parameter here so that this will work
  // with both cpp-owned and lua-owned T.
  template<typename U>
  static void push( cthread L, U&& m )
  requires Pushable<decltype( *std::forward<U>( m ) )> &&
           base::is_maybe_v<std::remove_cvref_t<U>>
  {
    if( m.has_value() )
      lua::push( L, *std::forward<U>( m ) );
    else {
      for( int i = 0; i < nvalues; ++i ) //
        lua::push( L, nil );
    }
  }

  static lua_expect<M> get( cthread L, int idx, tag<M> )
  requires Gettable<T>
  {
    if( type_of_idx( L, idx ) == type::nil )
      // Result will have a value because nil is a valid value
      // that can be converted to a maybe<T>.
      return M();
    // We don't want to just return the result of lua::get here
    // because then even if it fails, there will be a value in
    // the result. But we don't want this because if the value on
    // the stack is not nil, then it /must/ succeed in conversion
    // in order for the result to have a value.
    auto res = lua::get<T>( L, idx );
    if( !res.has_value() ) return res.error();
    return *res;
  }
};

template<typename T>
requires HasRefUserdataOwnershipModel<T>
struct type_traits<base::maybe<T>>
  : userdata_type_traits_cpp_owned<base::maybe<T>> {};

template<typename T>
requires HasValueUserdataOwnershipModel<T>
struct type_traits<base::maybe<T>>
  : userdata_type_traits_lua_owned<base::maybe<T>> {};

template<typename T>
requires HasValueUserdataOwnershipModel<T>
void define_usertype_for( state&,
                          tag<::base::maybe<T>> ) = delete;

template<typename T>
requires HasRefUserdataOwnershipModel<T>
void define_usertype_for( state& st, tag<::base::maybe<T>> ) {
  using U      = ::base::maybe<T>;
  auto const L = st.resource();
  auto u       = st.usertype.create<U>();

  u["has_value"] = []( U& o ) { return o.has_value(); };
  u["reset"]     = []( U& o ) { o.reset(); };
  u["emplace"]   = []( U& o ) -> T& { return o.emplace(); };
  // This one will return either lua nil or userdata T&.
  u["value"] = [&]( U& o ) -> base::maybe<T&> { return o; };

  table mt           = u[metatable_key];
  auto const __index = mt["__index"].template as<rfunction>();

  mt["__index"] = [L, __index](
                      U& o, any const key ) -> base::maybe<any> {
    if( auto const member = __index( o, key ); member != nil )
      return member.template as<any>();
    if( !o.has_value() )
      throw_lua_error(
          L, "cannot get field '{}' on an empty value.", key );
    any const underlying = as<any>( L, *o );
    return underlying[key];
  };

  mt["__newindex"] = [L]( U& o, any const key, any const val ) {
    if( !o.has_value() )
      throw_lua_error(
          L, "cannot set field '{}' on an empty value.", key );
    any const underlying = as<any>( L, *o );
    underlying[key]      = val;
  };
}

} // namespace lua
