/****************************************************************
**colony.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-12-15.
*
* Description: Data structure representing a Colony.
*
*****************************************************************/
#include "colony.hpp"

// gs
#include "commodity.rds.hpp"

// ss
#include "ss/sons-of-liberty.hpp"
#include "ss/units.hpp"

// config
#include "config/colony.rds.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/register.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// luapp
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// base
#include "base/error.hpp"
#include "base/keyval.hpp"
#include "base/scope-exit.hpp"
#include "base/string.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

// This should be called at the end of any non-const member func-
// tion that can edit the jobs/units maps.
void validate_job_maps( Colony const& colony ) {
  unordered_set<UnitId> all;

  // First compile a list of all unit IDs mentioned.
  for( auto const& [job, units] : colony.indoor_jobs )
    all.insert( units.begin(), units.end() );
  for( auto const& [direction, outdoor_unit] :
       colony.outdoor_jobs )
    if( outdoor_unit.has_value() )
      all.insert( outdoor_unit->unit_id );

  // Now for each unit ID, make sure that it is in precisely one
  // of the jobs-to-units maps.

  for( UnitId id : all ) {
    bool indoor_jobs_has = false;
    for( auto const& [job, units] : colony.indoor_jobs ) {
      CHECK_LE( int( units.size() ), 3 );
      if( find( units.begin(), units.end(), id ) !=
          units.end() ) {
        CHECK( !indoor_jobs_has,
               "unit id {} appears multiple times in the indoor "
               "jobs lists.",
               id );
        indoor_jobs_has = true;
      }
    }
    bool outdoor_jobs_has = false;
    for( auto const& [direction, outdoor_unit] :
         colony.outdoor_jobs ) {
      if( outdoor_unit.has_value() &&
          outdoor_unit->unit_id == id ) {
        CHECK( !indoor_jobs_has,
               "unit id {} appears in both the indoor and "
               "outdoor jobs lists.",
               id );
        CHECK( !outdoor_jobs_has,
               "unit id {} appears multiple times in the out "
               "door jobs list.",
               id );
        outdoor_jobs_has = true;
      }
    }
  }
}

} // namespace

/****************************************************************
** Colony
*****************************************************************/
// This function should only validate those things that don't
// need access to any game state outside of this one colony ob-
// ject. E.g., no validating things that require access to the
// terrain or to unit info; that stuff should go in the top-level
// validation functions.
valid_or<string> Colony::validate() const {
  validate_job_maps( *this );

  // Colony has non-empty stripped name.
  REFL_VALIDATE( !base::trim( name ).empty(),
                 "Colony {}'s name is empty (when stripped).",
                 id );

  // Colony has at least one colonist.
  // NOTE: we don't validate this because a colony can be in this
  // state transitively while moving units. Also, there isn't
  // anything actually inconsistent with a colony not having any
  // colonists -- it's just not allowed by the game as a matter
  // of policy.  A modded game might allow it.

  // Colony's commodity quantites are in correct range.
  for( auto comm : refl::enum_values<e_commodity> ) {
    REFL_VALIDATE( commodities[comm] >= 0,
                   "Colony '{}' has a negative quantity of {}.",
                   name, comm );
  }

  // Each indoor building has a valid number of workers.
  for( auto const& [indoor_job, units] : indoor_jobs ) {
    REFL_VALIDATE(
        int( units.size() ) <=
            config_colony.max_workers_per_building,
        "Colony '{}' has {} colonists assigned to produce {} "
        "but a maximum of {} are allowed.",
        name, units.size(), indoor_job,
        config_colony.max_workers_per_building );
  }

  // Make sure that the list of teachers is in sync between the
  // teacher's map and the indoor_jobs map.
  REFL_VALIDATE( indoor_jobs[e_indoor_job::teacher].size() ==
                     teachers.size(),
                 "inconsistent list of teachers between the "
                 "indoor_jobs map and the teachers map." );
  for( UnitId unit_id : indoor_jobs[e_indoor_job::teacher] ) {
    REFL_VALIDATE(
        teachers.contains( unit_id ),
        "teachers list does not contain expected unit id {}.",
        unit_id );
  }

  // All colony's units can occupy a colony.
  // TODO: to do this I think we just check that each unit is a
  // human and a base type.

  // Colony's building set is self-consistent.
  // TODO

  // 10. Unit mfg jobs are self-consistent.
  // TODO

  // 11. Unit land jobs are good.
  // TODO

  // 12. Colony's production status is self-consistent.
  // TODO
  return valid;
}

vector<UnitId> colony_units_all( Colony const& colony ) {
  vector<UnitId> res;
  res.reserve( 40 );
  for( e_indoor_job job : refl::enum_values<e_indoor_job> )
    res.insert( res.end(), colony.indoor_jobs[job].begin(),
                colony.indoor_jobs[job].end() );
  for( e_direction d : refl::enum_values<e_direction> )
    if( maybe<OutdoorUnit> const& job = colony.outdoor_jobs[d];
        job.has_value() )
      res.push_back( job->unit_id );
  return res;
}

} // namespace rn

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace rn {
namespace {

LUA_STARTUP( lua::state& st ) {
  // CommodityQuantityMap
  // TODO: make this generic.
  [&] {
    using U = ::rn::CommodityQuantityMap;
    auto u  = st.usertype.create<U>();

    u[lua::metatable_key]["__index"] =
        [&]( U& obj, e_commodity c ) { return obj[c]; };

    u[lua::metatable_key]["__newindex"] =
        [&]( U& obj, e_commodity c, int quantity ) {
          obj[c] = quantity;
        };
  }();

  // CustomHouseMap.
  // TODO: make this generic.
  [&] {
    using U = ::rn::CustomHouseMap;
    auto u  = st.usertype.create<U>();

    u[lua::metatable_key]["__index"] =
        [&]( U& obj, e_commodity c ) { return obj[c]; };

    u[lua::metatable_key]["__newindex"] =
        [&]( U& obj, e_commodity c, int quantity ) {
          obj[c] = quantity;
        };
  }();

  // ColonyBuildingsMap
  // TODO: make this generic.
  [&] {
    using U = ::rn::ColonyBuildingsMap;
    auto u  = st.usertype.create<U>();

    u[lua::metatable_key]["__index"] =
        [&]( U& obj, e_colony_building c ) { return obj[c]; };

    u[lua::metatable_key]["__newindex"] =
        [&]( U& obj, e_colony_building c, bool b ) {
          obj[c] = b;
        };
  }();

  // !! NOTE: because we overwrote the __index and __newindex
  // metamethods on this userdata we cannot add any further
  // (non-metatable) members on this object, since there will be
  // no way to look them up by name.
};

LUA_STARTUP( lua::state& st ) {
  using U = Colony;
  auto u  = st.usertype.create<U>();

  // Getters.
  u["id"]              = &U::id;
  u["nation"]          = &U::nation;
  u["name"]            = &U::name;
  u["location"]        = &U::location;
  u["sons_of_liberty"] = &U::sons_of_liberty;
  u["buildings"]       = &U::buildings;
  u["commodities"]     = &U::commodities;
  u["custom_house"]    = &U::custom_house;
  // FIXME: figure out how to expose C++ maps to Lua.
  u["teachers"] = &U::teachers;
};

} // namespace
} // namespace rn
