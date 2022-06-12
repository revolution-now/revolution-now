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
#include "commodity.hpp"
#include "error.hpp"
#include "expect.hpp"
#include "lua-enum.hpp"
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

using CommodityQuantityMap = refl::enum_map<e_commodity, int>;

/****************************************************************
** Fwd Decls
*****************************************************************/
struct UnitsState;

/****************************************************************
** e_colony_building
*****************************************************************/
LUA_ENUM_DECL( colony_building );

/****************************************************************
** e_indoor_job
*****************************************************************/
LUA_ENUM_DECL( indoor_job );

/****************************************************************
** Colony
*****************************************************************/
struct Colony {
  Colony() = default;

  bool operator==( Colony const& ) const = default;

  /************************* Getters ***************************/
  ColonyId id() const { return o_.id; }

  e_nation nation() const { return o_.nation; }

  std::string const& name() const { return o_.name; }

  Coord location() const { return o_.location; }

  refl::enum_map<e_commodity, int> const& commodities() const {
    return o_.commodities;
  }

  auto const& units() const { return o_.units; }

  auto const& indoor_jobs() const { return o_.indoor_jobs; }

  auto const& outdoor_jobs() const { return o_.outdoor_jobs; }

  auto const& buildings() const { return o_.buildings; }

  auto const& production() const { return o_.production; }

  int hammers() const { return o_.hammers; }

  int bells() const { return o_.bells; }

  /************************ Modifiers **************************/
  void add_building( e_colony_building building );

  void rm_building( e_colony_building building );

  void set_nation( e_nation new_nation );

  refl::enum_map<e_commodity, int>& commodities() {
    return o_.commodities;
  }

  void add_hammers( int hammers );

  /************************ Functions **************************/
  int  population() const;
  bool has_unit( UnitId id ) const;

  std::string debug_string() const;

  base::valid_or<std::string> validate() const {
    return o_.validate();
  }

  // Implement refl::WrapsReflected.
  Colony( wrapped::Colony&& o ) : o_( std::move( o ) ) {}
  wrapped::Colony const&            refl() const { return o_; }
  static constexpr std::string_view refl_ns   = "rn";
  static constexpr std::string_view refl_name = "Colony";

 private:
  // This should only be called via the appropriate high level
  // function (below) to ensure that other state gets updated as
  // needed.
  void add_unit( UnitId id, ColonyJob_t const& job );

  friend void move_unit_to_colony( UnitsState& units_state,
                                   Colony&     colony,
                                   UnitId      unit_id,
                                   ColonyJob_t const& job );

 private:
  // This should only be called via the appropriate high level
  // function (below) to ensure that other state gets updated as
  // needed.
  void remove_unit( UnitId id );

  friend void remove_unit_from_colony( UnitsState& units_state,
                                       Colony&     colony,
                                       UnitId      unit_id );

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
LUA_USERDATA_TRAITS( ::rn::CommodityQuantityMap,
                     owned_by_cpp ){};

}
