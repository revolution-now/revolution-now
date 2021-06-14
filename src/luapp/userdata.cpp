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
#include "error.hpp"

// C++ standard library
#include <unordered_map>

using namespace std;

namespace lua {

namespace {

void setup_new_metatable( cthread L, int metatable_idx,
                          LuaCFunction* fmt,
                          LuaCFunction* call_destructor ) {
  c_api C( L );
  CHECK( C.type_of( metatable_idx ) == type::table );

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
    C.setfield( metatable_idx - 1, name.c_str() );
  }
}

} // namespace

namespace detail {

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

void push_string( cthread L, std::string const& s ) {
  c_api C( L );
  C.push( s );
}

bool push_userdata_impl(
    cthread L, int object_size,
    base::function_ref<void( void* )> placement_new,
    LuaCFunction* fmt, LuaCFunction* call_destructor,
    std::string const& type_name ) {
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
    //
    // Stack:
    //   metatable
    //   userdata
    setup_new_metatable( L, /*metatable_idx=*/-1, fmt,
                         call_destructor );
  }

  C.setmetatable( -2 );
  // Stack:
  //   userdata
  CHECK( C.stack_size() == initial_stack_size + 1 );

  return metatable_created;
}

} // namespace detail

} // namespace lua
