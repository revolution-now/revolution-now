/****************************************************************
**game-state.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-12.
*
* Description: Unit tests for the src/game-state.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/game-state.hpp"

// Revolution Now
#include "src/gs-root.hpp"

// refl
#include "refl/cdr.hpp"

// cdr
#include "cdr/converter.hpp"
#include "cdr/ext-base.hpp"
#include "cdr/ext-builtin.hpp"
#include "cdr/ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace ::std;
using namespace ::cdr::literals;

using ::cdr::testing::conv_from_bt;

cdr::value cdr_game_state_default = cdr::table{
    "version"_key =
        cdr::table{
            "major"_key = 0,
            "minor"_key = 0,
            "patch"_key = 0,
        },
    "settings"_key =
        cdr::table{
            "difficulty"_key = 0,
        },
    "events"_key = cdr::table{},
    "units"_key =
        cdr::table{
            "next_unit_id"_key = 1,
            "units"_key        = cdr::list{},
        },
    "players"_key =
        cdr::table{
            "players"_key = cdr::table{},
        },
    "turn"_key =
        cdr::table{
            "time_point"_key =
                cdr::table{
                    "year"_key   = 0,
                    "season"_key = "winter",
                },
            "started"_key   = false,
            "need_eot"_key  = false,
            "nation"_key    = cdr::null,
            "remainder"_key = cdr::list{},
        },
    "colonies"_key =
        cdr::table{
            "next_colony_id"_key = 1,
            "colonies"_key       = cdr::list{},
        },
    "land_view"_key =
        cdr::table{
            "viewport"_key =
                cdr::table{
                    "zoom"_key     = 1.0,
                    "center_x"_key = 0.0,
                    "center_y"_key = 0.0,
                },
        },
    "zzz_terrain"_key =
        cdr::table{
            "placement_seed"_key = 0,
            "world_map"_key =
                cdr::table{
                    "size"_key =
                        cdr::table{
                            "h"_key = 0,
                            "w"_key = 0,
                        },
                    "data"_key       = cdr::list{},
                    "has_coords"_key = false,
                },
        },
};

// static_assert( equality_comparable<FormatVersion> );
// static_assert( equality_comparable<SettingsState> );
// static_assert( equality_comparable<EventsState> );
// static_assert( equality_comparable<UnitsState> );
// static_assert( equality_comparable<PlayersState> );
// static_assert( equality_comparable<TurnState> );
// static_assert( equality_comparable<ColoniesState> );
// static_assert( equality_comparable<LandViewState> );
// static_assert( equality_comparable<TerrainState> );
// static_assert( equality_comparable<RootState> );

TEST_CASE( "[game-state] some test" ) {
  cdr::converter conv;
  RootState      root_def;
  cdr::value     v = conv.to( root_def );
  REQUIRE( v == cdr_game_state_default );
  // Round trip.
  REQUIRE( conv_from_bt<RootState>( conv, v ) == root_def );
  // From the original cdr.
  REQUIRE( conv_from_bt<RootState>(
               conv, cdr_game_state_default ) == root_def );
}

} // namespace
} // namespace rn
