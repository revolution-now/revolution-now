/****************************************************************
**ref.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-12.
*
* Description: Unit tests for the src/ss/ref.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/ss/ref.hpp"

// ss
#include "src/ss/root.hpp"

// gfx
#include "gfx/cdr-matrix.hpp"

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
            "difficulty"_key = "discoverer",
            "game_options"_key =
                cdr::table{
                    "flags"_key =
                        cdr::table{
                            "show_indian_moves"_key   = false,
                            "show_foreign_moves"_key  = false,
                            "fast_piece_slide"_key    = false,
                            "end_of_turn"_key         = false,
                            "autosave"_key            = false,
                            "combat_analysis"_key     = false,
                            "water_color_cycling"_key = false,
                            "tutorial_hints"_key      = false,
                            "show_fog_of_war"_key     = false,
                        },
                },
        },
    "events"_key = cdr::table{},
    "units"_key =
        cdr::table{
            "next_unit_id"_key = 1,
            "units"_key        = cdr::list{},
        },
    "players"_key =
        cdr::table{
            "players"_key =
                cdr::table{
                    "dutch"_key   = cdr::null,
                    "english"_key = cdr::null,
                    "french"_key  = cdr::null,
                    "spanish"_key = cdr::null,
                },
            "humans"_key =
                cdr::table{
                    "dutch"_key   = false,
                    "english"_key = false,
                    "french"_key  = false,
                    "spanish"_key = false,
                },
            "global_market_state"_key =
                cdr::table{
                    "commodities"_key =
                        cdr::table{
                            "food"_key =
                                cdr::table{
                                    "intrinsic_volume"_key = 0 },
                            "sugar"_key =
                                cdr::table{
                                    "intrinsic_volume"_key = 0 },
                            "tobacco"_key =
                                cdr::table{
                                    "intrinsic_volume"_key = 0 },
                            "cotton"_key =
                                cdr::table{
                                    "intrinsic_volume"_key = 0 },
                            "fur"_key = cdr::
                                table{ "intrinsic_volume"_key =
                                           0 },
                            "lumber"_key =
                                cdr::table{
                                    "intrinsic_volume"_key = 0 },
                            "ore"_key = cdr::
                                table{ "intrinsic_volume"_key =
                                           0 },
                            "silver"_key =
                                cdr::table{
                                    "intrinsic_volume"_key = 0 },
                            "horses"_key =
                                cdr::table{
                                    "intrinsic_volume"_key = 0 },
                            "rum"_key = cdr::
                                table{ "intrinsic_volume"_key =
                                           0 },
                            "cigars"_key =
                                cdr::table{
                                    "intrinsic_volume"_key = 0 },
                            "cloth"_key =
                                cdr::table{
                                    "intrinsic_volume"_key = 0 },
                            "coats"_key =
                                cdr::table{
                                    "intrinsic_volume"_key = 0 },
                            "trade_goods"_key =
                                cdr::table{
                                    "intrinsic_volume"_key = 0 },
                            "tools"_key =
                                cdr::table{
                                    "intrinsic_volume"_key = 0 },
                            "muskets"_key = cdr::
                                table{ "intrinsic_volume"_key =
                                           0 },
                        },
                },
        },
    "turn"_key =
        cdr::table{
            "time_point"_key =
                cdr::table{
                    "year"_key   = 0,
                    "season"_key = "winter",
                    "turns"_key  = 0,
                },
            "cycle"_key =
                cdr::table{
                    "not_started"_key = cdr::table{},
                },
        },
    "colonies"_key =
        cdr::table{
            "next_colony_id"_key = 1,
            "colonies"_key       = cdr::list{},
        },
    "natives"_key =
        cdr::table{
            "next_dwelling_id"_key = 1,
            "tribes"_key =
                cdr::table{
                    "apache"_key   = cdr::null,
                    "sioux"_key    = cdr::null,
                    "tupi"_key     = cdr::null,
                    "arawak"_key   = cdr::null,
                    "cherokee"_key = cdr::null,
                    "iroquois"_key = cdr::null,
                    "aztec"_key    = cdr::null,
                    "inca"_key     = cdr::null,
                },
            "dwellings"_key                 = cdr::list{},
            "owned_land_without_minuit"_key = cdr::list{},
        },
    "land_view"_key =
        cdr::table{
            "viewport"_key =
                cdr::table{
                    "zoom"_key     = 1.0,
                    "center_x"_key = 0.0,
                    "center_y"_key = 0.0,
                    "world_size_tiles"_key =
                        cdr::table{
                            "h"_key = 0,
                            "w"_key = 0,
                        },
                },
            "minimap"_key =
                cdr::table{
                    "origin"_key =
                        cdr::table{
                            "x"_key = 0.0,
                            "y"_key = 0.0,
                        },
                },
            "map_revealed"_key =
                cdr::table{
                    "no_special_view"_key = cdr::table{},
                },
        },
    "zzz_terrain"_key =
        cdr::table{
            "placement_seed"_key = 0,
            "player_terrain"_key =
                cdr::table{
                    "dutch"_key   = cdr::null,
                    "english"_key = cdr::null,
                    "french"_key  = cdr::null,
                    "spanish"_key = cdr::null,
                },
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
            "proto_squares"_key =
                cdr::table{
                    "n"_key =
                        cdr::table{
                            "surface"_key         = "water",
                            "ground"_key          = "arctic",
                            "overlay"_key         = cdr::null,
                            "river"_key           = cdr::null,
                            "ground_resource"_key = cdr::null,
                            "forest_resource"_key = cdr::null,
                            "irrigation"_key      = false,
                            "road"_key            = false,
                            "sea_lane"_key        = false,
                            "lost_city_rumor"_key = false,
                        },
                    "w"_key =
                        cdr::table{
                            "surface"_key         = "water",
                            "ground"_key          = "arctic",
                            "overlay"_key         = cdr::null,
                            "river"_key           = cdr::null,
                            "ground_resource"_key = cdr::null,
                            "forest_resource"_key = cdr::null,
                            "irrigation"_key      = false,
                            "road"_key            = false,
                            "sea_lane"_key        = false,
                            "lost_city_rumor"_key = false,
                        },
                    "s"_key =
                        cdr::table{
                            "surface"_key         = "water",
                            "ground"_key          = "arctic",
                            "overlay"_key         = cdr::null,
                            "river"_key           = cdr::null,
                            "ground_resource"_key = cdr::null,
                            "forest_resource"_key = cdr::null,
                            "irrigation"_key      = false,
                            "road"_key            = false,
                            "sea_lane"_key        = false,
                            "lost_city_rumor"_key = false,
                        },
                    "e"_key =
                        cdr::table{
                            "surface"_key         = "water",
                            "ground"_key          = "arctic",
                            "overlay"_key         = cdr::null,
                            "river"_key           = cdr::null,
                            "ground_resource"_key = cdr::null,
                            "forest_resource"_key = cdr::null,
                            "irrigation"_key      = false,
                            "road"_key            = false,
                            "sea_lane"_key        = false,
                            "lost_city_rumor"_key = false,
                        },
                },
            "pacific_ocean_endpoints"_key = cdr::list{},
        },
};

// static_assert( equality_comparable<FormatVersion> );
// static_assert( equality_comparable<SettingsState> );
// static_assert( equality_comparable<EventsState> );
// static_assert( equality_comparable<UnitsState> );
// static_assert( equality_comparable<PlayersState> );
// static_assert( equality_comparable<TurnState> );
// static_assert( equality_comparable<ColoniesState> );
// static_assert( equality_comparable<NativesState> );
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
