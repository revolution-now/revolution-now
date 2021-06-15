/****************************************************************
**userdata.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-14.
*
* Description: Stuff for working with userdata.
*
*****************************************************************/
#pragma once

// luapp
#include "types.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/fmt.hpp"
#include "base/func-concepts.hpp"
#include "base/function-ref.hpp"
#include "base/maybe.hpp"

namespace lua {

namespace detail {

void push_string( cthread L, std::string const& s );

bool push_userdata_impl(
    cthread L, int object_size,
    base::function_ref<void( void* )> placement_new,
    LuaCFunction* fmt, LuaCFunction* call_destructor,
    std::string const& type_name );

} // namespace detail

// This will check that the value at idx is a userdata with the
// given name. If not, then it will check fail. Returns the
// pointer to the userdata C buffer.
void* check_udata( cthread L, int idx, char const* name );

// Get the canonical name for a userdata by type. For consis-
// tency, this function should always be used whenever a name for
// a userdata is needed.
template<typename T>
std::string userdata_typename() {
  static_assert( !std::is_rvalue_reference_v<T> );
  static_assert( !std::is_pointer_v<T> );
  std::string res;
  using T1 = T;
  if constexpr( std::is_reference_v<T1> ) res = res + "&";
  using T2 = std::remove_reference_t<T1>;
  if constexpr( std::is_const_v<T2> ) res = " const" + res;
  using T3 = std::remove_const_t<T2>;
  res      = base::demangled_typename<T3>() + res;
  return res;
}

// Push a C++ object as userdata by value, meaning that it will
// be moved into the storage.
//
// Returns true if it is the first time that a userdata of this
// type is being created.
template<typename T>
bool push_userdata_by_value( cthread L, T&& object ) noexcept {
  using fwd_t   = decltype( std::forward<T>( object ) );
  using T_noref = std::remove_reference_t<fwd_t>;
  static_assert( std::is_rvalue_reference_v<fwd_t> );
  static_assert( !std::is_pointer_v<T_noref> );

  static std::string const type_name =
      userdata_typename<T_noref>();

  static constexpr bool fmtable = base::has_fmt<T>;

  static auto call_destructor = []( lua_State* L ) -> int {
    void* ud     = check_udata( L, 1, type_name.c_str() );
    auto* object = static_cast<T*>( ud );
    object->~T();
    return 0;
  };

  static auto tostring = []( lua_State* L ) -> int {
    if constexpr( fmtable ) {
      void* ud     = check_udata( L, 1, type_name.c_str() );
      T*    object = static_cast<T*>( ud );
      detail::push_string(
          L,
          fmt::format( "{}@{}: {}", type_name, ud, *object ) );
      return 1;
    }
    return 0;
  };

  return detail::push_userdata_impl(
      L, sizeof( object ),
      [&]( void* ud ) {
        new( ud ) T( std::forward<T>( object ) );
      },
      fmtable ? tostring : nullptr, call_destructor, type_name );
}

// Push a C++ object as userdata by reference.
//
// Returns true if it is the first time that a userdata of this
// type is being created.
template<typename T>
bool push_userdata_by_ref( cthread L, T&& object ) noexcept {
  using fwd_t   = decltype( std::forward<T>( object ) );
  using T_noref = std::remove_reference_t<fwd_t>;
  static_assert( std::is_lvalue_reference_v<fwd_t> );
  static_assert( !std::is_pointer_v<T_noref> );

  static std::string const type_name =
      userdata_typename<fwd_t>();

  static constexpr bool fmtable =
      base::has_fmt<std::remove_const_t<T_noref>>;

  static auto tostring = []( lua_State* L ) -> int {
    if constexpr( fmtable ) {
      void*  ud     = check_udata( L, 1, type_name.c_str() );
      auto** object = static_cast<T_noref**>( ud );
      CHECK( object != nullptr );
      std::string res = fmt::format( "{}@{}: ", type_name, ud );
      if( *object != nullptr )
        res += fmt::format( "{}", **object );
      else
        res += "nullptr";
      detail::push_string( L, res );
      return 1;
    }
    return 0;
  };

  return detail::push_userdata_impl(
      L, sizeof( void* ),
      [&]( void* ud ) {
        auto** pointer_storage = static_cast<T_noref**>( ud );
        // Store a pointer to the object.
        *pointer_storage = &object;
      },
      fmtable ? tostring : nullptr,
      /*call_destructor=*/nullptr, type_name );
}

// TODO: as ambiguities arise with this overload, add conditions
// into the requires clause to eliminate the unwanted ones.
// clang-format off
template<typename T>
void lua_push( cthread L, T&& o )
  requires(
      !std::is_scalar_v<std::remove_cvref_t<T>> &&
      !std::is_constructible_v<std::string, T> &&
      !base::NonOverloadedCallable<T> &&
      !std::is_pointer_v<std::remove_reference_t<T>> &&
       std::is_reference_v<decltype(std::forward<T>(o))> ) {
  // clang-format on
  using fwd_t = decltype( std::forward<T>( o ) );
  constexpr bool is_lvalue_ref =
      std::is_lvalue_reference_v<fwd_t>;
  if constexpr( is_lvalue_ref )
    push_userdata_by_ref( L, std::forward<T>( o ) );
  else
    push_userdata_by_value( L, std::forward<T>( o ) );
}

} // namespace lua
