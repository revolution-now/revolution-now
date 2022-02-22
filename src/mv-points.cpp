/****************************************************************
**mv-points.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-02.
*
* Description: A type for representing movement points that will
*              ensure correct handling of atomsal points.
*
*****************************************************************/
#include "mv-points.hpp"

// {fmt}
#include "fmt/format.h"

// luapp
#include "luapp/as.hpp"
#include "luapp/metatable.hpp"
#include "luapp/state.hpp"

// Cdr
#include "cdr/converter.hpp"
#include "cdr/ext-builtin.hpp"

// C++ standard library
#include <unordered_set>

using namespace std;

namespace rn {

namespace {} // namespace

MovementPoints::MovementPoints( int integral, int atoms_arg ) {
  atoms = ( ( integral + ( atoms_arg / factor ) ) * factor ) +
          ( atoms_arg % factor );
}

void to_str( MovementPoints const& o, std::string& out,
             base::ADL_t ) {
  if( o.atoms % o.factor == 0 )
    out += fmt::format( "{}", o.atoms / o.factor );
  else
    out += fmt::format( "{}+{}/{}", o.atoms / o.factor,
                        o.atoms % o.factor,
                        MovementPoints::factor );
}

base::valid_or<string> MovementPoints::validate() const {
  RETURN_IF_FALSE( atoms >= 0,
                   "MovementPoints object has negative points" );
  return base::valid;
}

maybe<MovementPoints> lua_get( lua::cthread L, int idx,
                               lua::tag<MovementPoints> ) {
  auto st = lua::state::view( L );

  maybe<lua::table> maybe_t = lua::get<lua::table>( L, idx );
  if( !maybe_t.has_value() ) return nothing;
  lua::table& t = *maybe_t;
  if( t["atoms"] == lua::nil ) return nothing;
  MovementPoints mv_pts( lua::as<int>( t["atoms"] ) );
  return mv_pts;
}

void lua_push( lua::cthread L, MovementPoints mv_pts ) {
  auto st = lua::state::view( L );

  lua::table t = st.table.create();
  t["atoms"]   = mv_pts.atoms;

  // FIXME: setting these metatables each time is too slow.
  lua::table meta = st.table.create();

  meta["__tostring"] = []( MovementPoints o ) {
    return fmt::format( "MovementPoints{{atoms={}}}", o.atoms );
  };

  meta["__eq"] = []( MovementPoints const& l,
                     MovementPoints const& r ) {
    return l == r;
  };

  setmetatable( t, meta );
  lua::push( L, t );
}

/****************************************************************
** Cdr
*****************************************************************/
cdr::value to_canonical( cdr::converter&       conv,
                         MovementPoints const& o,
                         cdr::tag_t<MovementPoints> ) {
  cdr::table tbl;
  tbl["atoms"] = conv.to( o.atoms );
  return tbl;
}

// We allow two ways to represent a MovementPoints:
//
//   1. integer: this is interpreted as whole movement points.
//      This is more convenient for the unit config files, since
//      a unit's maximum movement points will always be a whole
//      number.
//   2. table: this allows specifying the atoms, which is
//      necessary when saving a game, at which point a unit may
//      only have a fraction of a movement left.
//
cdr::result<MovementPoints> from_canonical(
    cdr::converter& conv, cdr::value const& v,
    cdr::tag_t<MovementPoints> ) {
  if( v.holds<cdr::integer_type>() ) {
    // We have whole movement points.
    UNWRAP_RETURN( n, conv.from<int>( v ) );
    return MovementPoints( n );
  }
  // Assume table.
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  std::unordered_set<std::string> used_keys;
  UNWRAP_RETURN(
      n, conv.from_field<int>( tbl, "atoms", used_keys ) );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  MovementPoints res;
  res.atoms = n;
  return res;
}

} // namespace rn
