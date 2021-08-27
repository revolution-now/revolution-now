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
#include "luapp/cast.hpp"
#include "luapp/metatable.hpp"
#include "luapp/state.hpp"

namespace rn {

namespace {} // namespace

MovementPoints::MovementPoints( int integral, int atoms ) {
  points_atoms = ( ( integral + ( atoms / factor ) ) * factor ) +
                 ( atoms % factor );
}

std::string MovementPoints::to_string() const {
  if( points_atoms % factor == 0 )
    return fmt::format( "{:d}", points_atoms / factor );
  else
    return fmt::format( "{:d} {:d}/{:d}", points_atoms / factor,
                        points_atoms % factor, factor );
}

valid_deserial_t MovementPoints::check_invariants_safe() const {
  if( points_atoms < 0 )
    return invalid_deserial(
        "MovementPoints object has negative points" );
  return valid;
}

maybe<MovementPoints> lua_get( lua::cthread L, int idx,
                               lua::tag<MovementPoints> ) {
  auto st = lua::state::view( L );

  maybe<lua::table> maybe_t = lua::get<lua::table>( L, idx );
  if( !maybe_t.has_value() ) return nothing;
  lua::table& t = *maybe_t;
  if( t["atoms"] == lua::nil ) return nothing;
  MovementPoints mv_pts( lua::cast<int>( t["atoms"] ) );
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
  base::maybe<int const&> i = v.get_if<int>();
  if( !i.has_value() )
    return rcl::error( fmt::format(
        "cannot convert value of type {} to MovementPoints.",
        name_of( type_of( v ) ) ) );
  return MovementPoints( *i );
}

} // namespace rn
