/****************************************************************
**state.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: RAII type for global lua state.
*
*****************************************************************/
#pragma once

// luapp
#include "as.hpp"
#include "call.hpp"
#include "cthread.hpp"
#include "error.hpp"
#include "func-push.hpp"
#include "rfunction.hpp"
#include "rstring.hpp"
#include "rtable.hpp"
#include "rthread.hpp"
#include "usertype.hpp"

// base
#include "base/error.hpp"
#include "base/zero.hpp"

// C++ standard library
#include <source_location>
#include <string_view>

namespace lua {

/****************************************************************
** state
*****************************************************************/
// This represents a global Lua state (including all of the
// threads that it contains). It is an RAII object and thus owns
// the state.
struct state : base::zero<state, cthread> {
  using Base = base::zero<state, cthread>;

 private:
  // Creates a non-owned (view) state.
  state( cthread L );

 public:
  // Creates an owned Lua state and sets a panic function.
  state();

  // Creates a non-owning view of the state.
  static state view( cthread L ) { return state( L ); }

  state( state&& )            = default;
  state& operator=( state&& ) = default;

 private:
  friend Base;

  // Implement base::zero.
  void free_resource();

 public:
  /**************************************************************
  ** Threads
  ***************************************************************/
  struct Thread {
    Thread( cthread cth );

    // This is guaranteed to be the main thread because it is the
    // one that we get when we create the state.
    rthread main() noexcept;

    // Create a new thread with empty stack.
    rthread create() noexcept;

    // Create a new coroutine, which is defined as a new thread
    // with a function pushed onto its stack, ready to be run by
    // coroutine.resume.
    rthread create_coro( rfunction func ) noexcept;

   private:
    cthread L;
  } thread;

  /**************************************************************
  ** Strings
  ***************************************************************/
  struct String {
    String( cthread cth ) : L( cth ) {}

    rstring create( std::string_view sv ) noexcept;

   private:
    cthread L;
  } string;

  /**************************************************************
  ** Tables
  ***************************************************************/
  struct Table {
    Table( cthread cth );

    ::lua::table global() noexcept;
    ::lua::table create() noexcept;

   private:
    cthread L;
  } table;

  /**************************************************************
  ** Functions
  ***************************************************************/
  struct Function {
    Function( cthread cth );

    template<Pushable Func>
    rfunction create( Func&& func ) noexcept {
      lua::push( L, std::forward<Func>( func ) );
      rfunction res = lua::get_or_luaerr<rfunction>( L, -1 );
      pop_stack( L, 1 );
      return res;
    }

   private:
    cthread L;
  } function;

  /**************************************************************
  ** Libs
  ***************************************************************/
  struct Lib {
    Lib( cthread cth ) : L( cth ) {}

    void open_all();

   private:
    cthread L;
  } lib;

  /**************************************************************
  ** Usertype
  ***************************************************************/
  struct Usertype {
    Usertype( cthread cth ) : L( cth ) {}

    template<CanHaveUsertype T>
    usertype<T> create() {
      return lua::usertype<T>( L );
    }

   private:
    cthread L;
  } usertype;

  /**************************************************************
  ** Scripts
  ***************************************************************/
  struct Script {
    Script( cthread cth );

    rfunction load( std::string_view code ) noexcept;

    lua_expect<rfunction> load_safe(
        std::string_view code ) noexcept;

    void operator()( std::string_view code );

    template<GettableOrVoid R = void>
    R run( std::string_view code ) {
      lua::push( L, load( code ) );
      return call_lua_unsafe_and_get<R>( L );
    }

    template<GettableOrVoid R = void>
    error_type_for_return_type<R> run_safe(
        std::string_view code ) noexcept {
      UNWRAP_RETURN( func, load_safe( code ) );
      lua::push( L, func );
      return call_lua_safe_and_get<R>( L );
    }

    template<GettableOrVoid R = void>
    R run_file( std::string_view file ) {
      load_file( file );
      return call_lua_unsafe_and_get<R>( L );
    }

    template<GettableOrVoid R = void>
    error_type_for_return_type<R> run_file_safe(
        std::string_view file ) noexcept {
      HAS_VALUE_OR_RET( load_file_safe( file ) );
      return call_lua_safe_and_get<R>( L );
    }

   private:
    lua_valid load_file_safe( std::string_view file );
    void load_file( std::string_view file );

    cthread L;
  } script;

  /**************************************************************
  ** Indexer
  ***************************************************************/
  template<typename U>
  auto operator[]( U&& idx ) noexcept {
    return table.global()[std::forward<U>( idx )];
  }

  /**************************************************************
  ** Casting
  ***************************************************************/
  template<typename To, typename From>
  requires Castable<From, To>
  [[nodiscard]] To as( From&& from,
                       std::source_location loc =
                           std::source_location::current() ) {
    cthread L = resource();
    return lua::as<To>( L, std::forward<From>( from ), loc );
  }

  /**************************************************************
  ** Throw error
  ***************************************************************/
  template<typename... Args>
  [[noreturn]] void error(
      fmt::format_string<Args...> const fmt_str,
      Args&&... args ) const {
    throw_lua_error( resource(), fmt_str, FWD( args )... );
  }

  template<typename... Args>
  [[noreturn]] void error( std::string const& msg ) const {
    throw_lua_error( resource(), "{}", msg );
  }

 private:
  state( state const& )            = delete;
  state& operator=( state const& ) = delete;
};

// Cannot push an entire global state. You can push a thread
// though (rthread).
void lua_push( cthread, state const& ) = delete;

} // namespace lua
