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
#include "errors.hpp"
#include "lua.hpp"

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

string Colony::to_string() const {
  return fmt::format(
      "Colony{{name=\"{}\",id={},nation={},coord={},population={"
      "}}}",
      name(), id(), nation(), location(), population() );
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

} // namespace rn

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace rn {
namespace {

LUA_MODULE()

LUA_STARTUP( sol::state& st ) {
  sol::usertype<Colony> colony =
      st.new_usertype<Colony>( "Colony", sol::no_constructor );

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
};

} // namespace
} // namespace rn
