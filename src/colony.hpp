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
#include "colony-mfg.hpp"
#include "commodity.hpp"
#include "error.hpp"
#include "fb.hpp"
#include "fmt-helper.hpp"
#include "id.hpp"
#include "nation.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

// Rds
#include "rds/colony.hpp"

// Flatbuffers
#include "fb/colony_generated.h"

// C++ standard library
#include <unordered_map>
#include <unordered_set>

namespace rn {

class Colony {
 public:
  Colony()  = default; // for serialization framework.
  ~Colony() = default;
  Colony( Colony const& ) = delete;
  Colony( Colony&& )      = default;

  Colony& operator=( Colony const& ) = delete;
  Colony& operator=( Colony&& ) = default;

  bool operator==( Colony const& ) const = default;

  /************************* Getters ***************************/

  ColonyId           id() const { return id_; }
  e_nation           nation() const { return nation_; }
  std::string const& name() const { return name_; }
  Coord              location() const { return location_; }
  int                sentiment() const { return sentiment_; }
  int prod_hammers() const { return prod_hammers_; }
  int prod_tools() const { return prod_tools_; }
  int commodity_quantity( e_commodity commodity ) const;
  // These units will be in unspecified order (order may depend
  // on hash table iteration) so the caller should take care to
  // not depend on the ordering returned by this function.
  std::vector<UnitId>                          units() const;
  std::unordered_set<e_colony_building> const& buildings()
      const {
    return buildings_;
  }
  std::unordered_map<UnitId, ColonyJob_t> const& units_jobs()
      const {
    return units_;
  }

  /************************ Modifiers **************************/
  // NOTE: these modifiers do not enforce invariants!
  void add_building( e_colony_building building );
  void add_unit( UnitId id, ColonyJob_t const& job );
  void remove_unit( UnitId id );
  void set_commodity_quantity( e_commodity comm, int q );
  void set_nation( e_nation new_nation );

  std::unordered_map<e_commodity, int>& commodities() {
    return commodities_;
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

  std::string to_string() const;

  // This class itself is not equipped to check all of its own
  // invariants, since many of them require other game state to
  // validate.
  valid_deserial_t check_invariants_safe() const {
    return valid;
  }

 private:
  friend expect<ColonyId, std::string> cstate_create_colony(
      e_nation nation, Coord const& where,
      std::string_view name );

  using FlatMap_e_commodity_int =
      std::unordered_map<e_commodity, int>;
  using FlatMap_UnitId_ColonyJob_t =
      std::unordered_map<UnitId, ColonyJob_t>;
  using FlatSet_e_colony_building =
      std::unordered_set<e_colony_building>;

  // clang-format off
  SERIALIZABLE_TABLE_MEMBERS( fb, Colony,
  // Basic info.
  ( ColonyId,                   id_           ),
  ( e_nation,                   nation_       ),
  ( std::string,                name_         ),
  ( Coord,                      location_     ),

  // Commodities.
  ( FlatMap_e_commodity_int,    commodities_  ),

  // Serves to both record the units in this colony as well as
  // their occupations.
  ( FlatMap_UnitId_ColonyJob_t, units_        ),
  ( FlatSet_e_colony_building,  buildings_    ),

  // Production
  ( maybe<e_colony_building>,     production_   ),
  ( int,                        prod_hammers_ ),
  ( int,                        prod_tools_   ),

  // Liberty sentiment: [0,100].
  ( int,                        sentiment_    ));
  // clang-format on
};
NOTHROW_MOVE( Colony );

} // namespace rn

DEFINE_FORMAT( ::rn::Colony, "{}", o.to_string() );

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS( ::rn::Colony, owned_by_cpp ){};
}
