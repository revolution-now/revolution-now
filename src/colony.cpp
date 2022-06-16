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

// Revolution Now
#include "commodity.hpp"
#include "error.hpp"
#include "gs-units.hpp"
#include "logger.hpp"
#include "lua.hpp"

// config
#include "config/colony.rds.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// luapp
#include "luapp/state.hpp"

// base
#include "base/keyval.hpp"
#include "base/scope-exit.hpp"
#include "base/string.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

// This should be called at the end of any non-const member func-
// tion that can edit the jobs/units maps.
void validate_job_maps( wrapped::Colony const& colony ) {
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
** e_colony_building
*****************************************************************/
LUA_ENUM( colony_building );

/****************************************************************
** e_indoor_job
*****************************************************************/
LUA_ENUM( indoor_job );

/****************************************************************
** wrapped::Colony
*****************************************************************/
// This function should only validate those things that don't
// need access to any game state outside of this one colony ob-
// ject. E.g., no validating things that require access to the
// terrain or to unit info; that stuff should go in the top-level
// validation functions.
valid_or<string> wrapped::Colony::validate() const {
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

/****************************************************************
** Colony
*****************************************************************/
int Colony::population() const {
  int size = 0;
  for( e_indoor_job job : refl::enum_values<e_indoor_job> )
    size += o_.indoor_jobs[job].size();
  for( e_direction d : refl::enum_values<e_direction> )
    size += o_.outdoor_jobs[d].has_value() ? 1 : 0;
  return size;
}

string Colony::debug_string() const {
  return fmt::format(
      "Colony{{name=\"{}\",id={},nation={},coord={},population={"
      "}}}",
      name(), id(), nation(), location(), population() );
}

void Colony::add_building( e_colony_building building ) {
  o_.buildings[building] = true;
}

void Colony::rm_building( e_colony_building building ) {
  o_.buildings[building] = false;
}

vector<UnitId> Colony::all_units() const {
  vector<UnitId> res;
  res.reserve( 40 );
  for( e_indoor_job job : refl::enum_values<e_indoor_job> )
    res.insert( res.end(), o_.indoor_jobs[job].begin(),
                o_.indoor_jobs[job].end() );
  for( e_direction d : refl::enum_values<e_direction> )
    if( maybe<OutdoorUnit> const& job = o_.outdoor_jobs[d];
        job.has_value() )
      res.push_back( job->unit_id );
  return res;
}

void Colony::add_unit( UnitId id, ColonyJob_t const& job ) {
  SCOPE_EXIT( CHECK( validate() ) );
  CHECK( !has_unit( id ), "Unit {} already in colony.", id );
  switch( job.to_enum() ) {
    case ColonyJob::e::indoor: {
      auto const& o = job.get<ColonyJob::indoor>();
      o_.indoor_jobs[o.job].push_back( id );
      break;
    }
    case ColonyJob::e::outdoor: {
      auto const&         o = job.get<ColonyJob::outdoor>();
      maybe<OutdoorUnit>& outdoor_unit =
          o_.outdoor_jobs[o.direction];
      CHECK( !outdoor_unit.has_value() );
      outdoor_unit = OutdoorUnit{ .unit_id = id, .job = o.job };
      break;
    }
  }
}

void Colony::remove_unit( UnitId id ) {
  SCOPE_EXIT( CHECK( validate() ) );

  for( auto& [job, units] : o_.indoor_jobs ) {
    if( find( units.begin(), units.end(), id ) != units.end() ) {
      units.erase( find( units.begin(), units.end(), id ) );
      return;
    }
  }

  for( auto& [direction, outdoor_unit] : o_.outdoor_jobs ) {
    if( outdoor_unit.has_value() &&
        outdoor_unit->unit_id == id ) {
      outdoor_unit = nothing;
      return;
    }
  }

  FATAL( "unit {} not found in colony.", id );
}

bool Colony::has_unit( UnitId id ) const {
  vector<UnitId> units = all_units();
  return find( units.begin(), units.end(), id ) != units.end();
}

void Colony::set_nation( e_nation new_nation ) {
  o_.nation = new_nation;
}

void Colony::add_hammers( int hammers ) {
  o_.hammers += hammers;
  CHECK_GE( o_.hammers, 0 );
}

} // namespace rn

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace rn {
namespace {

// CommodityQuantityMap
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::CommodityQuantityMap;
  auto u  = st.usertype.create<U>();

  u[lua::metatable_key]["__index"] =
      [&]( U& obj, e_commodity c ) { return obj[c]; };

  u[lua::metatable_key]["__newindex"] =
      [&]( U& obj, e_commodity c, int quantity ) {
        obj[c] = quantity;
      };

  // !! NOTE: because we overwrote the __index and __newindex
  // metamethods on this userdata we cannot add any further
  // (non-metatable) members on this object, since there will be
  // no way to look them up by name.
};

LUA_STARTUP( lua::state& st ) {
  using U = Colony;
  auto u  = st.usertype.create<U>();

  // Getters.
  u["id"]       = &U::id;
  u["nation"]   = &U::nation;
  u["name"]     = &U::name;
  u["location"] = &U::location;
  u["bells"]    = &U::bells;

  // Modifiers.
  u["add_building"] = &U::add_building;
  u["rm_building"]  = &U::rm_building;

  // TODO: this should be exposed to lua as a non-function prop-
  // erty.
  u["commodities"] = []( U& obj ) -> decltype( auto ) {
    return obj.commodities();
  };
};

} // namespace
} // namespace rn
