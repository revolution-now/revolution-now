/****************************************************************
**unit-composer.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-04.
*
* Description: Represents unit type and inventory, along with
*              type transformations resulting from inventory
*              changes.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "commodity.hpp"
#include "maybe.hpp"
#include "utype.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

// Rds
#include "unit-composer.rds.hpp"

namespace rn {

/****************************************************************
** UnitComposition
*****************************************************************/
class UnitComposition {
 public:
  using UnitInventoryMap =
      std::unordered_map<e_unit_inventory, int>;

  UnitComposition() = default;

  static UnitComposition create( UnitType type );
  static UnitComposition create( e_unit_type type );

  static maybe<UnitComposition> create(
      UnitType type, UnitInventoryMap inventory );

  maybe<UnitComposition> with_new_type( UnitType type ) const;

  bool operator==( UnitComposition const& ) const = default;

  e_unit_type base_type() const { return o_.type.base_type(); }
  e_unit_type type() const { return o_.type.type(); }
  UnitType    type_obj() const { return o_.type; }

  UnitInventoryMap const& inventory() const {
    return o_.inventory;
  }

  // Implement refl::WrapsReflected.
  UnitComposition( wrapped::UnitComposition&& o )
    : o_( std::move( o ) ) {}
  wrapped::UnitComposition const&   refl() const { return o_; }
  static constexpr std::string_view refl_ns = "rn";
  static constexpr std::string_view refl_name =
      "UnitComposition";

 private:
  wrapped::UnitComposition o_;
};

/****************************************************************
** Transformations
*****************************************************************/
struct UnitTransformationResult {
  UnitComposition new_comp = {};
  std::unordered_map<e_unit_type_modifier,
                     e_unit_type_modifier_delta>
      modifier_deltas = {};
  // Positive means that it has been extracted FROM the unit,
  // negative means it has been consumed BY the unit.
  std::unordered_map<e_commodity, int> commodity_deltas = {};

  bool operator==( UnitTransformationResult const& ) const =
      default;

  friend void to_str( UnitTransformationResult const& o,
                      std::string& out, base::ADL_t );
};

// What derived/base units can this unit become, along with the
// delta of commodities and modifiers required for each one. The
// resulting commodity deltas are interpreted as applying to the
// provided commodity store which is assumed to be the one that
// will be used for supplying commodities for the unit change.
// Also, commodities that the unit already has will be reused.
// For example, if a soldier is transformed into a dragoon, the
// muskets will be reused, so the only commodity delta will be
// -50 horses, meaning that 50 horses would have to be subtracted
// from the commodity store in order to affect that transforma-
// tion. Also in that example, there would be one modifier delta,
// namely adding `horses`. Finally, note that this function will
// filter out any results that would result in any commodity in
// the commodity store having a negative quantity (i.e., there is
// not enough). In the cases where a unit transformation could
// potentially consume a variable number of commodities (e.g.
// tools for a pioneer) this function will assume that we want to
// use the maximum number allowed by the unit and that is avail-
// able in the commodity store.
std::vector<UnitTransformationResult>
possible_unit_transformations(
    UnitComposition const& comp,
    std::unordered_map<e_commodity, int> const&
        commodity_store );

// Strip a unit of all commodities and modifiers and return the
// single base type along with the commodity and modifier deltas
// that it took to get there. The commodity deltas will always be
// positive, meaning that the unit is shedding commodities.
UnitTransformationResult strip_to_base_type(
    UnitComposition const& comp );

struct UnitTransformationFromCommodityResult {
  UnitComposition new_comp = {};
  std::unordered_map<e_unit_type_modifier,
                     e_unit_type_modifier_delta>
      modifier_deltas = {};
  int quantity_used   = {};

  bool operator==( UnitTransformationFromCommodityResult const& )
      const = default;

  friend void to_str(
      UnitTransformationFromCommodityResult const& o,
      std::string& out, base::ADL_t );
};

// Can unit receive commodity, and if so how many and what unit
// type will it become?
std::vector<UnitTransformationFromCommodityResult>
unit_receive_commodity( UnitComposition const& comp,
                        Commodity const&       commodity );

void adjust_for_independence_status(
    std::vector<UnitTransformationResult>& input,
    bool independence_declared );

void adjust_for_independence_status(
    std::vector<UnitTransformationFromCommodityResult>& input,
    bool independence_declared );

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS( ::rn::UnitComposition, owned_by_lua ){};
}
