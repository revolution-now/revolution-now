/****************************************************************
**unit-composer.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-04.
*
* Description: Unit tests for the src/unit-composer.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/ss/unit-composer.hpp"

// Revolution Now
#include "src/lua.hpp"

// luapp
#include "src/luapp/state.hpp"

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

void sort_by_new_type( vector<UnitTransformationResult>& v ) {
  sort( v.begin(), v.end(), []( auto const& l, auto const& r ) {
    return static_cast<int>( l.new_comp.type() ) <
           static_cast<int>( r.new_comp.type() );
  } );
}

void sort_by_new_type(
    vector<UnitTransformationFromCommodityResult>& v ) {
  sort( v.begin(), v.end(), []( auto const& l, auto const& r ) {
    return static_cast<int>( l.new_comp.type() ) <
           static_cast<int>( r.new_comp.type() );
  } );
}

TEST_CASE( "[unit-composer] operator[]" ) {
  auto ut       = UnitType::create( e_unit_type::pioneer );
  auto maybe_uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 80 } } );
  REQUIRE( maybe_uc.has_value() );
  UnitComposition const& uc = *maybe_uc;
  REQUIRE( uc[e_unit_inventory::tools] == 80 );
  REQUIRE( uc[e_unit_inventory::gold] == 0 );
}

TEST_CASE( "[unit-composer] pioneer tool count" ) {
  auto ut = UnitType::create( e_unit_type::pioneer );
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
  ut = UnitType::create( e_unit_type::hardy_pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 120 } } );
  REQUIRE( !uc.has_value() );

  ut = UnitType::create( e_unit_type::pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 105 } } );
  REQUIRE( !uc.has_value() );
  ut = UnitType::create( e_unit_type::hardy_pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 105 } } );
  REQUIRE( !uc.has_value() );

  ut = UnitType::create( e_unit_type::pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 100 } } );
  REQUIRE( uc.has_value() );
  ut = UnitType::create( e_unit_type::hardy_pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 100 } } );
  REQUIRE( uc.has_value() );

  ut = UnitType::create( e_unit_type::pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 90 } } );
  REQUIRE( !uc.has_value() );
  ut = UnitType::create( e_unit_type::hardy_pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 90 } } );
  REQUIRE( !uc.has_value() );

  ut = UnitType::create( e_unit_type::pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 80 } } );
  REQUIRE( uc.has_value() );
  ut = UnitType::create( e_unit_type::hardy_pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 80 } } );
  REQUIRE( uc.has_value() );

  ut = UnitType::create( e_unit_type::pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 20 } } );
  REQUIRE( uc.has_value() );
  ut = UnitType::create( e_unit_type::hardy_pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 20 } } );
  REQUIRE( uc.has_value() );

  ut = UnitType::create( e_unit_type::pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 15 } } );
  REQUIRE( !uc.has_value() );
  ut = UnitType::create( e_unit_type::hardy_pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 15 } } );
  REQUIRE( !uc.has_value() );

  ut = UnitType::create( e_unit_type::pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 0 } } );
  REQUIRE( !uc.has_value() );
  ut = UnitType::create( e_unit_type::hardy_pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, 0 } } );
  REQUIRE( !uc.has_value() );

  ut = UnitType::create( e_unit_type::pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, -20 } } );
  REQUIRE( !uc.has_value() );
  ut = UnitType::create( e_unit_type::hardy_pioneer );
  uc = UnitComposition::create(
      ut, /*inventory=*/{ { e_unit_inventory::tools, -20 } } );
  REQUIRE( !uc.has_value() );
}

TEST_CASE(
    "[unit-composer] possible_unit_transformations "
    "no commodities available" ) {
  // These remain empty for this test.
  unordered_map<e_commodity, int> comms;

  UnitComposition                  comp{};
  vector<UnitTransformationResult> res;
  vector<UnitTransformationResult> expected;

  // free_colonist.
  comp = UnitComposition::create( e_unit_type::free_colonist );
  res  = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::free_colonist ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas  = {},
          .commodity_deltas = {},
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::missionary,
                              e_unit_type::free_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = {},
      } };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // missionary.
  comp     = UnitComposition::create( e_unit_type::missionary );
  res      = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::free_colonist ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = {},
      },
      UnitTransformationResult{
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
  comp =
      UnitComposition::create( e_unit_type::jesuit_missionary );
  res      = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::jesuit_colonist ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = {},
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::jesuit_missionary ),
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
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::indentured_servant ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::tools, 100 } },
      },
      UnitTransformationResult{
          .new_comp =
              UnitComposition::create(
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
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::missionary,
                              e_unit_type::indentured_servant )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
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
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::hardy_colonist ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::tools, 80 } },
      },
      UnitTransformationResult{
          .new_comp =
              UnitComposition::create(
                  /*type=*/UnitType::create(
                      e_unit_type::hardy_pioneer,
                      e_unit_type::hardy_colonist )
                      .value(),
                  /*inventory=*/{ { e_unit_inventory::tools,
                                    80 } } )
                  .value(),
          .modifier_deltas  = {},
          .commodity_deltas = {},
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::missionary,
                              e_unit_type::hardy_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::tools, 80 } },
      } };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  comp     = UnitComposition::create( e_unit_type::soldier );
  res      = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::free_colonist ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::muskets, 50 } },
      },
      UnitTransformationResult{
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
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::missionary,
                              e_unit_type::free_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
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
  comp = UnitComposition::create(
      UnitType::create( e_unit_type::soldier,
                        e_unit_type::petty_criminal )
          .value() );
  res      = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::petty_criminal ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::muskets, 50 } },
      },
      UnitTransformationResult{
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
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::missionary,
                              e_unit_type::petty_criminal )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
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
  comp = UnitComposition::create( e_unit_type::veteran_dragoon );
  res  = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::missionary,
                              e_unit_type::veteran_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::horses, 50 },
                                { e_commodity::muskets, 50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::scout,
                              e_unit_type::veteran_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::muskets, 50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::veteran_colonist ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::horses, 50 },
                                { e_commodity::muskets, 50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::veteran_soldier,
                              e_unit_type::veteran_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::horses, 50 } },
      },
      UnitTransformationResult{
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
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::continental_army,
                              e_unit_type::veteran_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::independence,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::horses, 50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::continental_cavalry,
                              e_unit_type::veteran_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::independence,
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
    "[unit-composer] possible_unit_transformations full "
    "commodities available" ) {
  // This will remain full for this test.
  unordered_map<e_commodity, int> comms;
  for( e_commodity c : refl::enum_values<e_commodity> )
    comms[c] = 100;

  UnitComposition                  comp{};
  vector<UnitTransformationResult> res;
  vector<UnitTransformationResult> expected;

  // free_colonist.
  comp = UnitComposition::create( e_unit_type::free_colonist );
  res  = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::free_colonist ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas  = {},
          .commodity_deltas = {},
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::soldier,
                              e_unit_type::free_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::muskets, -50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::dragoon,
                              e_unit_type::free_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::muskets, -50 },
                                { e_commodity::horses, -50 } },
      },
      UnitTransformationResult{
          .new_comp =
              UnitComposition::create(
                  /*type=*/UnitType::create(
                      e_unit_type::pioneer,
                      e_unit_type::free_colonist )
                      .value(),
                  /*inventory=*/{ { e_unit_inventory::tools,
                                    100 } } )
                  .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::tools, -100 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::missionary,
                              e_unit_type::free_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = {},
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::scout,
                              e_unit_type::free_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::horses, -50 } },
      },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // missionary.
  comp     = UnitComposition::create( e_unit_type::missionary );
  res      = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::free_colonist ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = {},
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::soldier,
                              e_unit_type::free_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::muskets, -50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::dragoon,
                              e_unit_type::free_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::muskets, -50 },
                                { e_commodity::horses, -50 } },
      },
      UnitTransformationResult{
          .new_comp =
              UnitComposition::create(
                  /*type=*/UnitType::create(
                      e_unit_type::pioneer,
                      e_unit_type::free_colonist )
                      .value(),
                  /*inventory=*/{ { e_unit_inventory::tools,
                                    100 } } )
                  .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::tools, -100 } },
      },
      UnitTransformationResult{
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
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::scout,
                              e_unit_type::free_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
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
  comp =
      UnitComposition::create( e_unit_type::jesuit_missionary );
  res      = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::jesuit_colonist ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = {},
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::soldier,
                              e_unit_type::jesuit_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::muskets, -50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::dragoon,
                              e_unit_type::jesuit_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::muskets, -50 },
                                { e_commodity::horses, -50 } },
      },
      UnitTransformationResult{
          .new_comp =
              UnitComposition::create(
                  /*type=*/UnitType::create(
                      e_unit_type::pioneer,
                      e_unit_type::jesuit_colonist )
                      .value(),
                  /*inventory=*/{ { e_unit_inventory::tools,
                                    100 } } )
                  .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::tools, -100 } },
      },
      UnitTransformationResult{
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
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::scout,
                              e_unit_type::jesuit_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
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
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::indentured_servant ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::tools, 100 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/
                          UnitType::create(
                              e_unit_type::soldier,
                              e_unit_type::indentured_servant )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::muskets, -50 },
                                { e_commodity::tools, 100 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/
                          UnitType::create(
                              e_unit_type::dragoon,
                              e_unit_type::indentured_servant )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::muskets, -50 },
                                { e_commodity::tools, 100 },
                                { e_commodity::horses, -50 } },
      },
      UnitTransformationResult{
          .new_comp =
              UnitComposition::create(
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
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::missionary,
                              e_unit_type::indentured_servant )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::tools, 100 } },
      },
      UnitTransformationResult{
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
      UnitTransformationResult{
          .new_comp =
              UnitComposition::create(
                  /*type=*/
                  UnitType::create( e_unit_type::soldier,
                                    e_unit_type::hardy_colonist )
                      .value(),
                  /*inventory=*/{} )
                  .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::muskets, -50 },
                                { e_commodity::tools, 80 } },
      },
      UnitTransformationResult{
          .new_comp =
              UnitComposition::create(
                  /*type=*/
                  UnitType::create( e_unit_type::dragoon,
                                    e_unit_type::hardy_colonist )
                      .value(),
                  /*inventory=*/{} )
                  .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::muskets, -50 },
                                { e_commodity::tools, 80 },
                                { e_commodity::horses, -50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::missionary,
                              e_unit_type::hardy_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::tools, 80 } },
      },
      UnitTransformationResult{
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
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::hardy_colonist ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::tools, 80 } },
      },
      UnitTransformationResult{
          .new_comp =
              UnitComposition::create(
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

  comp     = UnitComposition::create( e_unit_type::soldier );
  res      = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::free_colonist ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::muskets, 50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::dragoon,
                              e_unit_type::free_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::horses, -50 } },
      },
      UnitTransformationResult{
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
      UnitTransformationResult{
          .new_comp =
              UnitComposition::create(
                  /*type=*/UnitType::create(
                      e_unit_type::pioneer,
                      e_unit_type::free_colonist )
                      .value(),
                  /*inventory=*/{ { e_unit_inventory::tools,
                                    100 } } )
                  .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::tools, -100 },
                                { e_commodity::muskets, 50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::scout,
                              e_unit_type::free_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::muskets, 50 },
                                { e_commodity::horses, -50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::missionary,
                              e_unit_type::free_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
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
  comp = UnitComposition::create(
      UnitType::create( e_unit_type::soldier,
                        e_unit_type::petty_criminal )
          .value() );
  res      = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::petty_criminal ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::muskets, 50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::dragoon,
                              e_unit_type::petty_criminal )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::horses, -50 } },
      },
      UnitTransformationResult{
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
      UnitTransformationResult{
          .new_comp =
              UnitComposition::create(
                  /*type=*/UnitType::create(
                      e_unit_type::pioneer,
                      e_unit_type::petty_criminal )
                      .value(),
                  /*inventory=*/{ { e_unit_inventory::tools,
                                    100 } } )
                  .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::tools, -100 },
                                { e_commodity::muskets, 50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::scout,
                              e_unit_type::petty_criminal )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::muskets, 50 },
                                { e_commodity::horses, -50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::missionary,
                              e_unit_type::petty_criminal )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::muskets, 50 } },
      } };

  // Veteran Dragoon.
  comp = UnitComposition::create( e_unit_type::veteran_dragoon );
  res  = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp =
              UnitComposition::create(
                  /*type=*/UnitType::create(
                      e_unit_type::pioneer,
                      e_unit_type::veteran_colonist )
                      .value(),
                  /*inventory=*/{ { e_unit_inventory::tools,
                                    100 } } )
                  .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::muskets, 50 },
                                { e_commodity::horses, 50 },
                                { e_commodity::tools, -100 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::missionary,
                              e_unit_type::veteran_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::horses, 50 },
                                { e_commodity::muskets, 50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::scout,
                              e_unit_type::veteran_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::muskets, 50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::veteran_colonist ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::horses, 50 },
                                { e_commodity::muskets, 50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::veteran_soldier,
                              e_unit_type::veteran_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::horses, 50 } },
      },
      UnitTransformationResult{
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
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::continental_army,
                              e_unit_type::veteran_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::independence,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::horses, 50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::continental_cavalry,
                              e_unit_type::veteran_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::independence,
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
    "[unit-composer] possible_unit_transformations partial "
    "commodities available" ) {
  // This will remain full for this test.
  unordered_map<e_commodity, int> comms;
  for( e_commodity c : refl::enum_values<e_commodity> )
    comms[c] = 40;

  UnitComposition                  comp{};
  vector<UnitTransformationResult> res;
  vector<UnitTransformationResult> expected;

  // free_colonist.
  comp = UnitComposition::create( e_unit_type::free_colonist );
  res  = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::free_colonist ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas  = {},
          .commodity_deltas = {},
      },
      UnitTransformationResult{
          .new_comp =
              UnitComposition::create(
                  /*type=*/UnitType::create(
                      e_unit_type::pioneer,
                      e_unit_type::free_colonist )
                      .value(),
                  /*inventory=*/{ { e_unit_inventory::tools,
                                    40 } } )
                  .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::tools, -40 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::missionary,
                              e_unit_type::free_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = {},
      },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // missionary.
  comp     = UnitComposition::create( e_unit_type::missionary );
  res      = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::free_colonist ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = {},
      },
      UnitTransformationResult{
          .new_comp =
              UnitComposition::create(
                  /*type=*/UnitType::create(
                      e_unit_type::pioneer,
                      e_unit_type::free_colonist )
                      .value(),
                  /*inventory=*/{ { e_unit_inventory::tools,
                                    40 } } )
                  .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::tools, -40 } },
      },
      UnitTransformationResult{
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
  comp =
      UnitComposition::create( e_unit_type::jesuit_missionary );
  res      = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::jesuit_colonist ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = {},
      },
      UnitTransformationResult{
          .new_comp =
              UnitComposition::create(
                  /*type=*/UnitType::create(
                      e_unit_type::pioneer,
                      e_unit_type::jesuit_colonist )
                      .value(),
                  /*inventory=*/{ { e_unit_inventory::tools,
                                    40 } } )
                  .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::tools, -40 } },
      },
      UnitTransformationResult{
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
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::indentured_servant ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::tools, 100 } },
      },
      UnitTransformationResult{
          .new_comp =
              UnitComposition::create(
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
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::missionary,
                              e_unit_type::indentured_servant )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
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
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::missionary,
                              e_unit_type::hardy_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::tools, 80 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::hardy_colonist ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::tools, 80 } },
      },
      UnitTransformationResult{
          .new_comp =
              UnitComposition::create(
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
  comp     = UnitComposition::create( e_unit_type::soldier );
  res      = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::free_colonist ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::muskets, 50 } },
      },
      UnitTransformationResult{
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
      UnitTransformationResult{
          .new_comp =
              UnitComposition::create(
                  /*type=*/UnitType::create(
                      e_unit_type::pioneer,
                      e_unit_type::free_colonist )
                      .value(),
                  /*inventory=*/{ { e_unit_inventory::tools,
                                    40 } } )
                  .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::tools, -40 },
                                { e_commodity::muskets, 50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::missionary,
                              e_unit_type::free_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
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
  comp = UnitComposition::create(
      UnitType::create( e_unit_type::soldier,
                        e_unit_type::petty_criminal )
          .value() );
  res      = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::petty_criminal ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::muskets, 50 } },
      },
      UnitTransformationResult{
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
      UnitTransformationResult{
          .new_comp =
              UnitComposition::create(
                  /*type=*/UnitType::create(
                      e_unit_type::pioneer,
                      e_unit_type::petty_criminal )
                      .value(),
                  /*inventory=*/{ { e_unit_inventory::tools,
                                    40 } } )
                  .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::tools, -40 },
                                { e_commodity::muskets, 50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::missionary,
                              e_unit_type::petty_criminal )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::muskets, 50 } },
      } };

  // Veteran Dragoon.
  comp = UnitComposition::create( e_unit_type::veteran_dragoon );
  res  = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp =
              UnitComposition::create(
                  /*type=*/UnitType::create(
                      e_unit_type::pioneer,
                      e_unit_type::veteran_colonist )
                      .value(),
                  /*inventory=*/{ { e_unit_inventory::tools,
                                    40 } } )
                  .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::muskets, 50 },
                                { e_commodity::horses, 50 },
                                { e_commodity::tools, -40 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::missionary,
                              e_unit_type::veteran_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::horses, 50 },
                                { e_commodity::muskets, 50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::scout,
                              e_unit_type::veteran_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::muskets, 50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::veteran_colonist ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::horses, 50 },
                                { e_commodity::muskets, 50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::veteran_soldier,
                              e_unit_type::veteran_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::horses, 50 } },
      },
      UnitTransformationResult{
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
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::continental_army,
                              e_unit_type::veteran_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::independence,
                  e_unit_type_modifier_delta::add },
                { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::horses, 50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::continental_cavalry,
                              e_unit_type::veteran_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::independence,
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
    "[unit-composer] possible_unit_transformations full "
    "commodities available, misc types" ) {
  // This will remain full for this test.
  unordered_map<e_commodity, int> comms;
  for( e_commodity c : refl::enum_values<e_commodity> )
    comms[c] = 100;

  UnitComposition                  comp{};
  vector<UnitTransformationResult> res;
  vector<UnitTransformationResult> expected;

  // regular.
  comp     = UnitComposition::create( e_unit_type::regular );
  res      = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::regular ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas  = {},
          .commodity_deltas = {},
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::cavalry,
                              e_unit_type::regular )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = { { e_commodity::horses, -50 } },
      },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // cavalry.
  comp     = UnitComposition::create( e_unit_type::cavalry );
  res      = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::regular ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = { { e_commodity::horses, 50 } },
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::cavalry,
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
  comp     = UnitComposition::create( e_unit_type::artillery );
  res      = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
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
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::damaged_artillery ),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::strength,
                  e_unit_type_modifier_delta::del } },
          .commodity_deltas = {},
      },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // damaged_artillery.
  comp =
      UnitComposition::create( e_unit_type::damaged_artillery );
  res      = possible_unit_transformations( comp, comms );
  expected = {
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::artillery,
                              e_unit_type::damaged_artillery )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::strength,
                  e_unit_type_modifier_delta::add } },
          .commodity_deltas = {},
      },
      UnitTransformationResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::damaged_artillery ),
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

TEST_CASE( "[unit-composer] unit_receive_commodity" ) {
  UnitComposition                               comp{};
  Commodity                                     comm;
  vector<UnitTransformationFromCommodityResult> res;
  vector<UnitTransformationFromCommodityResult> expected;

  // free_colonist + 40 muskets.
  comp = UnitComposition::create( e_unit_type::free_colonist );
  comm = { .type = e_commodity::muskets, .quantity = 40 };
  res  = unit_receive_commodity( comp, comm );
  expected = {};
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // free_colonist + 50 muskets.
  comp = UnitComposition::create( e_unit_type::free_colonist );
  comm = { .type = e_commodity::muskets, .quantity = 50 };
  res  = unit_receive_commodity( comp, comm );
  expected = {
      UnitTransformationFromCommodityResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::soldier,
                              e_unit_type::free_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::muskets,
                  e_unit_type_modifier_delta::add } },
          .quantity_used = 50,
      },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // soldier + 50 muskets.
  comp     = UnitComposition::create( e_unit_type::soldier );
  comm     = { .type = e_commodity::muskets, .quantity = 50 };
  res      = unit_receive_commodity( comp, comm );
  expected = {};
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // indentured_servant + 100 horses.
  comp =
      UnitComposition::create( e_unit_type::indentured_servant );
  comm     = { .type = e_commodity::horses, .quantity = 100 };
  res      = unit_receive_commodity( comp, comm );
  expected = {
      UnitTransformationFromCommodityResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::scout,
                              e_unit_type::indentured_servant )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::horses,
                  e_unit_type_modifier_delta::add } },
          .quantity_used = 50,
      },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // hardy_colonist + 50 tools.
  comp = UnitComposition::create( e_unit_type::hardy_colonist );
  comm = { .type = e_commodity::tools, .quantity = 50 };
  res  = unit_receive_commodity( comp, comm );
  expected = {
      UnitTransformationFromCommodityResult{
          .new_comp =
              UnitComposition::create(
                  /*type=*/UnitType::create(
                      e_unit_type::hardy_pioneer,
                      e_unit_type::hardy_colonist )
                      .value(),
                  /*inventory=*/{ { e_unit_inventory::tools,
                                    40 } } )
                  .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::add } },
          .quantity_used = 40,
      },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // hardy_colonist + 120 tools.
  comp = UnitComposition::create( e_unit_type::hardy_colonist );
  comm = { .type = e_commodity::tools, .quantity = 120 };
  res  = unit_receive_commodity( comp, comm );
  expected = {
      UnitTransformationFromCommodityResult{
          .new_comp =
              UnitComposition::create(
                  /*type=*/UnitType::create(
                      e_unit_type::hardy_pioneer,
                      e_unit_type::hardy_colonist )
                      .value(),
                  /*inventory=*/{ { e_unit_inventory::tools,
                                    100 } } )
                  .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::add } },
          .quantity_used = 100,
      },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // hardy_pioneer with 20 tools + 20 tools.
  comp = UnitComposition::create(
             UnitType::create( e_unit_type::hardy_pioneer ),
             /*inventory=*/{ { e_unit_inventory::tools, 20 } } )
             .value();
  comm     = { .type = e_commodity::tools, .quantity = 20 };
  res      = unit_receive_commodity( comp, comm );
  expected = {
      UnitTransformationFromCommodityResult{
          .new_comp =
              UnitComposition::create(
                  /*type=*/UnitType::create(
                      e_unit_type::hardy_pioneer,
                      e_unit_type::hardy_colonist )
                      .value(),
                  /*inventory=*/{ { e_unit_inventory::tools,
                                    40 } } )
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
             UnitType::create( e_unit_type::hardy_pioneer ),
             /*inventory=*/{ { e_unit_inventory::tools, 80 } } )
             .value();
  comm     = { .type = e_commodity::tools, .quantity = 45 };
  res      = unit_receive_commodity( comp, comm );
  expected = {
      UnitTransformationFromCommodityResult{
          .new_comp =
              UnitComposition::create(
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
             UnitType::create( e_unit_type::hardy_pioneer ),
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

TEST_CASE( "[unit-composer] adjust_for_independence_status" ) {
  SECTION( "general" ) {
    vector<UnitTransformationResult> input;
    vector<UnitTransformationResult> expected;

    // Add independence after independence is declared.
    input = {
        UnitTransformationResult{
            .new_comp = UnitComposition::create(
                            /*type=*/UnitType::create(
                                e_unit_type::veteran_soldier,
                                e_unit_type::veteran_colonist )
                                .value(),
                            /*inventory=*/{} )
                            .value(),
        },
        UnitTransformationResult{
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
        UnitTransformationResult{
            .new_comp = UnitComposition::create(
                            /*type=*/UnitType::create(
                                e_unit_type::veteran_soldier,
                                e_unit_type::veteran_colonist )
                                .value(),
                            /*inventory=*/{} )
                            .value(),
        },
        UnitTransformationResult{
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
        UnitTransformationResult{
            .new_comp = UnitComposition::create(
                            /*type=*/UnitType::create(
                                e_unit_type::veteran_soldier,
                                e_unit_type::veteran_colonist )
                                .value(),
                            /*inventory=*/{} )
                            .value(),
        },
        UnitTransformationResult{
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
        UnitTransformationResult{
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
        UnitTransformationResult{
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
        UnitTransformationResult{
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
    vector<UnitTransformationFromCommodityResult> input;
    vector<UnitTransformationFromCommodityResult> expected;

    // Add independence after independence is declared.
    input = {
        UnitTransformationFromCommodityResult{
            .new_comp = UnitComposition::create(
                            /*type=*/UnitType::create(
                                e_unit_type::veteran_soldier,
                                e_unit_type::veteran_colonist )
                                .value(),
                            /*inventory=*/{} )
                            .value(),
        },
        UnitTransformationFromCommodityResult{
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
        UnitTransformationFromCommodityResult{
            .new_comp = UnitComposition::create(
                            /*type=*/UnitType::create(
                                e_unit_type::veteran_soldier,
                                e_unit_type::veteran_colonist )
                                .value(),
                            /*inventory=*/{} )
                            .value(),
        },
        UnitTransformationFromCommodityResult{
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
        UnitTransformationFromCommodityResult{
            .new_comp = UnitComposition::create(
                            /*type=*/UnitType::create(
                                e_unit_type::veteran_soldier,
                                e_unit_type::veteran_colonist )
                                .value(),
                            /*inventory=*/{} )
                            .value(),
        },
        UnitTransformationFromCommodityResult{
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
        UnitTransformationFromCommodityResult{
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
        UnitTransformationFromCommodityResult{
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
        UnitTransformationFromCommodityResult{
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

TEST_CASE( "[unit-composer] lua bindings" ) {
  lua::state st;
  st.lib.open_all();
  run_lua_startup_routines( st );

  auto script = R"(
    local uc
    local UC = unit_composer.UnitComposition
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
  )";
  REQUIRE( st.script.run_safe( script ) == valid );
}

TEST_CASE( "[unit-composer] strip_to_base_type " ) {
  UnitComposition          comp{};
  UnitTransformationResult res;
  UnitTransformationResult expected;

  comp = UnitComposition::create( e_unit_type::free_colonist );
  res  = strip_to_base_type( comp );
  expected = UnitTransformationResult{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::free_colonist ),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
  };
  REQUIRE( res == expected );

  comp = UnitComposition::create( e_unit_type::expert_farmer );
  res  = strip_to_base_type( comp );
  expected = UnitTransformationResult{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::expert_farmer ),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = {},
      .commodity_deltas = {},
  };
  REQUIRE( res == expected );

  comp = UnitComposition::create(
      UnitType::create( e_unit_type::dragoon,
                        e_unit_type::indentured_servant )
          .value() );
  res      = strip_to_base_type( comp );
  expected = UnitTransformationResult{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::indentured_servant ),
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

  comp = UnitComposition::create( e_unit_type::veteran_dragoon );
  res  = strip_to_base_type( comp );
  expected = UnitTransformationResult{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::veteran_colonist ),
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
  expected = UnitTransformationResult{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::free_colonist ),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::tools,
                             e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::tools, 80 } },
  };
  REQUIRE( res == expected );

  comp = UnitComposition::create(
             UnitType::create( e_unit_type::hardy_pioneer ),
             /*inventory=*/{ { e_unit_inventory::tools, 100 } } )
             .value();
  res      = strip_to_base_type( comp );
  expected = UnitTransformationResult{
      .new_comp = UnitComposition::create(
                      /*type=*/UnitType::create(
                          e_unit_type::hardy_colonist ),
                      /*inventory=*/{} )
                      .value(),
      .modifier_deltas  = { { e_unit_type_modifier::tools,
                             e_unit_type_modifier_delta::del } },
      .commodity_deltas = { { e_commodity::tools, 100 } },
  };
  REQUIRE( res == expected );
}

TEST_CASE( "[unit-composer] unit_lose_commodity" ) {
  UnitComposition                               comp{};
  Commodity                                     comm;
  vector<UnitTransformationFromCommodityResult> res;
  vector<UnitTransformationFromCommodityResult> expected;

  // hardy_pioneer with 20 tools - 20 tools.
  comp = UnitComposition::create(
             UnitType::create( e_unit_type::hardy_pioneer ),
             /*inventory=*/{ { e_unit_inventory::tools, 20 } } )
             .value();
  comm = { .type = e_commodity::tools, .quantity = 20 };
  res  = unit_lose_commodity( comp, comm );

  expected = {
      UnitTransformationFromCommodityResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::missionary,
                              e_unit_type::hardy_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::add } },
          .quantity_used = -20,
      },
      UnitTransformationFromCommodityResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::hardy_colonist,
                              e_unit_type::hardy_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::del } },
          .quantity_used = -20,
      },
  };
  sort_by_new_type( res );
  sort_by_new_type( expected );
  REQUIRE( FmtVerticalJsonList{ res } ==
           FmtVerticalJsonList{ expected } );

  // hardy_pioneer with 80 tools - 20 tools.
  comp = UnitComposition::create(
             UnitType::create( e_unit_type::hardy_pioneer ),
             /*inventory=*/{ { e_unit_inventory::tools, 80 } } )
             .value();
  comm     = { .type = e_commodity::tools, .quantity = 20 };
  res      = unit_lose_commodity( comp, comm );
  expected = {
      UnitTransformationFromCommodityResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::missionary,
                              e_unit_type::hardy_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::del },
                { e_unit_type_modifier::blessing,
                  e_unit_type_modifier_delta::add } },
          .quantity_used = -80,
      },
      UnitTransformationFromCommodityResult{
          .new_comp = UnitComposition::create(
                          /*type=*/UnitType::create(
                              e_unit_type::hardy_colonist,
                              e_unit_type::hardy_colonist )
                              .value(),
                          /*inventory=*/{} )
                          .value(),
          .modifier_deltas =
              { { e_unit_type_modifier::tools,
                  e_unit_type_modifier_delta::del } },
          .quantity_used = -80,
      },
      UnitTransformationFromCommodityResult{
          .new_comp =
              UnitComposition::create(
                  /*type=*/UnitType::create(
                      e_unit_type::hardy_pioneer,
                      e_unit_type::hardy_colonist )
                      .value(),
                  /*inventory=*/{ { e_unit_inventory::tools,
                                    60 } } )
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

TEST_CASE( "[unit-composer] promoted_from_activity" ) {
  SECTION( "petty_criminal / farming" ) {
    e_unit_activity const activity = e_unit_activity::farming;
    auto ut = UnitType::create( e_unit_type::petty_criminal );
    UNWRAP_CHECK(
        uc, UnitComposition::create( ut, /*inventory=*/{} ) );
    UnitComposition expected( wrapped::UnitComposition{
        .type =
            UnitType::create( e_unit_type::indentured_servant ),
        .inventory = uc.inventory(),
    } );
    REQUIRE( promoted_from_activity( uc, activity ) ==
             expected );
  }

  SECTION( "indentured_servant / farming" ) {
    e_unit_activity const activity = e_unit_activity::farming;
    auto                  ut =
        UnitType::create( e_unit_type::indentured_servant );
    UNWRAP_CHECK(
        uc, UnitComposition::create( ut, /*inventory=*/{} ) );
    UnitComposition expected( wrapped::UnitComposition{
        .type = UnitType::create( e_unit_type::free_colonist ),
        .inventory = uc.inventory(),
    } );
    REQUIRE( promoted_from_activity( uc, activity ) ==
             expected );
  }

  SECTION( "free_colonist / farming" ) {
    e_unit_activity const activity = e_unit_activity::farming;
    auto ut = UnitType::create( e_unit_type::free_colonist );
    UNWRAP_CHECK(
        uc, UnitComposition::create( ut, /*inventory=*/{} ) );
    UnitComposition expected( wrapped::UnitComposition{
        .type = UnitType::create( e_unit_type::expert_farmer ),
        .inventory = uc.inventory(),
    } );
    REQUIRE( promoted_from_activity( uc, activity ) ==
             expected );
  }

  SECTION( "free_colonist / fighting" ) {
    e_unit_activity const activity = e_unit_activity::fighting;
    auto ut = UnitType::create( e_unit_type::free_colonist );
    UNWRAP_CHECK(
        uc, UnitComposition::create( ut, /*inventory=*/{} ) );
    UnitComposition expected( wrapped::UnitComposition{
        .type =
            UnitType::create( e_unit_type::veteran_colonist ),
        .inventory = uc.inventory(),
    } );
    REQUIRE( promoted_from_activity( uc, activity ) ==
             expected );
  }

  SECTION( "free_colonist / pioneering" ) {
    e_unit_activity const activity = e_unit_activity::pioneering;
    auto ut = UnitType::create( e_unit_type::free_colonist );
    UNWRAP_CHECK(
        uc, UnitComposition::create( ut, /*inventory=*/{} ) );
    UnitComposition expected( wrapped::UnitComposition{
        .type = UnitType::create( e_unit_type::hardy_colonist ),
        .inventory = uc.inventory(),
    } );
    REQUIRE( promoted_from_activity( uc, activity ) ==
             expected );
  }

  SECTION( "scout / fighting" ) {
    e_unit_activity const activity = e_unit_activity::fighting;
    auto ut = UnitType::create( e_unit_type::scout );
    UNWRAP_CHECK(
        uc, UnitComposition::create( ut, /*inventory=*/{} ) );
    UnitComposition expected( wrapped::UnitComposition{
        .type = UnitType::create( e_unit_type::seasoned_scout ),
        .inventory = uc.inventory(),
    } );
    REQUIRE( promoted_from_activity( uc, activity ) ==
             expected );
  }

  SECTION( "pioneer / pioneering" ) {
    e_unit_activity const activity = e_unit_activity::pioneering;
    auto ut = UnitType::create( e_unit_type::pioneer );
    UNWRAP_CHECK( uc,
                  UnitComposition::create(
                      ut, /*inventory=*/{
                          { e_unit_inventory::tools, 80 } } ) );
    UnitComposition expected( wrapped::UnitComposition{
        .type = UnitType::create( e_unit_type::hardy_pioneer ),
        .inventory = uc.inventory(),
    } );
    REQUIRE( promoted_from_activity( uc, activity ) ==
             expected );
  }

  SECTION( "hardy_colonist / pioneering" ) {
    e_unit_activity const activity = e_unit_activity::pioneering;
    auto ut = UnitType::create( e_unit_type::hardy_colonist );
    UNWRAP_CHECK(
        uc, UnitComposition::create( ut, /*inventory=*/{} ) );
    REQUIRE( promoted_from_activity( uc, activity ) ==
             unexpected<UnitComposition>(
                 "viable unit type not found"s ) );
  }

  SECTION( "hardy_pioneer / pioneering" ) {
    e_unit_activity const activity = e_unit_activity::pioneering;
    auto ut = UnitType::create( e_unit_type::hardy_pioneer );
    UNWRAP_CHECK( uc,
                  UnitComposition::create(
                      ut, /*inventory=*/{
                          { e_unit_inventory::tools, 80 } } ) );
    REQUIRE( promoted_from_activity( uc, activity ) ==
             unexpected<UnitComposition>(
                 "viable unit type not found"s ) );
  }

  SECTION( "hardy_colonist / farming" ) {
    e_unit_activity const activity = e_unit_activity::farming;
    auto ut = UnitType::create( e_unit_type::hardy_colonist );
    UNWRAP_CHECK(
        uc, UnitComposition::create( ut, /*inventory=*/{} ) );
    REQUIRE( promoted_from_activity( uc, activity ) ==
             unexpected<UnitComposition>(
                 "viable unit type not found"s ) );
  }

  SECTION( "hardy_pioneer / farming" ) {
    e_unit_activity const activity = e_unit_activity::farming;
    auto ut = UnitType::create( e_unit_type::hardy_pioneer );
    UNWRAP_CHECK( uc,
                  UnitComposition::create(
                      ut, /*inventory=*/{
                          { e_unit_inventory::tools, 80 } } ) );
    REQUIRE( promoted_from_activity( uc, activity ) ==
             unexpected<UnitComposition>(
                 "viable unit type not found"s ) );
  }
}

} // namespace
} // namespace rn
