/****************************************************************
**user-config-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-17.
*
* Description: Unit tests for the user-config module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/user-config.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::base::valid;

/****************************************************************
** Helpers.
*****************************************************************/
fs::path output_folder() {
  error_code ec = {};
  fs::path res  = fs::temp_directory_path( ec );
  BASE_CHECK( ec.value() == 0,
              "failed to get temp folder path." );
  return res;
}

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[user-config] default construction/read/write" ) {
  UserConfig conf;

  REQUIRE( conf.read().validate() );

  REQUIRE( conf.read().game_saving.ask_before_overwrite );
  REQUIRE( conf.read().game_saving.ask_need_save_when_leaving );
  REQUIRE( conf.read().graphics.render_framebuffer_mode ==
           rr::e_render_framebuffer_mode::
               offscreen_with_logical_resolution );

  bool const ok = conf.modify( []( config_user_t& c ) {
    c.graphics.render_framebuffer_mode =
        rr::e_render_framebuffer_mode::direct_to_screen;
    c.game_saving.ask_before_overwrite = false;
  } );
  REQUIRE( ok );

  REQUIRE_FALSE( conf.read().game_saving.ask_before_overwrite );
  REQUIRE( conf.read().game_saving.ask_need_save_when_leaving );
  REQUIRE( conf.read().graphics.render_framebuffer_mode ==
           rr::e_render_framebuffer_mode::direct_to_screen );

  REQUIRE( conf.flush() == valid );
}

TEST_CASE( "[user-config] binding" ) {
  auto const tmpfile = output_folder() / "user-config-test.rcl";

  if( fs::exists( tmpfile ) ) fs::remove( tmpfile );
  BASE_CHECK( !fs::exists( tmpfile ) );

  // Phase 1: bind, write, but no flush.
  {
    UserConfig conf;

    REQUIRE( conf.try_bind_to_file( tmpfile ) == valid );

    REQUIRE( conf.read().validate() );

    REQUIRE( conf.read().game_saving.ask_before_overwrite );
    REQUIRE(
        conf.read().game_saving.ask_need_save_when_leaving );
    REQUIRE( conf.read().graphics.render_framebuffer_mode ==
             rr::e_render_framebuffer_mode::
                 offscreen_with_logical_resolution );

    bool const ok = conf.modify( []( config_user_t& c ) {
      c.graphics.render_framebuffer_mode =
          rr::e_render_framebuffer_mode::direct_to_screen;
      c.game_saving.ask_before_overwrite = false;
    } );
    REQUIRE( ok );

    REQUIRE_FALSE(
        conf.read().game_saving.ask_before_overwrite );
    REQUIRE(
        conf.read().game_saving.ask_need_save_when_leaving );
    REQUIRE( conf.read().graphics.render_framebuffer_mode ==
             rr::e_render_framebuffer_mode::direct_to_screen );
  }

  // Phase 2: bind again, with nothing to read, write again but
  // flush this time.
  {
    UserConfig conf;

    REQUIRE( conf.try_bind_to_file( tmpfile ) == valid );

    REQUIRE( conf.read().validate() );

    REQUIRE( conf.read().game_saving.ask_before_overwrite );
    REQUIRE(
        conf.read().game_saving.ask_need_save_when_leaving );
    REQUIRE( conf.read().graphics.render_framebuffer_mode ==
             rr::e_render_framebuffer_mode::
                 offscreen_with_logical_resolution );

    bool const ok = conf.modify( []( config_user_t& c ) {
      c.graphics.render_framebuffer_mode =
          rr::e_render_framebuffer_mode::direct_to_screen;
      c.game_saving.ask_before_overwrite = false;
    } );
    REQUIRE( ok );

    REQUIRE_FALSE(
        conf.read().game_saving.ask_before_overwrite );
    REQUIRE(
        conf.read().game_saving.ask_need_save_when_leaving );
    REQUIRE( conf.read().graphics.render_framebuffer_mode ==
             rr::e_render_framebuffer_mode::direct_to_screen );

    REQUIRE( conf.flush() == valid );
  }

  // Phase 3: recover from file.
  {
    UserConfig conf;

    REQUIRE( conf.try_bind_to_file( tmpfile ) == valid );
    REQUIRE( conf.read().validate() );

    REQUIRE_FALSE(
        conf.read().game_saving.ask_before_overwrite );
    REQUIRE(
        conf.read().game_saving.ask_need_save_when_leaving );
    REQUIRE( conf.read().graphics.render_framebuffer_mode ==
             rr::e_render_framebuffer_mode::direct_to_screen );

    REQUIRE( conf.flush() == valid );
  }
}

} // namespace
} // namespace rn
