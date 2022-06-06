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

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// luapp
#include "luapp/state.hpp"

// base
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
  for( auto const& [id, job] : colony.units ) all.insert( id );
  for( auto const& [job, units] : colony.indoor_jobs )
    all.insert( units.begin(), units.end() );
  for( auto const& [direction, outdoor_unit] :
       colony.outdoor_jobs )
    if( outdoor_unit.has_value() )
      all.insert( outdoor_unit->unit_id );

  // Now for each unit ID, make sure that it is in both the
  // unit-to-jobs map and precisely one of the jobs-to-units
  // maps.

  for( UnitId id : all ) {
    bool units_has       = colony.units.contains( id );
    bool indoor_jobs_has = false;
    for( auto const& [job, units] : colony.indoor_jobs ) {
      CHECK_LE( int( units.size() ), 3 );
      if( units.contains( id ) ) {
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

    bool jobs_has = indoor_jobs_has || outdoor_jobs_has;

    CHECK( units_has == jobs_has,
           "colony (id={}) is in an inconsistent state with "
           "regard to unit {}.",
           colony.id, id );
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
** Colony
*****************************************************************/

// This function should only validate those things that don't
// need access to any game state outside of this one colony ob-
// ject. E.g., no validating things that require access to the
// terrain or to unit info; that stuff should go in the top-level
// validation functions.
valid_or<string> wrapped::Colony::validate() const {
  // Colony has non-empty stripped name.
  REFL_VALIDATE( !base::trim( name ).empty(),
                 "Colony name is empty (when stripped)." );

  // Colony has at least one colonist.
  REFL_VALIDATE( units.size() > 0, "Colony {} has no units.",
                 id );

  // Colony's commodity quantites in correct range.
  for( auto comm : refl::enum_values<e_commodity> ) {
    REFL_VALIDATE( commodities[comm] >= 0,
                   "Colony {} has a negative quantity of {}.",
                   id, comm );
  }

  validate_job_maps( *this );

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

int Colony::population() const { return o_.units.size(); }

string Colony::debug_string() const {
  return fmt::format(
      "Colony{{name=\"{}\",id={},nation={},coord={},population={"
      "}}}",
      name(), id(), nation(), location(), population() );
}

void Colony::add_building( e_colony_building building ) {
  o_.buildings.insert( building );
}

void Colony::add_unit( UnitId id, ColonyJob_t const& job ) {
  SCOPE_EXIT( validate_job_maps( o_ ) );
  CHECK( !has_unit( id ), "Unit {} already in colony.", id );
  o_.units[id] = job;
  switch( job.to_enum() ) {
    case ColonyJob::e::indoor: {
      auto const& o = job.get<ColonyJob::indoor>();
      o_.indoor_jobs[o.job].insert( id );
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
  SCOPE_EXIT( validate_job_maps( o_ ) );
  CHECK( has_unit( id ), "Unit {} is not in colony.", id );
  o_.units.erase( id );

  for( auto& [job, units] : o_.indoor_jobs ) {
    if( units.contains( id ) ) {
      units.erase( id );
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

  SHOULD_NOT_BE_HERE;
}

bool Colony::has_unit( UnitId id ) const {
  return o_.units.contains( id );
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

  // TODO: this should be exposed to lua as a non-function prop-
  // erty.
  u["commodities"] = []( U& obj ) -> decltype( auto ) {
    return obj.commodities();
  };
};

} // namespace
} // namespace rn
