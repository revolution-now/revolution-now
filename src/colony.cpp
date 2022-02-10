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
#include "error.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "unit-composer.hpp"
#include "ustate.hpp"

// refl
#include "refl/to-str.hpp"

// luapp
#include "luapp/state.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/

int Colony::population() const { return units_.size(); }

int Colony::commodity_quantity( e_commodity commodity ) const {
  auto res = commodities_.find( commodity );
  if( res == commodities_.end() ) return 0;
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

void to_str( Colony const& o, string& out, base::ADL_t ) {
  out += fmt::format(
      "Colony{{name=\"{}\",id={},nation={},coord={},population={"
      "}}}",
      o.name(), o.id(), o.nation(), o.location(),
      o.population() );
}

void Colony::add_building( e_colony_building building ) {
  CHECK( !buildings_.contains( building ),
         "Colony already has a {}.", building );
  buildings_.insert( building );
}

void Colony::add_unit( UnitId id, ColonyJob_t const& job ) {
  CHECK( !has_unit( id ), "Unit {} already in colony.", id );
  units_[id] = job;
}

void Colony::remove_unit( UnitId id ) {
  CHECK( has_unit( id ), "Unit {} is not in colony.", id );
  units_.erase( id );
}

void Colony::set_commodity_quantity( e_commodity comm, int q ) {
  CHECK( q >= 0, "Colony commodity quantities must be >= 0." );
  commodities()[comm] = q;
}

bool Colony::has_unit( UnitId id ) const {
  return units_.contains( id );
}

void Colony::set_nation( e_nation new_nation ) {
  nation_ = new_nation;
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
    commodities_[type] += q;
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
