/****************************************************************
**unit-id.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-29.
*
* Description: Strongly-typed IDs for units.
*
*****************************************************************/
#include "unit-id.hpp"

// luapp
#include "luapp/types.hpp"

// cdr
#include "cdr/converter.hpp"
#include "cdr/ext-builtin.hpp"

// base
#include "base/error.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** UnitId
*****************************************************************/
void to_str( UnitId o, std::string& out, base::ADL_t tag ) {
  to_str( static_cast<int>( o ), out, tag );
}

cdr::value to_canonical( cdr::converter& conv, UnitId o,
                         cdr::tag_t<UnitId> ) {
  return to_canonical( conv, static_cast<int>( o ),
                       cdr::tag_t<int>{} );
}

cdr::result<UnitId> from_canonical( cdr::converter&   conv,
                                    cdr::value const& v,
                                    cdr::tag_t<UnitId> ) {
  UNWRAP_RETURN( int_res, conv.from<int>( v ) );
  return static_cast<UnitId>( int_res );
}

void lua_push( lua::cthread L, UnitId o ) {
  lua_push( L, static_cast<int>( o ) );
}

base::maybe<UnitId> lua_get( lua::cthread L, int idx,
                             lua::tag<UnitId> ) {
  UNWRAP_RETURN( int_res, lua::get<int>( L, idx ) );
  return static_cast<UnitId>( int_res );
}

} // namespace rn
