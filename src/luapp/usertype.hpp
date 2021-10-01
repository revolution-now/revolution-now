/****************************************************************
**usertype.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-23.
*
* Description: For defining members of userdata types.
*
*****************************************************************/
#pragma once

// luapp
#include "any.hpp"
#include "ext-userdata.hpp"
#include "func-push.hpp"
#include "metatable-key.hpp"
#include "rtable.hpp"

#define LUA_ADD_MEMBER( name ) ut[#name] = &UD::name

namespace lua {

namespace detail {

void usertype_set_member_getter( cthread L, char const* name,
                                 bool is_function );

void usertype_set_member_setter( cthread L, char const* name );

} // namespace detail

template<typename T>
concept CanHaveUsertype =
    !std::is_reference_v<T> && HasUserdataOwnershipModel<T>;

template<CanHaveUsertype T>
using usertype_type_held =
    std::conditional_t<HasRefUserdataOwnershipModel<T>, T&, T>;

template<CanHaveUsertype T>
struct usertype;

template<typename Func, typename T>
concept CanSetAsMember = requires( usertype<T>&       ut,
                                   std::string const& n,
                                   Func&&             func ) {
  ut.set_member( n, std::forward<Func>( func ) );
};

template<CanHaveUsertype T>
struct usertype {
  using Usertype = usertype_type_held<T>;

  usertype( cthread L_ ) : L( L_ ) {
    constexpr e_userdata_ownership_model semantics =
        traits_for<T>::model;
    register_userdata_metatable_if_needed<Usertype, semantics>(
        L );
  }

  template<base::MemberFunctionPointer F>
  requires //
      std::is_same_v<
          std::remove_const_t<
              typename mp::callable_traits<F>::object_type>,
          T>
  void set_member( std::string_view name, F func ) {
    using traits = mp::callable_traits<F>;
    using R      = typename traits::ret_type;
    using O = std::remove_const_t<typename traits::object_type>;
    using args_t = typename traits::arg_types;
    static_assert(
        std::is_same_v<
            R, typename mp::callable_traits<
                   decltype( make_flattened_member_function<O>(
                       func, (args_t*)nullptr ) )>::ret_type> );
    // Delegate to "flattened" version.
    set_member( name, make_flattened_member_function<O>(
                          func, (args_t*)nullptr ) );
  }

  // This is for "flattened" member functions, i.e. regular func-
  // tions that take a reference to the object type as a first
  // parameter.
  // clang-format off
  template<base::NonOverloadedCallable F>
  requires std::is_same_v<
      mp::head_t<typename mp::callable_traits<F>::arg_types>, T&>
  void set_member( std::string_view name, F&& func ) {
    // clang-format on
    push_existing_userdata_metatable<Usertype>( L );
    push_cpp_function( L, FWD( func ) );
    detail::usertype_set_member_getter(
        L, std::string( name ).c_str(),
        /*is_function=*/true );
  }

  template<base::MemberPointer F>
  requires //
      std::is_same_v<
          std::remove_const_t<
              typename mp::callable_traits<F>::object_type>,
          T>
  void set_member( std::string_view name, F&& func ) {
    using traits = mp::callable_traits<F>;
    using R      = typename traits::ret_type;
    using O = std::remove_const_t<typename traits::object_type>;
    using ref_t = decltype( make_member_var_getter_lambda<O>(
        func )( std::declval<O&>() ) );
    static_assert( std::is_same_v<R&, ref_t> );

    push_existing_userdata_metatable<Usertype>( L );
    push_cpp_function(
        L, make_member_var_getter_lambda<O>( func ) );
    detail::usertype_set_member_getter(
        L, std::string( name ).c_str(),
        /*is_function=*/false );

    constexpr bool is_const_member_variable =
        std::is_const_v<std::remove_reference_t<ref_t>>;

    if constexpr( !is_const_member_variable ) {
      push_existing_userdata_metatable<Usertype>( L );
      push_cpp_function(
          L, make_member_var_setter_lambda<O>( func ) );
      detail::usertype_set_member_setter(
          L, std::string( name ).c_str() );
    }
  }

  // clang-format off
private:
  // clang-format on

  struct proxy {
    proxy( usertype& ut, std::string_view name )
      : ut_( ut ), name_( name ) {}

    template<CanSetAsMember<T> Func>
    void operator=( Func&& func ) && {
      ut_.set_member( name_, std::forward<Func>( func ) );
    }

    usertype&   ut_;
    std::string name_;
  };

public:
  proxy operator[]( std::string_view sv ) {
    return proxy( *this, sv );
  }

  table operator[]( metatable_key_t ) {
    push_existing_userdata_metatable<Usertype>( L );
    UNWRAP_CHECK( res, lua::get<table>( L, -1 ) );
    pop_stack( L, 1 );
    return res;
  }

private:
  template<typename O, typename Func, typename... Args>
  static auto make_flattened_member_function(
      Func&& func, mp::type_list<Args...>* ) {
    return [func]( O& o, Args&&... args ) -> decltype( auto ) {
      return ( o.*func )( std::forward<Args>( args )... );
    };
  }

  template<typename O, typename Func>
  static auto make_member_var_getter_lambda( Func&& func ) {
    return
        [func]( O& o ) -> decltype( auto ) { return o.*func; };
  }

  template<typename O, typename Func>
  static auto make_member_var_setter_lambda( Func&& func ) {
    return [func]( O& o, any rhs ) -> decltype( auto ) {
      cthread L = rhs.this_cthread();
      lua::push( L, rhs );
      auto& ref_to_var = o.*func;
      using res_t      = decltype( ref_to_var );
      ref_to_var =
          get_or_luaerr<storage_type_for<res_t>>( L, -1 );
      pop_stack( L, 1 );
    };
  }

  cthread L;
};

} // namespace lua
