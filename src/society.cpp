/****************************************************************
**society.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-04.
*
* Description: Abstraction for nations and tribes.
*
*****************************************************************/
#include "society.hpp"
#include "unit-mgr.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/natives.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/natives.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
maybe<Society> society_on_square( SSConst const& ss,
                                  Coord          coord ) {
  // Check for European colony.
  if( auto id = ss.colonies.maybe_from_coord( coord ); id ) {
    e_nation const nation = ss.colonies.colony_for( *id ).nation;
    return Society::european{ .nation = nation };
  }

  // Check for native dwelling.
  if( maybe<DwellingId> id =
          ss.natives.maybe_dwelling_from_coord( coord );
      id ) {
    e_tribe const tribe = ss.natives.tribe_for( *id ).type;
    return Society::native{ .tribe = tribe };
  }

  // Check for unit.
  auto& units = ss.units;

  unordered_set<GenericUnitId> const& on_coord =
      units.from_coord( coord );
  if( on_coord.empty() ) return nothing;
  GenericUnitId const id = *on_coord.begin();

  switch( units.unit_kind( id ) ) {
    case e_unit_kind::euro: {
      e_nation const nation =
          units.unit_for( units.check_euro_unit( id ) ).nation();
      return Society::european{ .nation = nation };
    }
    case e_unit_kind::native: {
      e_tribe const tribe = tribe_for_unit(
          ss, units.unit_for( units.check_native_unit( id ) ) );
      return Society::native{ .tribe = tribe };
    }
  }
}

gfx::pixel flag_color_for_society( Society const& society ) {
  switch( society.to_enum() ) {
    case Society::e::european: {
      auto const& o = society.get<Society::european>();
      return config_nation.nations[o.nation].flag_color;
    }
    case Society::e::native: {
      auto const& o = society.get<Society::native>();
      return config_natives.tribes[o.tribe].flag_color;
    }
  }
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( tribe_on_square, maybe<e_tribe>, Coord square ) {
  SSConst const& ss = st["SS"].as<SS&>();
  return society_on_square( ss, square )
      .bind( []( Society const& society ) {
        return society.get_if<Society::native>().member(
            &Society::native::tribe );
      } );
}

} // namespace

} // namespace rn
