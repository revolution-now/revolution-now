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
#include "src/init.hpp"
#include "src/linking.hpp"

#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"

using namespace rn;

int main( int argc, char** argv ) {
  linker_dont_discard_me();
  run_all_init_routines( e_log_level::off,
                         { e_init_routine::configs } );
  int result = Catch::Session().run( argc, argv );
  run_all_cleanup_routines();
  return result;
}
