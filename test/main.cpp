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

// config
#include "src/config/rn.rds.hpp"

#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"

namespace testing {
[[nodiscard]] bool expensive_tests_enabled();
bool expensive_tests_enabled() {
  return ::rn::config_rn.development.unit_tests
      .run_expensive_tests;
}
}

using namespace rn;

int main( int argc, char** argv ) {
  linker_dont_discard_me();
  Engine engine;
  engine.init( e_engine_mode::unit_tests );
  int result = Catch::Session().run( argc, argv );
  return result;
}
