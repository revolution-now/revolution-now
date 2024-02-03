/****************************************************************
**unit-composition-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-02-03.
*
* Description: Unit tests for the ss/unit-composition module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/ss/unit-composition.hpp"

// Revolution Now
#include "src/lua.hpp"

// luapp
#include "src/luapp/state.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[unit-composition] operator[]" ) {
  auto ut       = UnitType( e_unit_type::pioneer );
  auto maybe_uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 80 } } );
  REQUIRE( maybe_uc.has_value() );
  UnitComposition const& uc = *maybe_uc;
  REQUIRE( uc[e_unit_inventory::tools] == 80 );
  REQUIRE( uc[e_unit_inventory::gold] == 0 );
}

TEST_CASE( "[unit-composition] pioneer tool count" ) {
  auto ut = UnitType( e_unit_type::pioneer );
  auto uc = UnitComposition::create( ut, /*inventory=*/{} );
  REQUIRE( !uc.has_value() );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::gold, 1000 } } );
  REQUIRE( !uc.has_value() );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 100 },
                          { e_unit_inventory::gold, 1000 } } );
  REQUIRE( !uc.has_value() );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 120 } } );
  REQUIRE( !uc.has_value() );
  ut = UnitType( e_unit_type::hardy_pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 120 } } );
  REQUIRE( !uc.has_value() );

  ut = UnitType( e_unit_type::pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 105 } } );
  REQUIRE( !uc.has_value() );
  ut = UnitType( e_unit_type::hardy_pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 105 } } );
  REQUIRE( !uc.has_value() );

  ut = UnitType( e_unit_type::pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 100 } } );
  REQUIRE( uc.has_value() );
  ut = UnitType( e_unit_type::hardy_pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 100 } } );
  REQUIRE( uc.has_value() );

  ut = UnitType( e_unit_type::pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 90 } } );
  REQUIRE( !uc.has_value() );
  ut = UnitType( e_unit_type::hardy_pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 90 } } );
  REQUIRE( !uc.has_value() );

  ut = UnitType( e_unit_type::pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 80 } } );
  REQUIRE( uc.has_value() );
  ut = UnitType( e_unit_type::hardy_pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 80 } } );
  REQUIRE( uc.has_value() );

  ut = UnitType( e_unit_type::pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 20 } } );
  REQUIRE( uc.has_value() );
  ut = UnitType( e_unit_type::hardy_pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 20 } } );
  REQUIRE( uc.has_value() );

  ut = UnitType( e_unit_type::pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 15 } } );
  REQUIRE( !uc.has_value() );
  ut = UnitType( e_unit_type::hardy_pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 15 } } );
  REQUIRE( !uc.has_value() );

  ut = UnitType( e_unit_type::pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 0 } } );
  REQUIRE( !uc.has_value() );
  ut = UnitType( e_unit_type::hardy_pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 0 } } );
  REQUIRE( !uc.has_value() );

  ut = UnitType( e_unit_type::pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, -20 } } );
  REQUIRE( !uc.has_value() );
  ut = UnitType( e_unit_type::hardy_pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, -20 } } );
  REQUIRE( !uc.has_value() );
}

TEST_CASE( "[unit-transformation] lua bindings" ) {
  lua::state st;
  st.lib.open_all();
  run_lua_startup_routines( st );

  auto script = R"lua(
    local uc
    local UC = unit_composition.UnitComposition
    -- free_colonist
    uc = UC.create_with_type( "free_colonist" )
    assert( uc )
    assert( uc:type() == "free_colonist" )
    assert( uc:base_type() == "free_colonist" )
    assert( uc:type_obj():base_type() ==
            "free_colonist" )
    -- dragoon
    uc = UC.create_with_type( "dragoon" )
    assert( uc )
    assert( uc:type() == "dragoon" )
    assert( uc:base_type() == "free_colonist" )
    -- veteran_soldier
    uc = UC.create_with_type( "veteran_soldier" )
    assert( uc )
    assert( uc:type() == "veteran_soldier" )
    assert( uc:base_type() == "veteran_colonist" )
    -- pioneer
    local ut_obj = unit_type.UnitType.create_with_base(
        "pioneer", "expert_farmer" )
    uc = UC.create_with_type_obj( ut_obj )
    assert( uc )
    assert( uc:type() == "pioneer" )
    assert( uc:base_type() == "expert_farmer" )
  )lua";
  REQUIRE( st.script.run_safe( script ) == valid );
}

} // namespace
} // namespace rn
