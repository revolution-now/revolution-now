/****************************************************************
**unit-transformation.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-04.
*
* Description: Unit tests for the src/unit-transformation.*
*module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/unit-transformation.hpp"

// Testing.
#include "test/fake/world.hpp"

// Revolution Now
#include "src/unit-mgr.hpp"

// ss
#include "ss/player.rds.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"
#include "base/to-str-tags.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::base::FmtVerticalJsonList;

void sort_by_new_type( vector<UnitTransformation>& v ) {
  sort( v.begin(), v.end(), []( auto const& l, auto const& r ) {
    return static_cast<int>( l.new_comp.type() ) <
           static_cast<int>( r.new_comp.type() );
  } );
}

void sort_by_new_type(
    vector<UnitTransformationFromCommodity>& v ) {
  sort( v.begin(), v.end(), []( auto const& l, auto const& r ) {
    return static_cast<int>( l.new_comp.type() ) <
           static_cast<int>( r.new_comp.type() );
  } );
}

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
      _, L, _, //
      L, L, L, //
      _, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE(
    "[unit-transformation] possible_unit_transformations no "
    "commodities available" ) {
  // These remain empty for this test.
  unordered_map<e_commodity, int> comms;

  UnitComposition            comp{};
  vector<UnitTransformation> res;
  vector<UnitTransformation> expected;

  // free_colonist.
  comp     = UnitComposition( e_unit_type::free_colonist );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::free_colonist,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = {},
    } };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // missionary.
  comp     = UnitComposition( e_unit_type::missionary );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::free_colonist,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    } };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // jesuit_missionary.
  comp     = UnitComposition( e_unit_type::jesuit_missionary );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::jesuit_colonist,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::jesuit_missionary,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    } };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // Pioneer/indentured_servant with 100 tools.
  comp = UnitComposition::create(
             UnitType::create( e_unit_type::pioneer,
                               e_unit_type::indentured_servant )
                 .value(),
             /*inventory=*/{ { e_unit_inventory::tools, 100 } } )
             .value();
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::indentured_servant,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::tools, 100 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::pioneer,
                          e_unit_type::indentured_servant )
                          .value(),
                      /*inventory=*/{ { e_unit_inventory::tools,
                                        100 } } )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::indentured_servant )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::tools, 100 } },
    } };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // Hardy Pioneer with 80 tools.
  comp = UnitComposition::create(
             UnitType::create( e_unit_type::hardy_pioneer,
                               e_unit_type::hardy_colonist )
                 .value(),
             /*inventory=*/{ { e_unit_inventory::tools, 80 } } )
             .value();
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::hardy_colonist,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::tools, 80 } },
    },
    UnitTransformation{
      .new_comp =
          UnitComposition::create(
              /*type=*/UnitType::create(
                  e_unit_type::hardy_pioneer,
                  e_unit_type::hardy_colonist )
                  .value(),
              /*inventory=*/{ { e_unit_inventory::tools, 80 } } )
              .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::hardy_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::tools, 80 } },
    } };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  comp     = UnitComposition( e_unit_type::soldier );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::free_colonist,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::muskets, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::soldier,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::muskets, 50 } },
    } };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // Soldier with petty criminal.
  comp = UnitComposition(
      UnitType::create( e_unit_type::soldier,
                        e_unit_type::petty_criminal )
          .value() );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::petty_criminal,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::muskets, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::soldier,
                          e_unit_type::petty_criminal )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::petty_criminal )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::muskets, 50 } },
    } };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // Veteran Dragoon.
  comp     = UnitComposition( e_unit_type::veteran_dragoon );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::veteran_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::horses, 50 },
                            { e_commodity::muskets, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::scout,
                          e_unit_type::veteran_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::muskets, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::veteran_colonist,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::horses, 50 },
                            { e_commodity::muskets, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::veteran_soldier,
                          e_unit_type::veteran_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::horses, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::veteran_dragoon,
                          e_unit_type::veteran_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::continental_army,
                          e_unit_type::veteran_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::independence,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::horses, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::continental_cavalry,
                          e_unit_type::veteran_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::independence,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = {},
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );
}

TEST_CASE(
    "[unit-transformation] possible_unit_transformations full "
    "commodities available" ) {
  // This will remain full for this test.
  unordered_map<e_commodity, int> comms;
  for( e_commodity c : refl::enum_values<e_commodity> )
    comms[c] = 100;

  UnitComposition            comp{};
  vector<UnitTransformation> res;
  vector<UnitTransformation> expected;

  // free_colonist.
  comp     = UnitComposition( e_unit_type::free_colonist );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::free_colonist,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::soldier,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::muskets, -50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::dragoon,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::muskets, -50 },
                            { e_commodity::horses, -50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::pioneer,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{ { e_unit_inventory::tools,
                                        100 } } )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::tools, -100 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::scout,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::horses, -50 } },
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // missionary.
  comp     = UnitComposition( e_unit_type::missionary );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::free_colonist,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::soldier,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::muskets, -50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::dragoon,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::muskets, -50 },
                            { e_commodity::horses, -50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::pioneer,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{ { e_unit_inventory::tools,
                                        100 } } )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::tools, -100 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::scout,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::horses, -50 } },
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // jesuit_missionary.
  comp     = UnitComposition( e_unit_type::jesuit_missionary );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::jesuit_colonist,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::soldier,
                          e_unit_type::jesuit_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::muskets, -50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::dragoon,
                          e_unit_type::jesuit_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::muskets, -50 },
                            { e_commodity::horses, -50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::pioneer,
                          e_unit_type::jesuit_colonist )
                          .value(),
                      /*inventory=*/{ { e_unit_inventory::tools,
                                        100 } } )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::tools, -100 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::jesuit_missionary,
                          e_unit_type::jesuit_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::scout,
                          e_unit_type::jesuit_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::horses, -50 } },
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // Pioneer/indentured_servant with 100 tools.
  comp = UnitComposition::create(
             UnitType::create( e_unit_type::pioneer,
                               e_unit_type::indentured_servant )
                 .value(),
             /*inventory=*/{ { e_unit_inventory::tools, 100 } } )
             .value();
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::indentured_servant,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::tools, 100 } },
    },
    UnitTransformation{
      .new_comp =
          UnitComposition::create(
              /*type=*/
              UnitType::create( e_unit_type::soldier,
                                e_unit_type::indentured_servant )
                  .value(),
              /*inventory=*/{} )
              .value(),
      .modifier_deltas  = { { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::muskets, -50 },
                            { e_commodity::tools, 100 } },
    },
    UnitTransformation{
      .new_comp =
          UnitComposition::create(
              /*type=*/
              UnitType::create( e_unit_type::dragoon,
                                e_unit_type::indentured_servant )
                  .value(),
              /*inventory=*/{} )
              .value(),
      .modifier_deltas  = { { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::muskets, -50 },
                            { e_commodity::tools, 100 },
                            { e_commodity::horses, -50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::pioneer,
                          e_unit_type::indentured_servant )
                          .value(),
                      /*inventory=*/{ { e_unit_inventory::tools,
                                        100 } } )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::indentured_servant )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::tools, 100 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::scout,
                          e_unit_type::indentured_servant )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas =
          {
            { e_unit_type_modifier::tools,
              e_unit_type_modifier_delta::del },
            { e_unit_type_modifier::horses,
              e_unit_type_modifier_delta::add },
          },
      .commodity_deltas = { { e_commodity::tools, 100 },
                            { e_commodity::horses, -50 } },
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // Hardy Pioneer with 80 tools.
  comp = UnitComposition::create(
             UnitType::create( e_unit_type::hardy_pioneer,
                               e_unit_type::hardy_colonist )
                 .value(),
             /*inventory=*/{ { e_unit_inventory::tools, 80 } } )
             .value();
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp =
          UnitComposition::create(
              /*type=*/
              UnitType::create( e_unit_type::soldier,
                                e_unit_type::hardy_colonist )
                  .value(),
              /*inventory=*/{} )
              .value(),
      .modifier_deltas  = { { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::muskets, -50 },
                            { e_commodity::tools, 80 } },
    },
    UnitTransformation{
      .new_comp =
          UnitComposition::create(
              /*type=*/
              UnitType::create( e_unit_type::dragoon,
                                e_unit_type::hardy_colonist )
                  .value(),
              /*inventory=*/{} )
              .value(),
      .modifier_deltas  = { { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::muskets, -50 },
                            { e_commodity::tools, 80 },
                            { e_commodity::horses, -50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::hardy_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::tools, 80 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::scout,
                          e_unit_type::hardy_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas =
          {
            { e_unit_type_modifier::tools,
              e_unit_type_modifier_delta::del },
            { e_unit_type_modifier::horses,
              e_unit_type_modifier_delta::add },
          },
      .commodity_deltas = { { e_commodity::tools, 80 },
                            { e_commodity::horses, -50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::hardy_colonist,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::tools, 80 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::hardy_pioneer,
                          e_unit_type::hardy_colonist )
                          .value(),
                      /*inventory=*/{ { e_unit_inventory::tools,
                                        100 } } )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = { { e_commodity::tools, -20 } },
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  comp     = UnitComposition( e_unit_type::soldier );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::free_colonist,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::muskets, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::dragoon,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::horses, -50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::soldier,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::pioneer,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{ { e_unit_inventory::tools,
                                        100 } } )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::tools, -100 },
                            { e_commodity::muskets, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::scout,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::muskets, 50 },
                            { e_commodity::horses, -50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::muskets, 50 } },
    } };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // Soldier with petty criminal.
  comp = UnitComposition(
      UnitType::create( e_unit_type::soldier,
                        e_unit_type::petty_criminal )
          .value() );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::petty_criminal,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::muskets, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::dragoon,
                          e_unit_type::petty_criminal )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::horses, -50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::soldier,
                          e_unit_type::petty_criminal )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::pioneer,
                          e_unit_type::petty_criminal )
                          .value(),
                      /*inventory=*/{ { e_unit_inventory::tools,
                                        100 } } )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::tools, -100 },
                            { e_commodity::muskets, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::scout,
                          e_unit_type::petty_criminal )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::muskets, 50 },
                            { e_commodity::horses, -50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::petty_criminal )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::muskets, 50 } },
    } };

  // Veteran Dragoon.
  comp     = UnitComposition( e_unit_type::veteran_dragoon );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::pioneer,
                          e_unit_type::veteran_colonist )
                          .value(),
                      /*inventory=*/{ { e_unit_inventory::tools,
                                        100 } } )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::muskets, 50 },
                            { e_commodity::horses, 50 },
                            { e_commodity::tools, -100 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::veteran_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::horses, 50 },
                            { e_commodity::muskets, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::scout,
                          e_unit_type::veteran_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::muskets, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::veteran_colonist,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::horses, 50 },
                            { e_commodity::muskets, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::veteran_soldier,
                          e_unit_type::veteran_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::horses, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::veteran_dragoon,
                          e_unit_type::veteran_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::continental_army,
                          e_unit_type::veteran_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::independence,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::horses, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::continental_cavalry,
                          e_unit_type::veteran_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::independence,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = {},
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );
}

TEST_CASE(
    "[unit-transformation] possible_unit_transformations "
    "partial commodities available" ) {
  // This will remain full for this test.
  unordered_map<e_commodity, int> comms;
  for( e_commodity c : refl::enum_values<e_commodity> )
    comms[c] = 40;

  UnitComposition            comp{};
  vector<UnitTransformation> res;
  vector<UnitTransformation> expected;

  // free_colonist.
  comp     = UnitComposition( e_unit_type::free_colonist );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::free_colonist,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp =
          UnitComposition::create(
              /*type=*/UnitType::create(
                  e_unit_type::pioneer,
                  e_unit_type::free_colonist )
                  .value(),
              /*inventory=*/{ { e_unit_inventory::tools, 40 } } )
              .value(),
      .modifier_deltas  = { { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::tools, -40 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = {},
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // missionary.
  comp     = UnitComposition( e_unit_type::missionary );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::free_colonist,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp =
          UnitComposition::create(
              /*type=*/UnitType::create(
                  e_unit_type::pioneer,
                  e_unit_type::free_colonist )
                  .value(),
              /*inventory=*/{ { e_unit_inventory::tools, 40 } } )
              .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::tools, -40 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // jesuit_missionary.
  comp     = UnitComposition( e_unit_type::jesuit_missionary );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::jesuit_colonist,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp =
          UnitComposition::create(
              /*type=*/UnitType::create(
                  e_unit_type::pioneer,
                  e_unit_type::jesuit_colonist )
                  .value(),
              /*inventory=*/{ { e_unit_inventory::tools, 40 } } )
              .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::tools, -40 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::jesuit_missionary,
                          e_unit_type::jesuit_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // Pioneer/indentured_servant with 100 tools.
  comp = UnitComposition::create(
             UnitType::create( e_unit_type::pioneer,
                               e_unit_type::indentured_servant )
                 .value(),
             /*inventory=*/{ { e_unit_inventory::tools, 100 } } )
             .value();
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::indentured_servant,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::tools, 100 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::pioneer,
                          e_unit_type::indentured_servant )
                          .value(),
                      /*inventory=*/{ { e_unit_inventory::tools,
                                        100 } } )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::indentured_servant )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::tools, 100 } },
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // Hardy Pioneer with 80 tools.
  comp = UnitComposition::create(
             UnitType::create( e_unit_type::hardy_pioneer,
                               e_unit_type::hardy_colonist )
                 .value(),
             /*inventory=*/{ { e_unit_inventory::tools, 80 } } )
             .value();
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::hardy_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::tools, 80 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::hardy_colonist,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::tools, 80 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::hardy_pioneer,
                          e_unit_type::hardy_colonist )
                          .value(),
                      /*inventory=*/{ { e_unit_inventory::tools,
                                        100 } } )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = { { e_commodity::tools, -20 } },
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // soldier/free_colonist.
  comp     = UnitComposition( e_unit_type::soldier );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::free_colonist,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::muskets, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::soldier,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp =
          UnitComposition::create(
              /*type=*/UnitType::create(
                  e_unit_type::pioneer,
                  e_unit_type::free_colonist )
                  .value(),
              /*inventory=*/{ { e_unit_inventory::tools, 40 } } )
              .value(),
      .modifier_deltas  = { { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::tools, -40 },
                            { e_commodity::muskets, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::muskets, 50 } },
    } };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // Soldier with petty criminal.
  comp = UnitComposition(
      UnitType::create( e_unit_type::soldier,
                        e_unit_type::petty_criminal )
          .value() );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::petty_criminal,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::muskets, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::soldier,
                          e_unit_type::petty_criminal )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp =
          UnitComposition::create(
              /*type=*/UnitType::create(
                  e_unit_type::pioneer,
                  e_unit_type::petty_criminal )
                  .value(),
              /*inventory=*/{ { e_unit_inventory::tools, 40 } } )
              .value(),
      .modifier_deltas  = { { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::tools, -40 },
                            { e_commodity::muskets, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::petty_criminal )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::muskets, 50 } },
    } };

  // Veteran Dragoon.
  comp     = UnitComposition( e_unit_type::veteran_dragoon );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp =
          UnitComposition::create(
              /*type=*/UnitType::create(
                  e_unit_type::pioneer,
                  e_unit_type::veteran_colonist )
                  .value(),
              /*inventory=*/{ { e_unit_inventory::tools, 40 } } )
              .value(),
      .modifier_deltas  = { { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::muskets, 50 },
                            { e_commodity::horses, 50 },
                            { e_commodity::tools, -40 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::missionary,
                          e_unit_type::veteran_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::horses, 50 },
                            { e_commodity::muskets, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::scout,
                          e_unit_type::veteran_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::muskets, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::veteran_colonist,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::horses, 50 },
                            { e_commodity::muskets, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::veteran_soldier,
                          e_unit_type::veteran_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::horses, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::veteran_dragoon,
                          e_unit_type::veteran_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::continental_army,
                          e_unit_type::veteran_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::independence,
                              e_unit_type_modifier_delta::add },
                            { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::horses, 50 } },
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::continental_cavalry,
                          e_unit_type::veteran_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::independence,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = {},
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );
}

TEST_CASE(
    "[unit-transformation] possible_unit_transformations full "
    "commodities available, misc types" ) {
  // This will remain full for this test.
  unordered_map<e_commodity, int> comms;
  for( e_commodity c : refl::enum_values<e_commodity> )
    comms[c] = 100;

  UnitComposition            comp{};
  vector<UnitTransformation> res;
  vector<UnitTransformation> expected;

  // regular.
  comp     = UnitComposition( e_unit_type::regular );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::regular,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp =
          UnitComposition::create(
              /*type=*/UnitType::create( e_unit_type::cavalry,
                                         e_unit_type::regular )
                  .value(),
              /*inventory=*/{} )
              .value(),
      .modifier_deltas  = { { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::horses, -50 } },
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // cavalry.
  comp     = UnitComposition( e_unit_type::cavalry );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::regular,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::horses, 50 } },
    },
    UnitTransformation{
      .new_comp =
          UnitComposition::create(
              /*type=*/UnitType::create( e_unit_type::cavalry,
                                         e_unit_type::regular )
                  .value(),
              /*inventory=*/{} )
              .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // artillery.
  comp     = UnitComposition( e_unit_type::artillery );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::artillery,
                          e_unit_type::damaged_artillery )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::damaged_artillery,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::strength,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = {},
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // damaged_artillery.
  comp     = UnitComposition( e_unit_type::damaged_artillery );
  res      = possible_unit_transformations( comp, comms );
  expected = {
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::artillery,
                          e_unit_type::damaged_artillery )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::strength,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = {},
    },
    UnitTransformation{
      .new_comp = UnitComposition::create(
                      /*type=*/e_unit_type::damaged_artillery,
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );
}

TEST_CASE( "[unit-transformation] unit_receive_commodity" ) {
  UnitComposition                         comp{};
  Commodity                               comm;
  vector<UnitTransformationFromCommodity> res;
  vector<UnitTransformationFromCommodity> expected;

  // free_colonist + 40 muskets.
  comp     = UnitComposition( e_unit_type::free_colonist );
  comm     = { .type = e_commodity::muskets, .quantity = 40 };
  res      = unit_receive_commodity( comp, comm );
  expected = {};
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // free_colonist + 50 muskets.
  comp     = UnitComposition( e_unit_type::free_colonist );
  comm     = { .type = e_commodity::muskets, .quantity = 50 };
  res      = unit_receive_commodity( comp, comm );
  expected = {
    UnitTransformationFromCommodity{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::soldier,
                          e_unit_type::free_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas = { { e_unit_type_modifier::muskets,
                             e_unit_type_modifier_delta::add } },
      .quantity_used   = 50,
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // soldier + 50 muskets.
  comp     = UnitComposition( e_unit_type::soldier );
  comm     = { .type = e_commodity::muskets, .quantity = 50 };
  res      = unit_receive_commodity( comp, comm );
  expected = {};
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // missionary + 50 horses.
  comp     = UnitComposition( e_unit_type::missionary );
  comm     = { .type = e_commodity::horses, .quantity = 50 };
  res      = unit_receive_commodity( comp, comm );
  expected = {};
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // indentured_servant + 100 horses.
  comp     = UnitComposition( e_unit_type::indentured_servant );
  comm     = { .type = e_commodity::horses, .quantity = 100 };
  res      = unit_receive_commodity( comp, comm );
  expected = {
    UnitTransformationFromCommodity{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::scout,
                          e_unit_type::indentured_servant )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas = { { e_unit_type_modifier::horses,
                             e_unit_type_modifier_delta::add } },
      .quantity_used   = 50,
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // hardy_colonist + 50 tools.
  comp     = UnitComposition( e_unit_type::hardy_colonist );
  comm     = { .type = e_commodity::tools, .quantity = 50 };
  res      = unit_receive_commodity( comp, comm );
  expected = {
    UnitTransformationFromCommodity{
      .new_comp =
          UnitComposition::create(
              /*type=*/UnitType::create(
                  e_unit_type::hardy_pioneer,
                  e_unit_type::hardy_colonist )
                  .value(),
              /*inventory=*/{ { e_unit_inventory::tools, 40 } } )
              .value(),
      .modifier_deltas = { { e_unit_type_modifier::tools,
                             e_unit_type_modifier_delta::add } },
      .quantity_used   = 40,
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // hardy_colonist + 120 tools.
  comp     = UnitComposition( e_unit_type::hardy_colonist );
  comm     = { .type = e_commodity::tools, .quantity = 120 };
  res      = unit_receive_commodity( comp, comm );
  expected = {
    UnitTransformationFromCommodity{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::hardy_pioneer,
                          e_unit_type::hardy_colonist )
                          .value(),
                      /*inventory=*/{ { e_unit_inventory::tools,
                                        100 } } )
                      .value(),
      .modifier_deltas = { { e_unit_type_modifier::tools,
                             e_unit_type_modifier_delta::add } },
      .quantity_used   = 100,
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // hardy_pioneer with 20 tools + 20 tools.
  comp = UnitComposition::create(
             e_unit_type::hardy_pioneer,
             /*inventory=*/{ { e_unit_inventory::tools, 20 } } )
             .value();
  comm     = { .type = e_commodity::tools, .quantity = 20 };
  res      = unit_receive_commodity( comp, comm );
  expected = {
    UnitTransformationFromCommodity{
      .new_comp =
          UnitComposition::create(
              /*type=*/UnitType::create(
                  e_unit_type::hardy_pioneer,
                  e_unit_type::hardy_colonist )
                  .value(),
              /*inventory=*/{ { e_unit_inventory::tools, 40 } } )
              .value(),
      .modifier_deltas = {},
      .quantity_used   = 20,
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // hardy_pioneer with 80 tools + 45 tools.
  comp = UnitComposition::create(
             e_unit_type::hardy_pioneer,
             /*inventory=*/{ { e_unit_inventory::tools, 80 } } )
             .value();
  comm     = { .type = e_commodity::tools, .quantity = 45 };
  res      = unit_receive_commodity( comp, comm );
  expected = {
    UnitTransformationFromCommodity{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::hardy_pioneer,
                          e_unit_type::hardy_colonist )
                          .value(),
                      /*inventory=*/{ { e_unit_inventory::tools,
                                        100 } } )
                      .value(),
      .modifier_deltas = {},
      .quantity_used   = 20,
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // hardy_pioneer with 100 tools + 20 tools.
  comp = UnitComposition::create(
             e_unit_type::hardy_pioneer,
             /*inventory=*/{ { e_unit_inventory::tools, 100 } } )
             .value();
  comm     = { .type = e_commodity::tools, .quantity = 20 };
  res      = unit_receive_commodity( comp, comm );
  expected = {};
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );
}

TEST_CASE(
    "[unit-transformation] adjust_for_independence_status" ) {
  SECTION( "general" ) {
    vector<UnitTransformation> input;
    vector<UnitTransformation> expected;

    // Add independence after independence is declared.
    input = {
      UnitTransformation{
        .new_comp = UnitComposition::create(
                        /*type=*/UnitType::create(
                            e_unit_type::veteran_soldier,
                            e_unit_type::veteran_colonist )
                            .value(),
                        /*inventory=*/{} )
                        .value(),
      },
      UnitTransformation{
        .new_comp = UnitComposition::create(
                        /*type=*/UnitType::create(
                            e_unit_type::continental_army,
                            e_unit_type::veteran_colonist )
                            .value(),
                        /*inventory=*/{} )
                        .value(),
        .modifier_deltas =
            { { e_unit_type_modifier::independence,
                e_unit_type_modifier_delta::add } } },
    };
    expected = {
      UnitTransformation{
        .new_comp = UnitComposition::create(
                        /*type=*/UnitType::create(
                            e_unit_type::veteran_soldier,
                            e_unit_type::veteran_colonist )
                            .value(),
                        /*inventory=*/{} )
                        .value(),
      },
      UnitTransformation{
        .new_comp = UnitComposition::create(
                        /*type=*/UnitType::create(
                            e_unit_type::continental_army,
                            e_unit_type::veteran_colonist )
                            .value(),
                        /*inventory=*/{} )
                        .value(),
        .modifier_deltas =
            { { e_unit_type_modifier::independence,
                e_unit_type_modifier_delta::add } } },
    };
    adjust_for_independence_status(
        input, /*independence_declared=*/true );
    REQUIRE( FmtVerticalJsonList{ input } ==
             FmtVerticalJsonList{ expected } );

    // Add independence before independence is declared.
    input = {
      UnitTransformation{
        .new_comp = UnitComposition::create(
                        /*type=*/UnitType::create(
                            e_unit_type::veteran_soldier,
                            e_unit_type::veteran_colonist )
                            .value(),
                        /*inventory=*/{} )
                        .value(),
      },
      UnitTransformation{
        .new_comp = UnitComposition::create(
                        /*type=*/UnitType::create(
                            e_unit_type::continental_army,
                            e_unit_type::veteran_colonist )
                            .value(),
                        /*inventory=*/{} )
                        .value(),
        .modifier_deltas =
            { { e_unit_type_modifier::independence,
                e_unit_type_modifier_delta::add } },
      },
    };
    expected = {
      UnitTransformation{
        .new_comp = UnitComposition::create(
                        /*type=*/UnitType::create(
                            e_unit_type::veteran_soldier,
                            e_unit_type::veteran_colonist )
                            .value(),
                        /*inventory=*/{} )
                        .value(),
      },
    };
    adjust_for_independence_status(
        input, /*independence_declared=*/false );
    REQUIRE( FmtVerticalJsonList{ input } ==
             FmtVerticalJsonList{ expected } );

    // Remove independence before independence is declared.
    input = {
      UnitTransformation{
        .new_comp = UnitComposition::create(
                        /*type=*/UnitType::create(
                            e_unit_type::veteran_soldier,
                            e_unit_type::veteran_colonist )
                            .value(),
                        /*inventory=*/{} )
                        .value(),
        .modifier_deltas =
            { { e_unit_type_modifier::independence,
                e_unit_type_modifier_delta::del } },
      },
    };
    expected = {
      UnitTransformation{
        .new_comp = UnitComposition::create(
                        /*type=*/UnitType::create(
                            e_unit_type::veteran_soldier,
                            e_unit_type::veteran_colonist )
                            .value(),
                        /*inventory=*/{} )
                        .value(),
        .modifier_deltas =
            { { e_unit_type_modifier::independence,
                e_unit_type_modifier_delta::del } },
      },
    };
    adjust_for_independence_status(
        input, /*independence_declared=*/false );
    REQUIRE( FmtVerticalJsonList{ input } ==
             FmtVerticalJsonList{ expected } );
  }
  SECTION( "commodity" ) {
    vector<UnitTransformationFromCommodity> input;
    vector<UnitTransformationFromCommodity> expected;

    // Add independence after independence is declared.
    input = {
      UnitTransformationFromCommodity{
        .new_comp = UnitComposition::create(
                        /*type=*/UnitType::create(
                            e_unit_type::veteran_soldier,
                            e_unit_type::veteran_colonist )
                            .value(),
                        /*inventory=*/{} )
                        .value(),
      },
      UnitTransformationFromCommodity{
        .new_comp = UnitComposition::create(
                        /*type=*/UnitType::create(
                            e_unit_type::continental_army,
                            e_unit_type::veteran_colonist )
                            .value(),
                        /*inventory=*/{} )
                        .value(),
        .modifier_deltas =
            { { e_unit_type_modifier::independence,
                e_unit_type_modifier_delta::add } },
      },
    };
    expected = {
      UnitTransformationFromCommodity{
        .new_comp = UnitComposition::create(
                        /*type=*/UnitType::create(
                            e_unit_type::veteran_soldier,
                            e_unit_type::veteran_colonist )
                            .value(),
                        /*inventory=*/{} )
                        .value(),
      },
      UnitTransformationFromCommodity{
        .new_comp = UnitComposition::create(
                        /*type=*/UnitType::create(
                            e_unit_type::continental_army,
                            e_unit_type::veteran_colonist )
                            .value(),
                        /*inventory=*/{} )
                        .value(),
        .modifier_deltas =
            { { e_unit_type_modifier::independence,
                e_unit_type_modifier_delta::add } },
      },
    };
    adjust_for_independence_status(
        input, /*independence_declared=*/true );
    REQUIRE( FmtVerticalJsonList{ input } ==
             FmtVerticalJsonList{ expected } );

    // Add independence before independence is declared.
    input = {
      UnitTransformationFromCommodity{
        .new_comp = UnitComposition::create(
                        /*type=*/UnitType::create(
                            e_unit_type::veteran_soldier,
                            e_unit_type::veteran_colonist )
                            .value(),
                        /*inventory=*/{} )
                        .value(),
      },
      UnitTransformationFromCommodity{
        .new_comp = UnitComposition::create(
                        /*type=*/UnitType::create(
                            e_unit_type::continental_army,
                            e_unit_type::veteran_colonist )
                            .value(),
                        /*inventory=*/{} )
                        .value(),
        .modifier_deltas =
            { { e_unit_type_modifier::independence,
                e_unit_type_modifier_delta::add } },
      },
    };
    expected = {
      UnitTransformationFromCommodity{
        .new_comp = UnitComposition::create(
                        /*type=*/UnitType::create(
                            e_unit_type::veteran_soldier,
                            e_unit_type::veteran_colonist )
                            .value(),
                        /*inventory=*/{} )
                        .value(),
      },
    };
    adjust_for_independence_status(
        input, /*independence_declared=*/false );
    REQUIRE( FmtVerticalJsonList{ input } ==
             FmtVerticalJsonList{ expected } );

    // Remove independence before independence is declared.
    input = {
      UnitTransformationFromCommodity{
        .new_comp = UnitComposition::create(
                        /*type=*/UnitType::create(
                            e_unit_type::veteran_soldier,
                            e_unit_type::veteran_colonist )
                            .value(),
                        /*inventory=*/{} )
                        .value(),
        .modifier_deltas =
            { { e_unit_type_modifier::independence,
                e_unit_type_modifier_delta::del } },
      },
    };
    expected = {
      UnitTransformationFromCommodity{
        .new_comp = UnitComposition::create(
                        /*type=*/UnitType::create(
                            e_unit_type::veteran_soldier,
                            e_unit_type::veteran_colonist )
                            .value(),
                        /*inventory=*/{} )
                        .value(),
        .modifier_deltas =
            { { e_unit_type_modifier::independence,
                e_unit_type_modifier_delta::del } },
      },
    };
    adjust_for_independence_status(
        input, /*independence_declared=*/false );
    REQUIRE( FmtVerticalJsonList{ input } ==
             FmtVerticalJsonList{ expected } );
  }
}

TEST_CASE( "[unit-transformation] strip_to_base_type " ) {
  UnitComposition    comp{};
  UnitTransformation res;
  UnitTransformation expected;

  comp     = UnitComposition( e_unit_type::free_colonist );
  res      = strip_to_base_type( comp );
  expected = UnitTransformation{
    .new_comp = UnitComposition::create(
                    /*type=*/e_unit_type::free_colonist,
                    /*inventory=*/{} )
                    .value(),
    .modifier_deltas  = {},
    .commodity_deltas = {},
  };
  REQUIRE( res == expected );

  comp     = UnitComposition( e_unit_type::expert_farmer );
  res      = strip_to_base_type( comp );
  expected = UnitTransformation{
    .new_comp = UnitComposition::create(
                    /*type=*/e_unit_type::expert_farmer,
                    /*inventory=*/{} )
                    .value(),
    .modifier_deltas  = {},
    .commodity_deltas = {},
  };
  REQUIRE( res == expected );

  comp = UnitComposition(
      UnitType::create( e_unit_type::dragoon,
                        e_unit_type::indentured_servant )
          .value() );
  res      = strip_to_base_type( comp );
  expected = UnitTransformation{
    .new_comp = UnitComposition::create(
                    /*type=*/e_unit_type::indentured_servant,
                    /*inventory=*/{} )
                    .value(),
    .modifier_deltas  = { { e_unit_type_modifier::horses,
                            e_unit_type_modifier_delta::del },
                          { e_unit_type_modifier::muskets,
                            e_unit_type_modifier_delta::del } },
    .commodity_deltas = { { e_commodity::horses, 50 },
                          { e_commodity::muskets, 50 } },
  };
  REQUIRE( res == expected );

  comp     = UnitComposition( e_unit_type::veteran_dragoon );
  res      = strip_to_base_type( comp );
  expected = UnitTransformation{
    .new_comp = UnitComposition::create(
                    /*type=*/e_unit_type::veteran_colonist,
                    /*inventory=*/{} )
                    .value(),
    .modifier_deltas  = { { e_unit_type_modifier::horses,
                            e_unit_type_modifier_delta::del },
                          { e_unit_type_modifier::muskets,
                            e_unit_type_modifier_delta::del } },
    .commodity_deltas = { { e_commodity::horses, 50 },
                          { e_commodity::muskets, 50 } },
  };
  REQUIRE( res == expected );

  comp = UnitComposition::create(
             UnitType::create( e_unit_type::pioneer,
                               e_unit_type::free_colonist )
                 .value(),
             /*inventory=*/{ { e_unit_inventory::tools, 80 } } )
             .value();
  res      = strip_to_base_type( comp );
  expected = UnitTransformation{
    .new_comp = UnitComposition::create(
                    /*type=*/e_unit_type::free_colonist,
                    /*inventory=*/{} )
                    .value(),
    .modifier_deltas  = { { e_unit_type_modifier::tools,
                            e_unit_type_modifier_delta::del } },
    .commodity_deltas = { { e_commodity::tools, 80 } },
  };
  REQUIRE( res == expected );

  comp = UnitComposition::create(
             e_unit_type::hardy_pioneer,
             /*inventory=*/{ { e_unit_inventory::tools, 100 } } )
             .value();
  res      = strip_to_base_type( comp );
  expected = UnitTransformation{
    .new_comp = UnitComposition::create(
                    /*type=*/e_unit_type::hardy_colonist,
                    /*inventory=*/{} )
                    .value(),
    .modifier_deltas  = { { e_unit_type_modifier::tools,
                            e_unit_type_modifier_delta::del } },
    .commodity_deltas = { { e_commodity::tools, 100 } },
  };
  REQUIRE( res == expected );
}

TEST_CASE( "[unit-transformation] query_unit_transformation" ) {
  UnitComposition           from{};
  UnitComposition           to{};
  maybe<UnitTransformation> res;
  maybe<UnitTransformation> expected;

  {
    from     = UnitComposition( e_unit_type::free_colonist );
    to       = UnitComposition( e_unit_type::free_colonist );
    res      = query_unit_transformation( from, to );
    expected = UnitTransformation{
      .new_comp         = to,
      .modifier_deltas  = {},
      .commodity_deltas = {},
    };
    REQUIRE( res == expected );

    swap( from, to );

    res      = query_unit_transformation( from, to );
    expected = UnitTransformation{
      .new_comp         = to,
      .modifier_deltas  = {},
      .commodity_deltas = {},
    };
    REQUIRE( res == expected );
  }

  {
    from     = UnitComposition( e_unit_type::expert_farmer );
    to       = UnitComposition( e_unit_type::expert_farmer );
    res      = query_unit_transformation( from, to );
    expected = UnitTransformation{
      .new_comp         = to,
      .modifier_deltas  = {},
      .commodity_deltas = {},
    };
    REQUIRE( res == expected );

    swap( from, to );

    res      = query_unit_transformation( from, to );
    expected = UnitTransformation{
      .new_comp         = to,
      .modifier_deltas  = {},
      .commodity_deltas = {},
    };
    REQUIRE( res == expected );
  }

  {
    from     = UnitComposition( e_unit_type::free_colonist );
    to       = UnitComposition( e_unit_type::missionary );
    res      = query_unit_transformation( from, to );
    expected = UnitTransformation{
      .new_comp         = to,
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = {},
    };
    REQUIRE( res == expected );

    swap( from, to );

    res      = query_unit_transformation( from, to );
    expected = UnitTransformation{
      .new_comp         = to,
      .modifier_deltas  = { { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = {},
    };
    REQUIRE( res == expected );
  }

  {
    from     = UnitComposition( e_unit_type::soldier );
    to       = UnitComposition( e_unit_type::missionary );
    res      = query_unit_transformation( from, to );
    expected = UnitTransformation{
      .new_comp         = to,
      .modifier_deltas  = { { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::blessing,
                              e_unit_type_modifier_delta::add } },
      .commodity_deltas = { { e_commodity::muskets, 50 } },
    };
    REQUIRE( res == expected );

    swap( from, to );

    res      = query_unit_transformation( from, to );
    expected = nothing;
    REQUIRE( res == expected );
  }

  {
    from = UnitComposition(
        UnitType::create( e_unit_type::dragoon,
                          e_unit_type::indentured_servant )
            .value() );
    to  = UnitComposition( e_unit_type::indentured_servant );
    res = query_unit_transformation( from, to );
    expected = UnitTransformation{
      .new_comp         = to,
      .modifier_deltas  = { { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::horses, 50 },
                            { e_commodity::muskets, 50 } },
    };
    REQUIRE( res == expected );

    swap( from, to );

    res      = query_unit_transformation( from, to );
    expected = nothing;
    REQUIRE( res == expected );
  }

  {
    from = UnitComposition(
        UnitType::create( e_unit_type::dragoon,
                          e_unit_type::indentured_servant )
            .value() );
    to       = UnitComposition( e_unit_type::free_colonist );
    res      = query_unit_transformation( from, to );
    expected = nothing;
    REQUIRE( res == expected );

    swap( from, to );

    res      = query_unit_transformation( from, to );
    expected = nothing;
    REQUIRE( res == expected );
  }

  {
    from = UnitComposition(
        UnitType::create( e_unit_type::dragoon,
                          e_unit_type::indentured_servant )
            .value() );
    to       = UnitComposition( e_unit_type::veteran_dragoon );
    res      = query_unit_transformation( from, to );
    expected = nothing;
    REQUIRE( res == expected );

    swap( from, to );

    res      = query_unit_transformation( from, to );
    expected = nothing;
    REQUIRE( res == expected );
  }

  {
    from     = UnitComposition( e_unit_type::dragoon );
    to       = UnitComposition( e_unit_type::veteran_dragoon );
    res      = query_unit_transformation( from, to );
    expected = nothing;
    REQUIRE( res == expected );

    swap( from, to );

    res      = query_unit_transformation( from, to );
    expected = nothing;
    REQUIRE( res == expected );
  }

  {
    from = UnitComposition(
        UnitType::create( e_unit_type::dragoon,
                          e_unit_type::indentured_servant )
            .value() );
    to = UnitComposition(
        UnitType::create( e_unit_type::soldier,
                          e_unit_type::indentured_servant )
            .value() );
    res      = query_unit_transformation( from, to );
    expected = UnitTransformation{
      .new_comp         = to,
      .modifier_deltas  = { { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::horses, 50 } },
    };
    REQUIRE( res == expected );

    swap( from, to );

    res      = query_unit_transformation( from, to );
    expected = nothing;
    REQUIRE( res == expected );
  }

  {
    from = UnitComposition(
        UnitType::create( e_unit_type::dragoon,
                          e_unit_type::indentured_servant )
            .value() );
    to = UnitComposition(
        UnitType::create( e_unit_type::scout,
                          e_unit_type::indentured_servant )
            .value() );
    res      = query_unit_transformation( from, to );
    expected = UnitTransformation{
      .new_comp         = to,
      .modifier_deltas  = { { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::muskets, 50 } },
    };
    REQUIRE( res == expected );

    swap( from, to );

    res      = query_unit_transformation( from, to );
    expected = nothing;
    REQUIRE( res == expected );
  }

  {
    from     = UnitComposition( e_unit_type::veteran_dragoon );
    to       = UnitComposition( e_unit_type::veteran_colonist );
    res      = query_unit_transformation( from, to );
    expected = UnitTransformation{
      .new_comp         = to,
      .modifier_deltas  = { { e_unit_type_modifier::horses,
                              e_unit_type_modifier_delta::del },
                            { e_unit_type_modifier::muskets,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::horses, 50 },
                            { e_commodity::muskets, 50 } },
    };
    REQUIRE( res == expected );

    swap( from, to );

    res      = query_unit_transformation( from, to );
    expected = nothing;
    REQUIRE( res == expected );
  }

  {
    from =
        UnitComposition::create(
            UnitType::create( e_unit_type::pioneer,
                              e_unit_type::free_colonist )
                .value(),
            /*inventory=*/{ { e_unit_inventory::tools, 80 } } )
            .value();
    to       = UnitComposition( e_unit_type::free_colonist );
    res      = query_unit_transformation( from, to );
    expected = UnitTransformation{
      .new_comp         = to,
      .modifier_deltas  = { { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::tools, 80 } },
    };
    REQUIRE( res == expected );

    swap( from, to );

    res      = query_unit_transformation( from, to );
    expected = nothing;
    REQUIRE( res == expected );
  }

  {
    from =
        UnitComposition::create(
            UnitType::create( e_unit_type::pioneer,
                              e_unit_type::free_colonist )
                .value(),
            /*inventory=*/{ { e_unit_inventory::tools, 80 } } )
            .value();
    to = UnitComposition::create(
             UnitType::create( e_unit_type::pioneer,
                               e_unit_type::free_colonist )
                 .value(),
             /*inventory=*/{ { e_unit_inventory::tools, 80 } } )
             .value();
    res      = query_unit_transformation( from, to );
    expected = UnitTransformation{
      .new_comp         = to,
      .modifier_deltas  = {},
      .commodity_deltas = {},
    };
    REQUIRE( res == expected );

    swap( from, to );

    res      = query_unit_transformation( from, to );
    expected = UnitTransformation{
      .new_comp         = to,
      .modifier_deltas  = {},
      .commodity_deltas = {},
    };
    REQUIRE( res == expected );
  }

  {
    from =
        UnitComposition::create(
            e_unit_type::hardy_pioneer,
            /*inventory=*/{ { e_unit_inventory::tools, 100 } } )
            .value();
    to       = UnitComposition( e_unit_type::petty_criminal );
    res      = query_unit_transformation( from, to );
    expected = nothing;
    REQUIRE( res == expected );

    swap( from, to );

    res      = query_unit_transformation( from, to );
    expected = nothing;
    REQUIRE( res == expected );
  }

  {
    from =
        UnitComposition::create(
            e_unit_type::hardy_pioneer,
            /*inventory=*/{ { e_unit_inventory::tools, 100 } } )
            .value();
    to       = UnitComposition( e_unit_type::hardy_colonist );
    res      = query_unit_transformation( from, to );
    expected = UnitTransformation{
      .new_comp         = to,
      .modifier_deltas  = { { e_unit_type_modifier::tools,
                              e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::tools, 100 } },
    };
    REQUIRE( res == expected );

    swap( from, to );

    res      = query_unit_transformation( from, to );
    expected = nothing;
    REQUIRE( res == expected );
  }
}

TEST_CASE( "[unit-transformation] unit_lose_commodity" ) {
  UnitComposition                         comp{};
  Commodity                               comm;
  vector<UnitTransformationFromCommodity> res;
  vector<UnitTransformationFromCommodity> expected;

  // hardy_pioneer with 20 tools - 20 tools.
  comp = UnitComposition::create(
             e_unit_type::hardy_pioneer,
             /*inventory=*/{ { e_unit_inventory::tools, 20 } } )
             .value();
  comm = { .type = e_commodity::tools, .quantity = 20 };
  res  = unit_lose_commodity( comp, comm );

  expected = {
    UnitTransformationFromCommodity{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::hardy_colonist,
                          e_unit_type::hardy_colonist )
                          .value(),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas = { { e_unit_type_modifier::tools,
                             e_unit_type_modifier_delta::del } },
      .quantity_used   = -20,
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // hardy_pioneer with 80 tools - 20 tools.
  comp = UnitComposition::create(
             e_unit_type::hardy_pioneer,
             /*inventory=*/{ { e_unit_inventory::tools, 80 } } )
             .value();
  comm     = { .type = e_commodity::tools, .quantity = 20 };
  res      = unit_lose_commodity( comp, comm );
  expected = {
    UnitTransformationFromCommodity{
      .new_comp =
          UnitComposition::create(
              /*type=*/UnitType::create(
                  e_unit_type::hardy_pioneer,
                  e_unit_type::hardy_colonist )
                  .value(),
              /*inventory=*/{ { e_unit_inventory::tools, 60 } } )
              .value(),
      .modifier_deltas = {},
      .quantity_used   = -20,
    },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );
}

TEST_CASE( "[unit-transformation] consume_20_tools pioneer" ) {
  World           W;
  UnitComposition comp = e_unit_type::pioneer;

  Unit& unit = W.units().unit_for(
      create_free_unit( W.units(), W.default_player(), comp ) );

  // Initially.
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 100 );
  // Consume.
  consume_20_tools( W.ss(), W.ts(), unit );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 80 );
  // Consume.
  consume_20_tools( W.ss(), W.ts(), unit );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 60 );
  // Consume.
  consume_20_tools( W.ss(), W.ts(), unit );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );
  // Consume.
  consume_20_tools( W.ss(), W.ts(), unit );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
  // Consume.
  consume_20_tools( W.ss(), W.ts(), unit );
  REQUIRE( unit.type() == e_unit_type::free_colonist );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 0 );
}

TEST_CASE( "[unit] consume_20_tools hardy_pioneer" ) {
  World           W;
  UnitComposition comp = e_unit_type::hardy_pioneer;

  Unit& unit = W.units().unit_for(
      create_free_unit( W.units(), W.default_player(), comp ) );

  // Initially.
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 100 );
  // Consume.
  consume_20_tools( W.ss(), W.ts(), unit );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 80 );
  // Consume.
  consume_20_tools( W.ss(), W.ts(), unit );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 60 );
  // Consume.
  consume_20_tools( W.ss(), W.ts(), unit );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );
  // Consume.
  consume_20_tools( W.ss(), W.ts(), unit );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
  // Consume.
  consume_20_tools( W.ss(), W.ts(), unit );
  REQUIRE( unit.type() == e_unit_type::hardy_colonist );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 0 );
}

} // namespace
} // namespace rn
