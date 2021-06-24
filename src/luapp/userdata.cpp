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
#include "types.hpp"

// base
#include "base/error.hpp"

// C++ standard library
#include <unordered_map>

using namespace std;

namespace lua {

namespace {

using ::base::maybe;
using ::base::nothing;

void build_index_table( cthread L ) {
  c_api C( L );
  // Stack:
  //   metatable

  // Push the metatable as an upvalue.
  C.pushvalue( -1 );
  // Stack:
  //   metatable
  //   metatable
  auto index = []( lua_State* st ) -> int {
    c_api C( st );
    DCHECK( C.stack_size() == 2 );
    if( C.type_of( -1 ) != type::string ) return 0;
    // The key is a string.

    /************************************************************
    ** See if key is in metatable.
    *************************************************************/
    // FIXME: find a better way to handle these special members.
    C.pushvalue( upvalue_index( 1 ) ); // userdata metatable.
    // Stack:
    //   metatable
    //   key
    //   userdata
    C.pushvalue( -2 );
    // Stack:
    //   key
    //   metatable
    //   key
    //   userdata
    C.gettable( -2 );
    // Stack:
    //   value
    //   metatable
    //   key
    //   userdata
    if( C.type_of( -1 ) != type::nil ) return 1;
    // Stack:
    //   nil
    //   metatable
    //   key
    //   userdata
    C.pop();

    /************************************************************
    ** Key is not in metatable, check member_types.
    *************************************************************/
    // Stack:
    //   metatable
    //   key
    //   userdata

    C.getfield( -1, "member_types" );
    DCHECK( C.stack_size() == 4 );
    DCHECK( C.type_of( -1 ) == type::table );
    // Stack:
    //   member types table
    //   metatable
    //   key
    //   userdata
    C.pushvalue( -3 );
    DCHECK( C.stack_size() == 5 );
    // Stack:
    //   key
    //   member types table
    //   metatable
    //   key
    //   userdata
    C.gettable( -2 );
    // Stack:
    //   member type
    //   member types table
    //   metatable
    //   key
    //   userdata
    DCHECK( C.stack_size() == 5 );
    // If this is nil, then the field just doesn't exist.
    if( C.type_of( -1 ) == type::nil ) return 0;
    // The key exists.
    CHECK( C.type_of( -1 ) == type::boolean );

    /************************************************************
    ** Key is in the member_types table.  Get member type.
    *************************************************************/
    // Stack:
    //   member type
    //   member types table
    //   metatable
    //   key
    //   userdata
    bool is_member_function = get_or_luaerr<bool>( st, -1 );
    C.pop( 2 );
    DCHECK( C.stack_size() == 3 );
    DCHECK( C.type_of( -1 ) == type::table );

    /************************************************************
    ** Get the associated function from the member_getters table.
    *************************************************************/
    // This is the function that is called with the userdata ob-
    // ject as the first argument to either get the variable
    // value (if this is a member variable) or to call the member
    // function (if it's a function).

    // Stack:
    //   metatable
    //   key
    //   userdata
    C.getfield( -1, "member_getters" );
    DCHECK( C.stack_size() == 4 );
    DCHECK( C.type_of( -1 ) == type::table );
    // Stack:
    //   member_getters table
    //   metatable
    //   key
    //   userdata
    C.pushvalue( -3 );
    DCHECK( C.stack_size() == 5 );
    // Stack:
    //   key
    //   member_getters table
    //   metatable
    //   key
    //   userdata
    C.gettable( -2 );
    // Stack:
    //   function
    //   member_getters table
    //   metatable
    //   key
    //   userdata
    DCHECK( C.stack_size() == 5 );
    CHECK( C.type_of( -1 ) == type::function );
    // Stack:
    //   function
    //   member_getters table
    //   metatable
    //   key
    //   userdata

    /************************************************************
    ** Run the function (for vars) or return it (for functions).
    *************************************************************/
    if( !is_member_function ) {
      // We have a member variable.
      C.pushvalue( 1 );
      DCHECK( C.stack_size() == 6 );
      // Stack:
      //   userdata
      //   function
      //   member_getters table
      //   metatable
      //   key
      //   userdata
      C.call( /*nargs=*/1, /*nresults=*/1 );
      DCHECK( C.stack_size() == 5 );
      // Stack:
      //   member variable value
      //   member_getters table
      //   metatable
      //   key
      //   userdata
      return 1;
    } else {
      // We have a member function.
      DCHECK( C.stack_size() == 5 );
      DCHECK( C.type_of( -1 ) == type::function );
      // Stack:
      //   function
      //   member_getters table
      //   metatable
      //   key
      //   userdata
      return 1;
    }
  };
  C.push( index, /*nupvalues=*/1 );
  // Stack:
  //   __index function
  //   metatable
}

void build_newindex_table( cthread L ) {
  c_api C( L );
  // Stack:
  //   metatable

  // Push the metatable as an upvalue.
  C.pushvalue( -1 );
  // Stack:
  //   metatable
  //   metatable
  auto newindex = []( lua_State* st ) -> int {
    c_api C( st );
    DCHECK( C.stack_size() == 3 );
    // Stack:
    //   newval
    //   key
    //   userdata
    if( C.type_of( -2 ) != type::string ) return 0;
    UNWRAP_CHECK( key, C.get<string>( -2 ) );
    // The key is a string.

    /************************************************************
    ** Check member_setters.
    *************************************************************/
    C.pushvalue( upvalue_index( 1 ) ); // userdata metatable.
    // Stack:
    //   metatable
    //   newval
    //   key
    //   userdata

    C.getfield( -1, "member_setters" );
    DCHECK( C.stack_size() == 5 );
    DCHECK( C.type_of( -1 ) == type::table );
    // Stack:
    //   member setters table
    //   metatable
    //   newval
    //   key
    //   userdata
    C.pushvalue( -4 ); // key
    DCHECK( C.stack_size() == 6 );
    // Stack:
    //   key
    //   member setters table
    //   metatable
    //   newval
    //   key
    //   userdata
    C.gettable( -2 );
    // Stack:
    //   member setter
    //   member setters table
    //   metatable
    //   newval
    //   key
    //   userdata
    DCHECK( C.stack_size() == 6 );
    // If this is nil, then the field just doesn't exist.
    if( C.type_of( -1 ) == type::nil )
      throw_lua_error(
          st, "attempt to set nonexistent field `{}'.", key );
    // The key exists.
    CHECK( C.type_of( -1 ) == type::function );

    /************************************************************
    ** Call the function to set the value.
    *************************************************************/
    // Stack:
    //   member setter
    //   member setters table
    //   metatable
    //   newval
    //   key
    //   userdata

    C.rotate( -6, 1 );
    DCHECK( C.stack_size() == 6 );
    // Stack:
    //   member setters table
    //   metatable
    //   newval
    //   key
    //   userdata
    //   member setter

    C.pop( 2 );
    DCHECK( C.stack_size() == 4 );
    // Stack:
    //   newval
    //   key
    //   userdata
    //   member setter

    C.swap_top();
    C.pop();
    DCHECK( C.stack_size() == 3 );
    // Stack:
    //   newval
    //   userdata
    //   member setter

    C.call( /*nargs=*/2, /*nresults=*/0 );
    DCHECK( C.stack_size() == 0 );
    // Stack:

    return 0;
  };
  C.push( newindex, /*nupvalues=*/1 );
  // Stack:
  //   __newindex function
  //   metatable
}

void setup_special_members(
    cthread L, e_userdata_ownership_model semantics ) {
  // Stack:
  //   metatable
  c_api C( L );
  bool  is_owned_by_lua = false;
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
  //   metatable
  C.setfield( -2, "is_owned_by_lua" );
  // Stack:
  //   metatable
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

  setup_special_members( L, semantics );

  build_index_table( L );
  // Stack:
  //   __index function
  //   metatable
  CHECK( C.type_of( -1 ) == type::function );
  C.setfield( -2, "__index" );

  build_newindex_table( L );
  // Stack:
  //   __newindex function
  //   metatable
  CHECK( C.type_of( -1 ) == type::function );
  C.setfield( -2, "__newindex" );

  // Build member type table. This is a table that will have one
  // boolean entry per member function or variable and will tell
  // whether it is a member function or a member variable.
  C.newtable();
  // Stack:
  //   member type table
  //   metatable
  C.setfield( -2, "member_types" );
  // Stack:
  //   metatable

  // Build member setters table. This is a table that will have
  // one entry per non-const member variable to allow the user
  // set them.
  C.newtable();
  // Stack:
  //   member setters table
  //   metatable
  C.setfield( -2, "member_setters" );
  // Stack:
  //   metatable

  // Build member_getters table. This is a table that will have
  // one entry per member function or variable (whose values will
  // be functions).
  C.newtable();
  // Stack:
  //   member_getters table
  //   metatable
  C.setfield( -2, "member_getters" );
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
