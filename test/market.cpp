/****************************************************************
**market.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-21.
*
* Description: Unit tests for the src/market.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/market.hpp"

// ss
#include "ss/player.hpp"

// Testing
#include "test/fake/world.hpp"

// refl
#include "refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    create_default_map();
    add_player( e_nation::french );
    add_player( e_nation::dutch );
  }

  void create_default_map() {
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{ L };
    build_map( std::move( tiles ), 1 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[market] market_price" ) {
  World   W;
  Player& french = W.player( e_nation::french );

  french.old_world.market.commodities[e_commodity::ore]
      .current_bid_price_in_hundreds = 5;
  REQUIRE( market_price( french, e_commodity::ore ) ==
           CommodityPrice{ .bid = 5, .ask = 8 } );

  french.old_world.market.commodities[e_commodity::food]
      .current_bid_price_in_hundreds = 1;
  REQUIRE( market_price( french, e_commodity::food ) ==
           CommodityPrice{ .bid = 1, .ask = 9 } );

  french.old_world.market.commodities[e_commodity::muskets]
      .current_bid_price_in_hundreds = 3;
  REQUIRE( market_price( french, e_commodity::muskets ) ==
           CommodityPrice{ .bid = 3, .ask = 4 } );
}

} // namespace
} // namespace rn
