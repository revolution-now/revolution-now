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
#include "ustate.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/natives.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/natives.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
maybe<Society_t> society_on_square( SSConst const& ss,
                                    Coord          coord ) {
  // Check for European colony.
  if( auto id = ss.colonies.maybe_from_coord( coord ); id ) {
    e_nation const nation = ss.colonies.colony_for( *id ).nation;
    return Society::european{ .nation = nation };
  }

  // Check for native dwelling.
  if( auto id = ss.natives.maybe_dwelling_from_coord( coord );
      id ) {
    e_tribe const tribe = ss.natives.dwelling_for( *id ).tribe;
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

gfx::pixel flag_color_for_society( Society_t const& society ) {
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

} // namespace rn
