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
#include "logger.hpp"
#include "lua.hpp"
#include "unit-composer.hpp"
#include "ustate.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// luapp
#include "luapp/state.hpp"

// base
#include "base/string.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

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

int Colony::commodity_quantity( e_commodity commodity ) const {
  auto res = o_.commodities.find( commodity );
  if( res == o_.commodities.end() ) return 0;
  return res->second;
}

vector<UnitId> Colony::units() const {
  vector<UnitId> units_working_in_colony;
  units_working_in_colony.reserve( units_jobs().size() );
  for( auto const& [unit_id, job] : units_jobs() ) {
    (void)job;
    units_working_in_colony.push_back( unit_id );
  }
  return units_working_in_colony;
}

string Colony::debug_string() const {
  return fmt::format(
      "Colony{{name=\"{}\",id={},nation={},coord={},population={"
      "}}}",
      name(), id(), nation(), location(), population() );
}

void Colony::add_building( e_colony_building building ) {
  CHECK( !o_.buildings.contains( building ),
         "Colony already has a {}.", building );
  o_.buildings.insert( building );
}

void Colony::add_unit( UnitId id, ColonyJob_t const& job ) {
  CHECK( !has_unit( id ), "Unit {} already in colony.", id );
  o_.units[id] = job;
}

void Colony::remove_unit( UnitId id ) {
  CHECK( has_unit( id ), "Unit {} is not in colony.", id );
  o_.units.erase( id );
}

void Colony::set_commodity_quantity( e_commodity comm, int q ) {
  CHECK( q >= 0, "Colony commodity quantities must be >= 0." );
  commodities()[comm] = q;
}

bool Colony::has_unit( UnitId id ) const {
  return o_.units.contains( id );
}

void Colony::set_nation( e_nation new_nation ) {
  o_.nation = new_nation;
}

void Colony::strip_unit_commodities( UnitId unit_id ) {
  UNWRAP_CHECK_MSG(
      coord, coord_for_unit( unit_id ),
      "unit must be on map to shed its commodities." );
  CHECK( coord == location(),
         "unit must be in colony to shed its commodities." );
  UnitTransformationResult tranform_res =
      unit_from_id( unit_id ).strip_to_base_type();
  for( auto [type, q] : tranform_res.commodity_deltas ) {
    CHECK( q > 0 );
    lg.debug( "adding {} {} to colony {}.", q, type, name() );
    o_.commodities[type] += q;
  }
}

} // namespace rn

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace rn {
namespace {

LUA_STARTUP( lua::state& st ) {
  auto colony = st.usertype.create<Colony>();

  // Getters.
  colony["id"]        = &Colony::id;
  colony["nation"]    = &Colony::nation;
  colony["name"]      = &Colony::name;
  colony["location"]  = &Colony::location;
  colony["sentiment"] = &Colony::sentiment;

  // Modifiers.
  colony["add_building"] = &Colony::add_building;
  colony["set_commodity_quantity"] =
      &Colony::set_commodity_quantity;
  colony["commodity_quantity"] = &Colony::commodity_quantity;
};

} // namespace
} // namespace rn
