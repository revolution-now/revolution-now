/****************************************************************
**colony.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-12-15.
*
* Description: Data structure representing a Colony.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "colony-id.hpp"
#include "colony-mfg.hpp"
#include "commodity.hpp"
#include "error.hpp"
#include "expect.hpp"
#include "nation.hpp"
#include "unit-id.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

// refl
#include "refl/enum-map.hpp"

// Rds
#include "colony.rds.hpp"

// C++ standard library
#include <unordered_map>
#include <unordered_set>

namespace rn {

class Colony {
 public:
  // This is provided for the serialization framework; a
  // default-constructed object will likely not be valid.
  Colony() = default;

  bool operator==( Colony const& ) const = default;

  /************************* Getters ***************************/

  ColonyId           id() const { return o_.id; }
  e_nation           nation() const { return o_.nation; }
  std::string const& name() const { return o_.name; }
  Coord              location() const { return o_.location; }
  int                bells() const { return o_.bells; }
  int prod_hammers() const { return o_.prod_hammers; }
  int commodity_quantity( e_commodity commodity ) const;
  // These units will be in unspecified order (order may depend
  // on hash table iteration) so the caller should take care to
  // not depend on the ordering returned by this function.
  std::vector<UnitId>                          units() const;
  std::unordered_set<e_colony_building> const& buildings()
      const {
    return o_.buildings;
  }
  std::unordered_map<UnitId, ColonyJob_t> const& units_jobs()
      const {
    return o_.units;
  }

  /************************ Modifiers **************************/
  // NOTE: these modifiers do not enforce invariants!
  void add_building( e_colony_building building );
  void add_unit( UnitId id, ColonyJob_t const& job );
  void remove_unit( UnitId id );
  void set_commodity_quantity( e_commodity comm, int q );
  void set_nation( e_nation new_nation );

  refl::enum_map<e_commodity, int>& commodities() {
    return o_.commodities;
  }

  // Will strip the unit of any commodities (including inventory
  // and modifiers) and deposit them commodities into the colony.
  // The unit must be on the colony square otherwise this will
  // check fail.
  void strip_unit_commodities( UnitId unit_id );

  /************************ Functions **************************/
  // NOTE: these modifiers do not enforce invariants!
  int  population() const;
  bool has_unit( UnitId id ) const;

  std::string debug_string() const;

  // Implement refl::WrapsReflected.
  Colony( wrapped::Colony&& o ) : o_( std::move( o ) ) {}
  wrapped::Colony const&            refl() const { return o_; }
  static constexpr std::string_view refl_ns   = "rn";
  static constexpr std::string_view refl_name = "Colony";

 private:
  friend struct ColoniesState;

  wrapped::Colony o_;
};
NOTHROW_MOVE( Colony );

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS( ::rn::Colony, owned_by_cpp ){};
}
