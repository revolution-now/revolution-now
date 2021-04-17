/****************************************************************
**nation.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-03.
*
* Description: Representation of nations.
*
*****************************************************************/
#include "nation.hpp"

// Revolution Now
#include "config-files.hpp"
#include "cstate.hpp"
#include "lua.hpp"
#include "ustate.hpp"

// Revolution Now (config)
#include "../config/ucl/nation.inl"

// C++ standard library
#include <cctype>

#define MAKE_NATION( __name )                               \
  m[e_nation::__name] = NationDesc {                        \
    TO_STRING( __name ), config_nation.__name.country_name, \
        config_nation.__name.adjective,                     \
        config_nation.__name.article,                       \
        config_nation.__name.flag_color                     \
  }

using namespace std;

namespace rn {

namespace {} // namespace

NationDesc const& nation_obj( e_nation nation ) {
  static unordered_map<e_nation, NationDesc> nations = [] {
    unordered_map<e_nation, NationDesc> m;
    MAKE_NATION( dutch );
    MAKE_NATION( french );
    MAKE_NATION( english );
    MAKE_NATION( spanish );
    CHECK( m.size() == enum_traits<e_nation>::count );
    return m;
  }();
  return nations[nation];
}

string NationDesc::name_proper() const {
  string res = name_lowercase;
  CHECK( !res.empty() );
  res[0] = std::toupper( res[0] );
  return res;
}

maybe<e_nation> nation_from_coord( Coord coord ) {
  if( auto maybe_colony_id = colony_from_coord( coord );
      maybe_colony_id )
    return colony_from_id( *maybe_colony_id ).nation();

  auto const& units = units_from_coord( coord );
  if( units.empty() ) return nothing;
  e_nation first = unit_from_id( *units.begin() ).nation();
  for( auto const& id : units ) {
    (void)id; // for release builds.
    DCHECK( first == unit_from_id( id ).nation() );
  }
  return first;
}

} // namespace rn

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_ENUM( nation );

}
