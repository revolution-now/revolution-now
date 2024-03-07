/****************************************************************
**fog-square-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-02-27.
*
* Description: Unit tests for the ss/fog-square module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/ss/fog-square.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using unexplored = PlayerSquare::unexplored;
using explored   = PlayerSquare::explored;
using fogged     = FogStatus::fogged;
using clear      = FogStatus::clear;

/****************************************************************
** Static Checks.
*****************************************************************/
// The fully-unexplored alternative should be first (index 0) for
// default construction purposes.
static_assert( std::is_same_v<std::variant_alternative_t<
                                  0, PlayerSquare::Base::base_t>,
                              unexplored> );

static_assert(
    std::is_same_v<
        std::variant_alternative_t<0, FogStatus::Base::base_t>,
        FogStatus::fogged> );

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[ss/fog-square] default construction" ) {
  PlayerSquare ps;
  REQUIRE( ps == unexplored{} );

  ps.emplace<explored>();
  REQUIRE( ps == explored{ .fog_status = fogged{} } );
}

} // namespace
} // namespace rn
