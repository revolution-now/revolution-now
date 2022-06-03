/****************************************************************
**harbor-units.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-02.
*
* Description: Unit tests for the src/harbor-units.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/harbor-units.hpp"

// Revolution Now
#include "src/gs-root.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

void build_world( RootState& root ) {
  //
}

TEST_CASE( "[harbor-units] is_unit_inbound" ) {
  RootState root;
  build_world( root );
  // TODO
}

TEST_CASE( "[harbor-units] is_unit_outbound" ) {
  RootState root;
  build_world( root );
  // TODO
}

TEST_CASE( "[harbor-units] is_unit_in_port" ) {
  RootState root;
  build_world( root );
  // TODO
}

TEST_CASE( "[harbor-units] harbor_units_on_dock" ) {
  RootState root;
  build_world( root );
  // TODO
}

TEST_CASE( "[harbor-units] harbor_units_in_port" ) {
  RootState root;
  build_world( root );
  // TODO
}

TEST_CASE( "[harbor-units] harbor_units_inbound" ) {
  RootState root;
  build_world( root );
  // TODO
}

TEST_CASE( "[harbor-units] harbor_units_outbound" ) {
  RootState root;
  build_world( root );
  // TODO
}

TEST_CASE( "[harbor-units] unit_sail_to_new_world" ) {
  RootState root;
  build_world( root );
  // TODO
}

TEST_CASE( "[harbor-units] unit_sail_to_harbor" ) {
  RootState root;
  build_world( root );
  // TODO
}

TEST_CASE( "[harbor-units] unit_move_to_port" ) {
  RootState root;
  build_world( root );
  // TODO
}

TEST_CASE( "[harbor-units] advance_unit_on_high_seas" ) {
  RootState root;
  build_world( root );
  // TODO
}

TEST_CASE( "[harbor-units] create_unit_in_harbor" ) {
  RootState root;
  build_world( root );
  // TODO
}

} // namespace
} // namespace rn
