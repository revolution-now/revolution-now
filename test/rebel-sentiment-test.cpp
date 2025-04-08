/****************************************************************
**rebel-sentiment-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-04-06.
*
* Description: Unit tests for the rebel-sentiment module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rebel-sentiment.hpp"

// Testing.
#include "test/fake/world.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      L, L, L, //
      L, _, L, //
      L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[rebel-sentiment] updated_rebel_sentiment" ) {
  world w;
}

TEST_CASE( "[rebel-sentiment] update_rebel_sentiment" ) {
  world w;
}

TEST_CASE(
    "[rebel-sentiment] should_show_rebel_sentiment_report" ) {
  world w;
}

TEST_CASE(
    "[rebel-sentiment] show_rebel_sentiment_change_report" ) {
  world w;
}

TEST_CASE(
    "[rebel-sentiment] rebel_sentiment_report_for_cc_report" ) {
  world w;
}

} // namespace
} // namespace rn
