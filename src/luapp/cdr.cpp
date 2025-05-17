/****************************************************************
**cdr.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-17.
*
* Description: Converts between CDR and Lua data structures.
*
*****************************************************************/
#include "cdr.hpp"

// luapp
#include "rtable.hpp"
#include "state.hpp"

using namespace std;

namespace lua {

/****************************************************************
** LuaToCdrConverter
*****************************************************************/
LuaToCdrConverter::LuaToCdrConverter( state& st ) : st_( st ) {}

any LuaToCdrConverter::convert( cdr::value const& o ) const {
  auto const visitor = [this]( auto const& v ) -> any {
    return this->convert( v );
  };
  return o.visit( visitor );
}

table LuaToCdrConverter::convert( cdr::table const& o ) const {
  table tbl = st_.table.create();
  for( auto const& [k, v] : o ) tbl[k] = convert( v );
  return tbl;
}

table LuaToCdrConverter::convert( cdr::list const& o ) const {
  table tbl = st_.table.create();
  for( int i = 1; auto const& v : o ) tbl[i++] = convert( v );
  return tbl;
}

rstring LuaToCdrConverter::convert(
    std::string const& o ) const {
  return st_.string.create( o );
}

any LuaToCdrConverter::convert(
    cdr::integer_type const o ) const {
  return st_.as<any>( o );
}

any LuaToCdrConverter::convert( double const o ) const {
  return st_.as<any>( o );
}

any LuaToCdrConverter::convert( bool const o ) const {
  return st_.as<any>( o );
}

any LuaToCdrConverter::convert( cdr::null_t const& ) const {
  return st_.as<any>( lua::nil );
}

} // namespace lua
