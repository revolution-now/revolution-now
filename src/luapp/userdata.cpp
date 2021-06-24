/****************************************************************
**userdata.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-14.
*
* Description: Stuff for working with userdata.
*
*****************************************************************/
#include "userdata.hpp"

// luapp
#include "c-api.hpp"

// base
#include "base/error.hpp"

// C++ standard library
#include <unordered_map>

using namespace std;

namespace lua {

namespace {

using ::base::maybe;
using ::base::nothing;

void build_index_table( cthread                    L,
                        e_userdata_ownership_model semantics ) {
  c_api C( L );
  C.newtable();
  // Stack:
  //   __index table
  bool is_owned_by_lua = false;
  switch( semantics ) {
    case e_userdata_ownership_model::owned_by_cpp:
      is_owned_by_lua = false;
      break;
    case e_userdata_ownership_model::owned_by_lua:
      is_owned_by_lua = true;
      break;
  }
  C.push( is_owned_by_lua );
  // Stack:
  //   is_owned_by_lua
  //   __index table
  C.setfield( -2, "is_owned_by_lua" );
  // Stack:
  //   __index table
}

// Expects metatable to be at the top of the stack.
void setup_new_metatable( cthread                    L,
                          e_userdata_ownership_model semantics,
                          LuaCFunction*              fmt,
                          LuaCFunction* call_destructor ) {
  c_api C( L );
  // Check metatable.
  CHECK( C.type_of( -1 ) == type::table );

  // We will only use those values where the function pointer is
  // not nullptr, so we can freely put them all in this list.
  initializer_list<pair<string, LuaCFunction*>> metatable{
      { "__gc", call_destructor },
      { "__tostring", fmt },
  };

  for( auto& [name, func] : metatable ) {
    if( func == nullptr ) continue;
    // Stack:
    //   metatable
    C.push( func );
    // Stack:
    //   c function
    //   metatable
    C.setfield( -2, name.c_str() );
  }
  // Stack:
  //   metatable

  build_index_table( L, semantics );
  // Stack:
  //   __index table
  //   metatable
  CHECK( C.type_of( -1 ) == type::table );
  C.setfield( -2, "__index" );
  // Stack:
  //   metatable
}

} // namespace

namespace detail {

void push_string( cthread L, std::string const& s ) {
  c_api C( L );
  C.push( s );
}

void push_existing_userdata_metatable_impl(
    cthread L, std::string const& type_name ) {
  c_api C( L );
  bool  metatable_created =
      C.udata_newmetatable( type_name.c_str() );
  // Stack:
  //   metatable
  CHECK( !metatable_created,
         "attempt to get userdata metatable for type {} before "
         "it was registered.",
         type_name );
}

void push_userdata_impl(
    cthread L, int object_size,
    base::function_ref<void( void* )> placement_new,
    std::string const&                type_name ) {
  c_api C( L );
  int   initial_stack_size = C.stack_size();

  // 1. Create userdata to hold unique_func.
  void* ud = C.newuserdata( object_size );
  // Stack:
  //   userdata
  DCHECK( C.stack_size() == initial_stack_size + 1 );

  // The ud pointer, since it is allocated by malloc, will appar-
  // ently have the correct alignment to store any type. This
  // callback should move the object into the allocated storage.
  placement_new( ud );

  // 2. Get the metatable for this userdata type and set it.
  push_existing_userdata_metatable_impl( L, type_name );
  // Stack:
  //   metatable
  //   userdata
  DCHECK( C.stack_size() == initial_stack_size + 2 );

  C.setmetatable( -2 );
  // Stack:
  //   userdata
  CHECK( C.stack_size() == initial_stack_size + 1 );
}

bool register_userdata_metatable_if_needed_impl(
    cthread L, e_userdata_ownership_model semantics,
    LuaCFunction* fmt, LuaCFunction* call_destructor,
    string const& type_name ) {
  c_api C( L );
  // Get or create metatable for userdata type.
  bool metatable_created =
      C.udata_newmetatable( type_name.c_str() );
  // Stack:
  //   metatable
  if( metatable_created )
    // This is a newly-created metatable.
    setup_new_metatable( L, semantics, fmt, call_destructor );
  C.pop();
  return metatable_created;
}

} // namespace detail

void* check_udata( cthread L, int idx, char const* name ) {
  c_api C( L );
  // Use `testudata` instead of `checkudata` so that we throw a
  // C++ error instead of a Lua error.
  void* ud = C.testudata( idx, name );
  DCHECK( ud != nullptr,
          "__gc method expected type {} but did not find it.",
          name );
  return ud;
}

base::maybe<void*> try_udata( cthread L, int idx,
                              char const* name ) {
  c_api C( L );
  // Use `testudata` instead of `checkudata` so that we dont'
  // throw a Lua error if it fails.
  void* ud = C.testudata( idx, name );
  if( ud == nullptr ) return base::nothing;
  return ud;
}

} // namespace lua
