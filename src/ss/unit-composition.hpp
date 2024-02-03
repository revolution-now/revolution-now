/****************************************************************
**unit-composition.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-02-03.
*
* Description: Represents unit type and inventory.
*
*****************************************************************/
#pragma once

// rds
#include "unit-composition.rds.hpp"

// Revolution Now
#include "expect.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {

/****************************************************************
** UnitComposition
*****************************************************************/
struct UnitComposition {
  using UnitInventoryMap = refl::enum_map<e_unit_inventory, int>;

  UnitComposition() = default;

  UnitComposition( UnitType type );
  UnitComposition( e_unit_type type );

  static expect<UnitComposition> create(
      UnitType type, UnitInventoryMap inventory );

  expect<UnitComposition> with_new_type( UnitType type ) const;

  bool operator==( UnitComposition const& ) const = default;

  e_unit_type base_type() const { return o_.type.base_type(); }
  e_unit_type type() const { return o_.type.type(); }
  UnitType const& type_obj() const { return o_.type; }

  UnitInventoryMap const& inventory() const {
    return o_.inventory;
  }

  // Returns zero if the unit has no such inventory type.
  int operator[]( e_unit_inventory inv ) const;

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

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS( ::rn::UnitComposition, owned_by_lua ){};
}
