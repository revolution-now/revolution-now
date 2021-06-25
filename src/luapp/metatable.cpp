/****************************************************************
**metatable.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-24.
*
* Description: For getting metatables of things.
*
*****************************************************************/
#include "metatable.hpp"

// luapp
#include "c-api.hpp"

using namespace std;

namespace lua {

namespace detail {

int get_stack_top( cthread L ) {
  c_api C( L );
  return C.gettop();
}

base::maybe<table> try_get_metatable(
    cthread L, int restore_stack_top_to ) {
  c_api C( L );
  C.getmetatable( -1 );
  if( C.type_of( -1 ) == type::nil ) {
    C.settop( restore_stack_top_to );
    return base::nothing;
  }

  table metatable( L, C.ref_registry() );
  C.settop( restore_stack_top_to );
  return metatable;
}

} // namespace detail

void setmetatable( table tbl, table meta ) {
  cthread L = tbl.this_cthread();
  c_api   C( L );
  int     start = C.stack_size();
  lua::push( L, tbl );
  lua::push( L, meta );
  C.setmetatable( -2 );
  C.pop();
  CHECK( C.stack_size() == start );
}

} // namespace lua
