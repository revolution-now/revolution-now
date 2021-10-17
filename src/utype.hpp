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
#include "coord.hpp"
#include "enum-map.hpp"
#include "fb.hpp"
#include "lua-enum.hpp"
#include "mv-points.hpp"
#include "tiles.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

// Rds
#include "rds/utype.hpp"

// Rcl
#include "rcl/ext.hpp"

// base
#include "base/fmt.hpp"

// Flatbuffers
#include "fb/utype_generated.h"

namespace rn {

/****************************************************************
** e_unit_type
*****************************************************************/
LUA_ENUM_DECL( unit_type );

/****************************************************************
** e_unit_type_modifier
*****************************************************************/
LUA_ENUM_DECL( unit_type_modifier );

struct UnitTypeModifierTraits {
  bool                  player_can_grant;
  ModifierAssociation_t association;
};

// This is for deserializing from Rcl config files.
rcl::convert_err<UnitTypeModifierTraits> convert_to(
    rcl::value const& v, rcl::tag<UnitTypeModifierTraits> );

/****************************************************************
** e_unit_activity
*****************************************************************/
LUA_ENUM_DECL( unit_activity );

/****************************************************************
** Unit Inventory
*****************************************************************/
maybe<std::pair<e_unit_type_modifier,
                ModifierAssociation::inventory const&>>
inventory_to_modifier( e_unit_inventory inv );

maybe<e_unit_inventory> commodity_to_inventory(
    e_commodity comm );

maybe<e_commodity> inventory_to_commodity(
    e_unit_inventory inv_type );

// TODO: Move this to Rds once we have reflected structures.
struct UnitInventoryTraits {
  maybe<e_commodity> commodity;
  int                min_quantity = 0;
  int                max_quantity = 0;
  int                multiple     = 0;
};

rcl::convert_err<UnitInventoryTraits> convert_to(
    rcl::value const& v, rcl::tag<UnitInventoryTraits> );

rcl::convert_valid rcl_validate( UnitInventoryTraits const& o );

/****************************************************************
** UnitTypeAttributes
*****************************************************************/
// Describes a unit type without regard
struct UnitTypeAttributes {
  std::string name{};

  // Rendering
  e_tile      tile{};
  bool        nat_icon_front{};
  e_direction nat_icon_position{};

  // Movement
  bool     ship{};
  int      visibility{};
  MvPoints movement_points{};

  // Combat
  int attack_points{};
  int defense_points{};

  // Cargo
  int cargo_slots{};
  // Slots occupied by this unit.
  maybe<int> cargo_slots_occupies{};

  UnitDeathAction_t on_death{};

  // If this is a derived unit type then it must specify a canon-
  // ical base type that will be used to construct it when none
  // is specified. In some cases there is only one allowed base
  // type (e.g. a veteran dragoon must have a veteran colonist as
  // its base type) in which case it must hold that value.
  maybe<e_unit_type> canonical_base{};

  // If this unit type has an expertise. This is only for some
  // base types.
  maybe<e_unit_activity> expertise{};

  // Describes how this unit (can be either base or derived) han-
  // dles the clearing of its specialty. If it is a base type
  // then this can be either null (cannot clear) or a unit type.
  // For derived types, this can be either null (meaning consult
  // the base type) or a unit type. See the `cleared_expertise`
  // function below for more info on the precise logic used.
  maybe<e_unit_type> cleared_expertise{};

  // Determines how the unit gets promoted or how the unit influ-
  // ences promotion of a base type. See sumtype comments to ex-
  // planation of the meaning of the variant alternatives.
  maybe<UnitPromotion_t> promotion{};

  // Tells us how to convert a base unit type into another in re-
  // sponse to the gain of some modifiers, e.g. when a
  // free_colonist is given horses, what unit type does it be-
  // come?  Only base unit types can have this.
  std::unordered_map<e_unit_type,
                     std::unordered_set<e_unit_type_modifier>>
      modifiers{};

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Derived fields.
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  e_unit_type type{};

  // Can this unit type be obtained from a base type
  // plus some modifiers?
  bool is_derived{};

  bool can_attack() const { return attack_points > 0; }
  bool is_military_unit() const { return can_attack(); }
};
NOTHROW_MOVE( UnitTypeAttributes );

rcl::convert_err<UnitTypeAttributes> convert_to(
    rcl::value const& v, rcl::tag<UnitTypeAttributes> );

UnitTypeAttributes const& unit_attr( e_unit_type type );

} // namespace rn

namespace lua {
LUA_USERDATA_TRAITS( ::rn::UnitTypeAttributes, owned_by_cpp ){};
}

namespace rn {

/****************************************************************
** UnitAttributesMap
*****************************************************************/
using UnitAttributesMap =
    ExhaustiveEnumMap<e_unit_type, UnitTypeAttributes>;

// This is for deserializing from Rcl config files.
rcl::convert_err<UnitAttributesMap> convert_to(
    rcl::value const& v, rcl::tag<UnitAttributesMap> );

// Post-deserialization validator found through ADL.
rcl::convert_valid rcl_validate( UnitAttributesMap const& m );

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

  e_unit_type base_type() const { return base_type_; }

  e_unit_type type() const { return type_; }

  bool operator==( UnitType const& ) const = default;

  UnitType();

  // If the base type and derived type are different then there
  // is always guaranteed to be a set of (non-empty) modifiers
  // that express that path, since that invariant is upheld by
  // this class. The returned set is empty if and only if the
  // base type and derived type are the same.
  std::unordered_set<e_unit_type_modifier> const&
  unit_type_modifiers();

  valid_deserial_t check_invariants_safe() const;

private:
  UnitType( e_unit_type base_type, e_unit_type type );

  // Check-fails when invariants are broken.
  void check_invariants_or_die() const;

  // clang-format off
  SERIALIZABLE_TABLE_MEMBERS( fb, UnitType,
  ( e_unit_type, base_type_ ),
  ( e_unit_type, type_      ));
  // clang-format on
};

// This type is intended to be light-weight and to be passed
// around by value and copied. It is also immutable.
static_assert( sizeof( UnitType ) <=
               2 * std::alignment_of_v<e_unit_type> );
static_assert( std::is_trivially_copyable_v<UnitType> );
static_assert( std::is_trivially_destructible_v<UnitType> );

// This will return nothing if the unit does not have an
// on_death.demoted property, otherwise it will return the new
// UnitType representing the demoted unit, which is guaranteed by
// the game rules (and validation performed during deserializa-
// tion of the unit descriptor configs) to exist regardless of
// base type.
maybe<UnitType> on_death_demoted_type( UnitType ut );

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

namespace rn {

//

} // namespace rn

DEFINE_FORMAT( ::rn::UnitType, "UnitType{{type={},base={}}}",
               o.type(), o.base_type() );
