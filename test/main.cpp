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
// Revolution Now
#include "init.hpp"
#include "linking.hpp"

#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"

using namespace rn;

int main( int argc, char** argv ) {
  linker_dont_discard_me();
  run_all_init_routines(
      e_log_level::off,
      { e_init_routine::configs, e_init_routine::rng } );
  int result = Catch::Session().run( argc, argv );
  run_all_cleanup_routines();
  return result;
}
