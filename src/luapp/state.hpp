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
#include "call.hpp"
#include "cast.hpp"
#include "cthread.hpp"
#include "rfunction.hpp"
#include "rstring.hpp"
#include "rtable.hpp"
#include "rthread.hpp"
#include "usertype.hpp"

// base
#include "base/error.hpp"
#include "base/zero.hpp"

// C++ standard library
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

  state( state&& ) = default;
  state& operator=( state&& ) = default;

private:
  friend Base;

  // Implement base::zero.
  void free_resource();

  // Implement base::zero.
  int copy_resource() const;

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

    table global() noexcept;
    table create() noexcept;

  private:
    cthread L;
  } table;

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
    void      load_file( std::string_view file );

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
  [[nodiscard]] To cast(
      From&&          from,
      base::SourceLoc loc = base::SourceLoc::current() ) {
    cthread L = resource();
    return lua::cast<To>( L, std::forward<From>( from ), loc );
  }

private:
  state( state const& ) = delete;
  state& operator=( state const& ) = delete;
};

// Cannot push an entire global state. You can push a thread
// though (rthread).
void lua_push( cthread, state const& ) = delete;

} // namespace lua
