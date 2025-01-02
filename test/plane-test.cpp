/****************************************************************
**plane-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-01.
*
* Description: Unit tests for the plane module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/plane.hpp"

// Testing.
#include "test/mocking.hpp"
#include "test/mocks/iplane.hpp"

// gfx
#include "src/gfx/resolution-enum.rds.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::e_resolution;
using ::mock::matchers::_;
using ::mock::matchers::Not;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[plane] on_logical_resolution_changed" ) {
  using enum e_resolution;

  MockIPlane p;
  e_resolution resolution = {};

  auto f = [&] {
    p.on_logical_resolution_changed( resolution );
  };

  SECTION( "supports none" ) {
    resolution = _640x400;
    p.EXPECT__supports_resolution( _ ).by_default().returns(
        false );
    p.EXPECT__on_logical_resolution_selected( _640x400 );
    f();
  }

  SECTION( "supports all" ) {
    resolution = _640x400;
    p.EXPECT__supports_resolution( _ ).by_default().returns(
        true );
    p.EXPECT__on_logical_resolution_selected( _640x400 );
    f();
  }

  SECTION( "supports actual only" ) {
    resolution = _640x400;
    p.EXPECT__supports_resolution( _640x400 ).returns( true );
    p.EXPECT__on_logical_resolution_selected( _640x400 );
    f();
  }

  SECTION( "supports all but actual" ) {
    resolution = _640x400;
    p.EXPECT__supports_resolution( _640x400 ).returns( false );

    p.EXPECT__supports_resolution( _640x360 ).returns( true );
    p.EXPECT__supports_resolution( _768x432 ).returns( true );
    p.EXPECT__supports_resolution( _480x270 ).returns( true );
    p.EXPECT__supports_resolution( _576x360 ).returns( true );
    p.EXPECT__supports_resolution( _640x400 ).returns( false );
    p.EXPECT__supports_resolution( _720x450 ).returns( true );
    p.EXPECT__supports_resolution( _640x480 ).returns( true );
    p.EXPECT__supports_resolution( _960x720 ).returns( true );
    p.EXPECT__supports_resolution( _852x360 ).returns( true );
    p.EXPECT__supports_resolution( _1280x540 ).returns( true );
    p.EXPECT__supports_resolution( _1146x480 ).returns( true );
    p.EXPECT__supports_resolution( _860x360 ).returns( true );
    p.EXPECT__supports_resolution( _960x400 ).returns( true );

    p.EXPECT__on_logical_resolution_selected( _640x360 );
    f();
  }
}

} // namespace
} // namespace rn
