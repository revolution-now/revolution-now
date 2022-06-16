/****************************************************************
**utype.hpp
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
#include "commodity.hpp"
#include "expect.hpp"
#include "lua-enum.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

// Rds
#include "utype.rds.hpp"

// base
#include "base/adl-tag.hpp"

namespace rn {

/****************************************************************
** e_unit_type
*****************************************************************/
LUA_ENUM_DECL( unit_type );

/****************************************************************
** e_unit_human
*****************************************************************/
LUA_ENUM_DECL( unit_human );

/****************************************************************
** e_unit_type_modifier
*****************************************************************/
LUA_ENUM_DECL( unit_type_modifier );

/****************************************************************
** e_unit_activity
*****************************************************************/
LUA_ENUM_DECL( unit_activity );

/****************************************************************
** Unit Inventory
*****************************************************************/
maybe<e_unit_type_modifier> inventory_to_modifier(
    e_unit_inventory inv );

maybe<e_unit_inventory> commodity_to_inventory(
    e_commodity comm );

maybe<e_commodity> inventory_to_commodity(
    e_unit_inventory inv_type );

/****************************************************************
** UnitTypeAttributes
*****************************************************************/
bool can_attack( UnitTypeAttributes const& attr );

bool is_military_unit( UnitTypeAttributes const& attr );

UnitTypeAttributes const& unit_attr( e_unit_type type );

} // namespace rn

namespace lua {
LUA_USERDATA_TRAITS( ::rn::UnitTypeAttributes, owned_by_cpp ){};
}

namespace rn {

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

// This will return nothing if the unit does not have an
// on_death.demoted property, otherwise it will return the new
// UnitType representing the demoted unit, which is guaranteed by
// the game rules (and validation performed during deserializa-
// tion of the unit descriptor configs) to exist regardless of
// base type.
maybe<UnitType> on_death_demoted_type( UnitType ut );

// For units that get demoted upon capture (e.g.
// veteran_colonist) this will return that demoted type.
maybe<e_unit_type> on_capture_demoted_type( UnitType ut );

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

UnitTypeAttributes const& unit_attr( UnitType type );

// This promotes a unit. If the promotion is possible then either
// the base type or derived type (or both) may change. The `ac-
// tivity` parameter may or may not be used depending on the unit
// type. The logic behind this function is a bit complicated; see
// the comments in the Rds definition for UnitPromotion as well
// as the function implementation for more info.
//
// This may be a bit expensive to call; it should not be called
// on every frame or on every unit in a given turn. It should
// only be called when we know that we want to try to promote a
// unit, which should not happen that often. It is ok to call it
// on the order of once per battle, although that probably won't
// happen since the probability of promotion in a battle is not
// large.
maybe<UnitType> promoted_unit_type( UnitType        ut,
                                    e_unit_activity activity );

// Will attempt to clear the expertise (if any) of the base type
// while holding any modifiers constant. Though if the derived
// type specifies a cleared_expertise target then that will be
// respected without regard for the base type: that target will
// be created with its default base type and returned.
maybe<UnitType> cleared_expertise( UnitType ut );

} // namespace rn

namespace lua {
LUA_USERDATA_TRAITS( ::rn::UnitType, owned_by_lua ){};
}
