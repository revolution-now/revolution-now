/****************************************************************
**usertype.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-24.
*
* Description: For defining members of userdata types.
*
*****************************************************************/
#include "usertype.hpp"

// luapp
#include "c-api.hpp"

using namespace std;

namespace lua::detail {

// Expects stack:
//   function
//   metatable
//
// Leaves stack empty.
void usertype_set_member_getter( cthread L, char const* name,
                                 bool is_function ) {
  c_api C( L );
  // Stack:
  //   function
  //   metatable
  CHECK( C.type_of( -2 ) == type::table );
  CHECK( C.type_of( -1 ) == type::function );

  CHECK( C.getfield( -2, "member_types" ) == type::table );
  // Stack:
  //   member_types table
  //   function
  //   metatable

  C.push( is_function );
  // Stack:
  //   is_function
  //   member_types table
  //   function
  //   metatable

  C.setfield( -2, name );
  C.pop();
  // Stack:
  //   function
  //   metatable

  CHECK( C.getfield( -2, "member_getters" ) == type::table );
  // Stack:
  //   member_getters table
  //   function
  //   metatable

  C.swap_top();
  // Stack:
  //   function
  //   member_getters table
  //   metatable
  C.setfield( -2, name );
  C.pop();
  // Stack:
  //   metatable

  C.pop();
}

// This one is only for member variables and not member func-
// tions.
//
// Expects stack:
//   function
//   metatable
//
// Leaves stack empty.
void usertype_set_member_setter( cthread L, char const* name ) {
  c_api C( L );
  // Stack:
  //   function
  //   metatable
  CHECK( C.type_of( -2 ) == type::table );
  CHECK( C.type_of( -1 ) == type::function );

  CHECK( C.getfield( -2, "member_setters" ) == type::table );
  // Stack:
  //   member_setters table
  //   function
  //   metatable

  C.swap_top();
  // Stack:
  //   function
  //   member_setters table
  //   metatable
  C.setfield( -2, name );
  // Stack:
  //   member_setters table
  //   metatable
  C.pop();
  // Stack:
  //   metatable

  C.pop();
}

} // namespace lua::detail
