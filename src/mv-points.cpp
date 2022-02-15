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

using namespace std;

namespace rn {

namespace {} // namespace

MovementPoints::MovementPoints( int integral, int atoms ) {
  points_atoms = ( ( integral + ( atoms / factor ) ) * factor ) +
                 ( atoms % factor );
}

void to_str( MovementPoints const& o, std::string& out,
             base::ADL_t ) {
  if( o.points_atoms % o.factor == 0 )
    out += fmt::format( "{:d}", o.points_atoms / o.factor );
  else
    out +=
        fmt::format( "{:d} {:d}/{:d}", o.points_atoms / o.factor,
                     o.points_atoms % o.factor, o.factor );
}

base::valid_or<string> MovementPoints::validate() const {
  RETURN_IF_FALSE( points_atoms >= 0,
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
  t["atoms"]   = mv_pts.points_atoms;

  // FIXME: setting these metatables each time is too slow.
  lua::table meta = st.table.create();

  meta["__tostring"] = []( MovementPoints o ) {
    return fmt::format( "MovementPoints{{atoms={}}}",
                        o.points_atoms );
  };

  meta["__eq"] = []( MovementPoints const& l,
                     MovementPoints const& r ) {
    return l == r;
  };

  setmetatable( t, meta );
  lua::push( L, t );
}

/****************************************************************
** Rcl
*****************************************************************/
rcl::convert_err<MovementPoints> convert_to(
    rcl::value const& v, rcl::tag<MovementPoints> ) {
  // TODO(migration): remove
  return rcl::via_cdr<MovementPoints>( v );
}

/****************************************************************
** Cdr
*****************************************************************/
cdr::value to_canonical( cdr::converter&       conv,
                         MovementPoints const& o,
                         cdr::tag_t<MovementPoints> ) {
  return conv.to( o.points_atoms );
}

cdr::result<MovementPoints> from_canonical(
    cdr::converter& conv, cdr::value const& v,
    cdr::tag_t<MovementPoints> ) {
  UNWRAP_RETURN( base_res, conv.from<int>( v ) );
  return MovementPoints( base_res );
}

} // namespace rn
