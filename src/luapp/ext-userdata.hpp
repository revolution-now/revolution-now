/****************************************************************
**ext-userdata.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-20.
*
* Description: Pre-defined type_traits for userdata.
*
*****************************************************************/
#pragma once

// luapp
#include "ext.hpp"
#include "userdata.hpp"

namespace lua {

/****************************************************************
** Userdata traits
*****************************************************************/
template<typename T>
struct userdata_type_traits_cpp_owned {
  // This should only be a base type.
  static_assert( !std::is_reference_v<T> );
  static_assert( !std::is_const_v<T> );

  static constexpr auto model =
      e_userdata_ownership_model::owned_by_cpp;

  // When we call a C++ function from Lua that takes a parameter
  // of type T that means that we need only to extract a T& from
  // Lua in order to call the C++ function, since the object
  // lives in native form somewhere (that is owned by C++).
  using storage_type           = T&;
  static constexpr int nvalues = 1;

  static base::maybe<T&> get( cthread L, int idx,
                              tag<T&> ) noexcept {
    static std::string type_name = userdata_typename<T&>();
    base::maybe<void*> ud =
        try_udata( L, idx, type_name.c_str() );
    if( !ud.has_value() ) return base::nothing;
    T** object = static_cast<T**>( *ud );
    return **object;
  }

  static base::maybe<T const&> get( cthread L, int idx,
                                    tag<T const&> ) noexcept {
    // userdatas are stored as non-const refs, so we can always
    // get non-const refs from them and const refs.
    static std::string type_name = userdata_typename<T&>();
    base::maybe<void*> ud =
        try_udata( L, idx, type_name.c_str() );
    if( !ud.has_value() ) return base::nothing;
    T const** object = static_cast<T const**>( *ud );
    return **object;
  }

  // This lvalue ref will prevent pushing temporaries, which we
  // want to prevent because Lua is only holding them by refer-
  // ence, i.e. they are supposed to live somewhere in C++ land.
  static void push( cthread L, T& o ) noexcept {
    push_userdata_by_ref( L, o );
  }
};

template<typename T>
struct userdata_type_traits_lua_owned {
  // This should only be a base type.
  static_assert( !std::is_reference_v<T> );
  static_assert( !std::is_const_v<T> );

  static constexpr auto model =
      e_userdata_ownership_model::owned_by_lua;

  // When we call a C++ function from Lua that takes a parameter
  // of type T that means that we need only to extract a T& from
  // Lua in order to call the C++ function, since the object
  // lives in native form in Lua.
  using storage_type           = T&;
  static constexpr int nvalues = 1;

  static base::maybe<T&> get( cthread L, int idx,
                              tag<T&> ) noexcept {
    static std::string type_name = userdata_typename<T>();
    base::maybe<void*> ud =
        try_udata( L, idx, type_name.c_str() );
    if( !ud.has_value() ) return base::nothing;
    T* object = static_cast<T*>( *ud );
    return *object;
  }

  static base::maybe<T const&> get( cthread L, int idx,
                                    tag<T const&> ) noexcept {
    static std::string type_name = userdata_typename<T>();
    base::maybe<void*> ud =
        try_udata( L, idx, type_name.c_str() );
    if( !ud.has_value() ) return base::nothing;
    T const* object = static_cast<T const*>( *ud );
    return *object;
  }

  // Pushing temporaries is OK because Lua owns the value and it
  // will be moved.
  static void push( cthread L, T&& o ) noexcept {
    push_userdata_by_value( L, std::move( o ) );
  }

  // The below two methods are deleted to force the user to make
  // a copy at the call site (to make the copy more explicit) if
  // they don't wish to move the object.
  static void push( cthread L, T const& o ) noexcept = delete;
  static void push( cthread L, T& o ) noexcept       = delete;
};

template<typename T, e_userdata_ownership_model Model>
using TraitsForModel = std::conditional_t<
    Model == e_userdata_ownership_model::owned_by_cpp,
    userdata_type_traits_cpp_owned<T>,
    std::conditional_t<
        Model == e_userdata_ownership_model::owned_by_lua,
        userdata_type_traits_lua_owned<T>, void>>;

template<typename T>
concept HasRefUserdataOwnershipModel = HasTraitsNvalues<T> &&
    std::is_base_of_v<userdata_type_traits_cpp_owned<T>,
                      traits_for<T>>;

template<typename T>
concept HasValueUserdataOwnershipModel = HasTraitsNvalues<T> &&
    std::is_base_of_v<userdata_type_traits_lua_owned<T>,
                      traits_for<T>>;
template<typename T>
concept HasUserdataOwnershipModel =
    HasRefUserdataOwnershipModel<T> ||
    HasValueUserdataOwnershipModel<T>;

#define LUA_USERDATA_TRAITS( type, model ) \
  template<>                               \
  struct type_traits<type>                 \
    : TraitsForModel<type, e_userdata_ownership_model::model>

} // namespace lua
