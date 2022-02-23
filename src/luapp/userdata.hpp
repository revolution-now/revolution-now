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
#include "ext.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/func-concepts.hpp"
#include "base/function-ref.hpp"
#include "base/maybe.hpp"

namespace lua {

// We support a canned ownership model, with two choices.
enum class e_userdata_ownership_model {
  // Objects of that type are stored as C++ objects by value and
  // are always owned by Lua. They are always non-const.
  owned_by_cpp,
  // Objects of that type are stored as C++ objects owned by C++,
  // and Lua always just holds references to them. They are al-
  // ways non-const.
  owned_by_lua,
};

// This will check that the value at idx is a userdata with the
// given name. If not, then it will check fail. Returns the
// pointer to the userdata C buffer.
void* check_udata( cthread L, int idx, char const* name );

// Same as above but will return nothing if we dont' have a user
// data by the given name.
base::maybe<void*> try_udata( cthread L, int idx,
                              char const* name );

// Get the canonical name for a userdata by type. For consis-
// tency, this function should always be used whenever a name for
// a userdata is needed.
template<typename T>
std::string const& userdata_typename() {
  static std::string const name = [] {
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
  }();
  return name;
}

namespace detail {

void push_string( cthread L, std::string const& s );

void push_userdata_impl(
    cthread L, int object_size,
    base::function_ref<void( void* )> placement_new,
    std::string const&                type_name );

bool register_userdata_metatable_if_needed_impl(
    cthread L, e_userdata_ownership_model semantics,
    LuaCFunction* fmt, LuaCFunction* call_destructor,
    std::string const& type_name );

void push_existing_userdata_metatable_impl(
    cthread L, std::string const& type_name );

template<typename T>
bool register_userdata_metatable_by_val_if_needed( cthread L ) {
  static_assert( !std::is_pointer_v<T> );
  static constexpr bool showable =
      base::Show<std::remove_const_t<T>>;
  // FIXME: see comment on corresponding code in the C++-owned
  // function below for an explanation of this.
  static_assert(
      showable ||
          // Use NonOverloadedCallable to help filter out lamb-
          // das; this is imperfect, but hopefully should do the
          // trick.
          base::NonOverloadedCallable<std::remove_cvref_t<T>>,
      "see comment above." ); // workaround

  static std::string const type_name = userdata_typename<T>();

  static auto call_destructor = []( lua_State* L ) -> int {
    void* ud     = check_udata( L, 1, type_name.c_str() );
    auto* object = reinterpret_cast<T*>( ud );
    std::destroy_at( object );
    return 0;
  };

  static auto tostring = []( lua_State* L ) -> int {
    if constexpr( showable ) {
      void* ud     = check_udata( L, 1, type_name.c_str() );
      T*    object = reinterpret_cast<T*>( ud );
      detail::push_string(
          L,
          fmt::format( "{}@{}: {}", type_name, ud, *object ) );
      return 1;
    }
    return 0;
  };

  static constexpr LuaCFunction* fmt_func =
      showable ? +tostring : nullptr;

  return detail::register_userdata_metatable_if_needed_impl(
      L, e_userdata_ownership_model::owned_by_lua, fmt_func,
      call_destructor, type_name );
}

template<typename T>
bool register_userdata_metatable_owned_by_cpp_if_needed(
    cthread L ) {
  using T_noref = std::remove_reference_t<T>;
  static_assert( !std::is_pointer_v<T_noref> );
  // FIXME: this is an ODR violation waiting to happen now that
  // we have reflected types that only implement to_str if the
  // translation unit happens to include the right headers. Maybe
  // the right solution is to make sure to have the Rds generated
  // files (which is where most reflected types are) include the
  // refl/to-str. In the mean time, in order to prevent any is-
  // sues, we will just static_assert that all types are showable
  // at this point, which should prevent any issues.
  static constexpr bool showable =
      base::Show<std::remove_const_t<T_noref>>;
  static_assert( showable, "see comment above." ); // workaround

  static std::string const type_name = userdata_typename<T>();

  static auto tostring = []( lua_State* L ) -> int {
    if constexpr( showable ) {
      void*  ud     = check_udata( L, 1, type_name.c_str() );
      auto** object = reinterpret_cast<T_noref**>( ud );
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

  static constexpr LuaCFunction* fmt_func =
      showable ? +tostring : nullptr;

  return detail::register_userdata_metatable_if_needed_impl(
      L, e_userdata_ownership_model::owned_by_cpp, fmt_func,
      /*call_destructor=*/nullptr, type_name );
}

} // namespace detail

// WARNING: the metadata for the type must have already been reg-
// istered by calling register_userdata_metatable_if_needed, oth-
// erwise this function will check-fail.
template<typename T>
void push_existing_userdata_metatable( cthread L ) {
  static std::string const type_name = userdata_typename<T>();
  detail::push_existing_userdata_metatable_impl( L, type_name );
}

template<typename T, e_userdata_ownership_model Semantics>
bool register_userdata_metatable_if_needed( cthread L ) {
  if constexpr( Semantics ==
                e_userdata_ownership_model::owned_by_lua )
    return detail::register_userdata_metatable_by_val_if_needed<
        T>( L );
  else if constexpr( Semantics ==
                     e_userdata_ownership_model::owned_by_cpp )
    return detail::
        register_userdata_metatable_owned_by_cpp_if_needed<T>(
            L );
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

  // This can't be static because of L.
  bool metatable_created = register_userdata_metatable_if_needed<
      T_noref, e_userdata_ownership_model::owned_by_lua>( L );

  detail::push_userdata_impl(
      L, sizeof( object ),
      [&]( void* ud ) {
        new( ud ) T( std::forward<T>( object ) );
      },
      type_name );

  return metatable_created;
}

// Push a C++ object as userdata by reference.
//
// Returns true if it is the first time that a userdata of this
// type is being created.
template<typename T>
bool push_userdata_by_ref( cthread L, T& object ) noexcept {
  using fwd_t = T&;
  static_assert( std::is_lvalue_reference_v<fwd_t> );
  static_assert( !std::is_pointer_v<T> );

  static std::string const type_name =
      userdata_typename<fwd_t>();

  // This can't be static because of L.
  bool metatable_created = register_userdata_metatable_if_needed<
      fwd_t, e_userdata_ownership_model::owned_by_cpp>( L );

  detail::push_userdata_impl(
      L, sizeof( void* ),
      [&]( void* ud ) {
        auto** pointer_storage = static_cast<T**>( ud );
        // Store a pointer to the object.
        *pointer_storage = &object;
      },
      type_name );

  return metatable_created;
}

} // namespace lua
