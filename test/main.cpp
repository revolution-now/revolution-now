/****************************************************************
**main.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-01.
*
* Description: Provides main() for the unit tests.
*
*****************************************************************/
// Testing
#include "test/fake/world.hpp"

// Revolution Now
#include "src/engine.hpp"
#include "src/linking.hpp"

#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"

using namespace rn;

int main( int argc, char** argv ) {
  linker_dont_discard_me();
  Engine engine;
  engine.init( e_engine_mode::unit_tests );
  int result = Catch::Session().run( argc, argv );
  return result;
}
