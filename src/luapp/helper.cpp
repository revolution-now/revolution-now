/****************************************************************
**helper.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-29.
*
* Description: High-level Lua helper object.
*
*****************************************************************/
#include "helper.hpp"

// luapp
#include "c-api.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/error.hpp"

// Lua
#include "lauxlib.h"

using namespace std;

namespace lua {

namespace {} // namespace

/****************************************************************
** helper
*****************************************************************/
helper::helper( cthread L ) : C( c_api::view( L ) ) {}

helper::helper() : C() {}

c_api& helper::api() noexcept { return C; }

void helper::openlibs() noexcept { C.openlibs(); }

int helper::noref() { return LUA_NOREF; }

// TODO: use this as a model for loading a piece of code once
// into the registry. Can remove eventually.
void helper::tables_slow( std::string_view path ) noexcept {
  if( tables_func_ref == LUA_NOREF ) {
    static string const code = R"lua(
      local path = ...
      local tab = _G
      for p in string.gmatch( path, '[^.]+' ) do
        tab[p] = tab[p] or {{}}
        tab = tab[p]
      end
    )lua";
    CHECK_HAS_VALUE( C.loadstring( code.c_str() ) );
    tables_func_ref = C.ref_registry();
  }

  CHECK( C.registry_get( tables_func_ref ) ==
         e_lua_type::function );
  C.push( string( path ).c_str() );
  CHECK_HAS_VALUE( C.pcall( /*nargs=*/1, /*nresults=*/0 ) );
}

void helper::tables( c_string_list const& path ) noexcept {
  C.pushglobaltable();
  for( char const* elem : path ) {
    e_lua_type type = C.getfield( /*table_idx=*/-1, elem );
    switch( type ) {
      case e_lua_type::nil: {
        C.pop(); // nil
        C.newtable();
        C.setfield( /*table_idx=*/-2, elem );
        C.getfield( /*table_idx=*/-1, elem );
        break;
      }
      case e_lua_type::table: {
        break;
      }
      default: {
        FATAL( "field {} is not a table, instead is of type {}.",
               elem, type );
      }
    }
  }
  C.pop( path.size() + 1 ); // + global table
}

void helper::traverse_and_push_table_and_key(
    c_string_list const& path ) noexcept {
  CHECK( path.size() > 0 );
  C.pushglobaltable();
  auto i = path.begin();
  while( true ) {
    char const* elem = *i;
    if( i == path.end() - 1 ) {
      C.push( elem );
      return;
    }
    e_lua_type type = C.getfield( /*table_idx=*/-1, elem );
    CHECK( type == e_lua_type::table,
           "field '{}' is not a table.", elem );
    ++i;
    C.swap_top();
    C.pop();
  }
}

e_lua_type helper::push_path(
    c_string_list const& path ) noexcept {
  C.pushglobaltable();
  char const* elem = "_G";
  e_lua_type  type = e_lua_type::table;
  auto        i    = path.begin();
  while( i != path.end() ) {
    CHECK( C.type_of( -1 ) == e_lua_type::table,
           "field '{}' is not a table.", elem );
    elem = *i;
    type = C.getfield( /*table_idx=*/-1, elem );
    ++i;
    C.swap_top();
    C.pop();
  }
  return type;
}

void helper::push_stateless_lua_c_function(
    LuaCFunction* func ) noexcept {
  C.push( func );
}

template<typename T>
bool helper::create_userdata( T&& object ) noexcept {
  int initial_stack_size = C.stack_size();

  static string const type_name = base::demangled_typename<T>();

  // 1. Create userdata to hold unique_func.
  void* ud = C.newuserdata( sizeof( object ) );
  // Stack:
  //   userdata
  DCHECK( C.stack_size() == initial_stack_size + 1 );

  // The ud pointer, since it is allocated by malloc, will appar-
  // ently have the correct alignment to store any type.
  new( ud ) T( forward<T>( object ) );
  // !! `object` is moved-from at this point.

  // 2. Get the metatable for this userdata type and set it. The
  // first time we do this for this type, created == true.
  bool metatable_created =
      C.udata_newmetatable( type_name.c_str() );
  // Stack:
  //   metatable
  //   userdata
  DCHECK( C.stack_size() == initial_stack_size + 2 );

  if( metatable_created ) {
    // This is the first time that we are creating a userdata of
    // this type, so the metatable will basically be empty. So
    // now we have to give it a __gc method so that it will get
    // freed properly when Lua garbage collects it. We must set
    // the __gc method in the metatable *before* setting the
    // metatable on the userdata.
    static auto gc = []( lua_State* L ) -> int {
      // We should get the object to be garbage collected as the
      // first argument. Use `testudata` instead of `checkudata`
      // so that we throw a C++ error instead of a Lua error.
      void* ud = luaL_testudata( L, 1, type_name.c_str() );
      DCHECK(
          ud != nullptr,
          "__gc method expected type {} but did not find it.",
          type_name );
      T* object = static_cast<T*>( ud );
      object->~T();
      return 0;
    };
    C.push( gc );
    // Stack:
    //   __gc method
    //   metatable
    //   userdata
    DCHECK( C.stack_size() == initial_stack_size + 3 );

    C.setfield( /*table_idx=*/-2, "__gc" );
    // Stack:
    //   metatable
    //   userdata
    DCHECK( C.stack_size() == initial_stack_size + 2 );
  }

  C.setmetatable( -2 );
  // Stack:
  //   userdata
  DCHECK( C.stack_size() == initial_stack_size + 1 );

  return metatable_created;
}

bool helper::push_stateful_lua_c_function(
    base::unique_func<int( lua_State* ) const> func ) noexcept {
  // Pushes new userdata onto stack.
  bool metatable_created = create_userdata( std::move( func ) );

  // 1. Create the closure with one upvalue (the userdata).
  static auto closure_caller = []( lua_State* L ) -> int {
    using Closure_t = remove_cvref_t<decltype( func )>;
    static string const type_name =
        base::demangled_typename<Closure_t>();
    // Use `testudata` instead of `checkudata` so that we throw a
    // C++ error instead of a Lua error.
    void* ud = luaL_testudata( L, lua_upvalueindex( 1 ),
                               type_name.c_str() );
    CHECK(
        ud != nullptr,
        "closure_caller expected type {} but did not find it.",
        type_name );
    Closure_t const& closure =
        *static_cast<Closure_t const*>( ud );
    return closure( L );
  };

  C.push( closure_caller, /*upvalues=*/1 );

  return metatable_created;
}

} // namespace lua
