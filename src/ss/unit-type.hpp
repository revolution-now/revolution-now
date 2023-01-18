/****************************************************************
**unit-type.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-11.
*
* Description: Unit type descriptors.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "expect.hpp"
#include "maybe.hpp"

// ss
#include "ss/commodity.rds.hpp"
#include "ss/mv-points.hpp"
#include "ss/unit-type.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

// base
#include "base/adl-tag.hpp"

// C++ standard library
#include <unordered_set>

namespace rn {

struct Player;

/****************************************************************
** Unit Inventory
*****************************************************************/
maybe<e_unit_inventory> commodity_to_inventory(
    e_commodity comm );

maybe<e_commodity> inventory_to_commodity(
    e_unit_inventory inv_type );

/****************************************************************
** UnitTypeAttributes
*****************************************************************/
bool can_attack( e_unit_type type );

bool is_military_unit( e_unit_type type );

// This is the only way that code outside of this module should
// query a unit's movement points, since it will take into ac-
// count bonuses.
MvPoints movement_points( Player const& player,
                          e_unit_type   type );

/****************************************************************
** UnitType
*****************************************************************/
// The purpose of this struct is to maintain an invariant, namely
// that the derived unit type (`type`) can be reached starting
// from the base type (`base_type`) via some set of modifiers.
//
// The reason that it must store the base type and not just the
// final type is because the game rules allow units to lose modi-
// fiers in various ways, in which case a unit must "remember"
// what it's base type was. For example, when an indentured ser-
// vant (the base type) is made into a soldier (the final type)
// and that soldier loses a battle then the unit must be demoted
// back to an indentured servant (and not e.g. a free colonist)
// and so therefore when we store the type of a unit we must in-
// clude this base type.
struct UnitType {
  static maybe<UnitType> create( e_unit_type type,
                                 e_unit_type base_type );

  // If type is a derived type then it will use its canonical
  // base for the base type.
  static UnitType create( e_unit_type type );

  e_unit_type base_type() const { return o_.base_type; }

  e_unit_type type() const { return o_.type; }

  bool operator==( UnitType const& ) const = default;

  UnitType();

  // If the base type and derived type are different then there
  // is always guaranteed to be a set of (non-empty) modifiers
  // that express that path, since that invariant is upheld by
  // this class. The returned set is empty if and only if the
  // base type and derived type are the same.
  std::unordered_set<e_unit_type_modifier> const&
  unit_type_modifiers();

  // Implement refl::WrapsReflected.
  UnitType( wrapped::UnitType&& o ) : o_( std::move( o ) ) {}
  wrapped::UnitType const&          refl() const { return o_; }
  static constexpr std::string_view refl_ns   = "rn";
  static constexpr std::string_view refl_name = "UnitType";

 private:
  UnitType( e_unit_type base_type, e_unit_type type );

  // Check-fails when invariants are broken.
  void check_invariants_or_die() const;

  wrapped::UnitType o_;
};

// This type is intended to be light-weight and to be passed
// around by value and copied. It is also immutable.
static_assert( sizeof( UnitType ) <=
               2 * std::alignment_of_v<e_unit_type> );
static_assert( std::is_trivially_copyable_v<UnitType> );
static_assert( std::is_trivially_destructible_v<UnitType> );

bool is_unit_human( UnitType ut );

// Can this unit type found a colony?
bool can_unit_found( UnitType ut );

// For scout or seasoned_scout.
maybe<e_scout_type> scout_type( e_unit_type type );

maybe<e_missionary_type> missionary_type( UnitType type );

// Try to add the modifiers to the type and return the resulting
// type if it works out.
maybe<UnitType> add_unit_type_modifiers(
    UnitType                                        ut,
    std::unordered_set<e_unit_type_modifier> const& modifiers );

// Try to remove the modifiers from the type and return the re-
// sulting type if it works out. The unit must have all of the
// modifiers in order for this to be successful.
maybe<UnitType> rm_unit_type_modifiers(
    UnitType                                        ut,
    std::unordered_set<e_unit_type_modifier> const& modifiers );

// Given a base unit type and a set of proposed modifiers, this
// function will determine if there is a resulting derived type.
// If there is, it is guaranteed to be unique since config vali-
// dation should have verified that.
maybe<UnitType> find_unit_type_modifiers(
    e_unit_type                                     base_type,
    std::unordered_set<e_unit_type_modifier> const& modifiers );

} // namespace rn

namespace lua {
LUA_USERDATA_TRAITS( ::rn::UnitType, owned_by_lua ){};
}
