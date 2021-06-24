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
struct usertype {
  using Usertype = usertype_type_held<T>;

  usertype( cthread L_ ) : L( L_ ) {
    constexpr e_userdata_ownership_model semantics =
        traits_for<T>::model;
    register_userdata_metatable_if_needed<Usertype, semantics>(
        L );
  }

  template<typename F>
  void set_constructor( F&& ) //
      requires HasValueUserdataOwnershipModel<T> {
    //
  }

  template<typename F>
  requires                              //
      base::MemberFunctionPointer<F> && //
      std::is_same_v<
          std::remove_const_t<
              typename mp::callable_traits<F>::object_type>,
          T>
  void set_member( std::string_view name, F func ) {
    using traits = mp::callable_traits<F>;
    using R      = typename traits::ret_type;
    using O = std::remove_const_t<typename traits::object_type>;
    using args_t = typename traits::arg_types;
    push_existing_userdata_metatable<Usertype>( L );
    static_assert(
        std::is_same_v<
            R, typename mp::callable_traits<
                   decltype( make_member_function_lambda<O>(
                       func, (args_t*)nullptr ) )>::ret_type> );
    push_cpp_function( L, make_member_function_lambda<O>(
                              func, (args_t*)nullptr ) );
    detail::usertype_set_member_getter(
        L, std::string( name ).c_str(),
        /*is_function=*/true );
  }

  template<typename F>
  requires                      //
      base::MemberPointer<F> && //
      std::is_same_v<
          std::remove_const_t<
              typename mp::callable_traits<F>::object_type>,
          T>
  void set_member( std::string_view name, F&& func ) {
    using traits = mp::callable_traits<F>;
    using R      = typename traits::ret_type;
    using O = std::remove_const_t<typename traits::object_type>;
    using args_t = typename traits::arg_types;
    static_assert( mp::type_list_size_v<args_t> == 0 );
    static_assert(
        std::is_same_v<
            R&, decltype( make_member_var_getter_lambda<O>(
                    func )( std::declval<O&>() ) )> );

    push_existing_userdata_metatable<Usertype>( L );
    push_cpp_function(
        L, make_member_var_getter_lambda<O>( func ) );
    detail::usertype_set_member_getter(
        L, std::string( name ).c_str(),
        /*is_function=*/false );

    push_existing_userdata_metatable<Usertype>( L );
    push_cpp_function(
        L, make_member_var_setter_lambda<O>( func ) );
    detail::usertype_set_member_setter(
        L, std::string( name ).c_str() );
  }

  // clang-format off
private:
  // clang-format on

  template<typename O, typename Func, typename... Args>
  static auto make_member_function_lambda(
      Func&& func, mp::type_list<Args...>* ) {
    // Need to take O as a reference because that is what our C++
    // <-> userdata conversion framework can convert between (it
    // currently does not support pointers to userdata).
    return [func]( O& o, Args&&... args ) -> decltype( auto ) {
      return ( o.*func )( std::forward<Args>( args )... );
    };
  }

  template<typename O, typename Func>
  static auto make_member_var_getter_lambda( Func&& func ) {
    // Need to take O as a reference because that is what our C++
    // <-> userdata conversion framework can convert between (it
    // currently does not support pointers to userdata).
    return
        [func]( O& o ) -> decltype( auto ) { return o.*func; };
  }

  template<typename O, typename Func>
  static auto make_member_var_setter_lambda( Func&& func ) {
    // Need to take O as a reference because that is what our C++
    // <-> userdata conversion framework can convert between (it
    // currently does not support pointers to userdata).
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
