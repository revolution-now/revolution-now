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

using ::cdr::list;
using ::cdr::null;
using ::cdr::table;
using ::cdr::value;
using ::cdr::testing::conv_from_bt;

value cdr_game_state_default = table{
  "version"_key =
      table{
        "major"_key = 0,
        "minor"_key = 0,
        "patch"_key = 0,
      },
  "settings"_key =
      table{
        "difficulty"_key = "discoverer",
        "game_options"_key =
            table{
              "flags"_key =
                  table{
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
        "cheat_options"_key =
            table{
              "enabled"_key = false,
            },
        "colony_options"_key =
            table{
              "numbers"_key = false,
            },
      },
  "events"_key = table{},
  "units"_key =
      table{
        "next_unit_id"_key             = 1,
        "units"_key                    = list{},
        "unit_ordering"_key            = list{},
        "curr_unit_ordering_index"_key = 0,
      },
  "players"_key =
      table{
        "players"_key =
            table{
              "dutch"_key   = null,
              "english"_key = null,
              "french"_key  = null,
              "spanish"_key = null,
            },
        "humans"_key =
            table{
              "dutch"_key   = false,
              "english"_key = false,
              "french"_key  = false,
              "spanish"_key = false,
            },
        "global_market_state"_key =
            table{
              "commodities"_key =
                  table{
                    "food"_key =
                        table{ "intrinsic_volume"_key = 0 },
                    "sugar"_key =
                        table{ "intrinsic_volume"_key = 0 },
                    "tobacco"_key =
                        table{ "intrinsic_volume"_key = 0 },
                    "cotton"_key =
                        table{ "intrinsic_volume"_key = 0 },
                    "furs"_key =
                        table{ "intrinsic_volume"_key = 0 },
                    "lumber"_key =
                        table{ "intrinsic_volume"_key = 0 },
                    "ore"_key =
                        table{ "intrinsic_volume"_key = 0 },
                    "silver"_key =
                        table{ "intrinsic_volume"_key = 0 },
                    "horses"_key =
                        table{ "intrinsic_volume"_key = 0 },
                    "rum"_key =
                        table{ "intrinsic_volume"_key = 0 },
                    "cigars"_key =
                        table{ "intrinsic_volume"_key = 0 },
                    "cloth"_key =
                        table{ "intrinsic_volume"_key = 0 },
                    "coats"_key =
                        table{ "intrinsic_volume"_key = 0 },
                    "trade_goods"_key =
                        table{ "intrinsic_volume"_key = 0 },
                    "tools"_key =
                        table{ "intrinsic_volume"_key = 0 },
                    "muskets"_key =
                        table{ "intrinsic_volume"_key = 0 },
                  },
            },
      },
  "turn"_key =
      table{
        "time_point"_key =
            table{
              "year"_key   = 0,
              "season"_key = "winter",
              "turns"_key  = 0,
            },
        "autosave"_key =
            table{
              "last_save"_key = null,
            },
        "cycle"_key =
            table{
              "not_started"_key = table{},
            },
      },
  "colonies"_key =
      table{
        "next_colony_id"_key = 1,
        "colonies"_key       = list{},
      },
  "natives"_key =
      table{
        "next_dwelling_id"_key = 1,
        "tribes"_key =
            table{
              "apache"_key   = null,
              "sioux"_key    = null,
              "tupi"_key     = null,
              "arawak"_key   = null,
              "cherokee"_key = null,
              "iroquois"_key = null,
              "aztec"_key    = null,
              "inca"_key     = null,
            },
        "dwellings"_key                 = list{},
        "owned_land_without_minuit"_key = list{},
      },
  "land_view"_key =
      table{
        "viewport"_key =
            table{
              "zoom"_key     = 0.0,
              "center_x"_key = 0.0,
              "center_y"_key = 0.0,
            },
        "minimap"_key =
            table{
              "origin"_key =
                  table{
                    "x"_key = 0.0,
                    "y"_key = 0.0,
                  },
            },
        "map_revealed"_key =
            table{
              "no_special_view"_key = table{},
            },
        "white_box"_key =
            table{
              "x"_key = 0,
              "y"_key = 0,
            },
      },
  "map"_key =
      table{
        "depletion"_key =
            table{
              "counters"_key = list{},
            },
      },
  "zzz_terrain"_key =
      table{
        "placement_seed"_key = 0,
        "player_terrain"_key =
            table{
              "dutch"_key   = null,
              "english"_key = null,
              "french"_key  = null,
              "spanish"_key = null,
            },
        "real_terrain"_key =
            table{
              "map"_key =
                  table{
                    "size"_key =
                        table{
                          "h"_key = 0,
                          "w"_key = 0,
                        },
                    "data"_key       = list{},
                    "has_coords"_key = false,
                  },
            },
        "proto_squares"_key =
            table{
              "n"_key =
                  table{
                    "surface"_key         = "water",
                    "ground"_key          = "arctic",
                    "overlay"_key         = null,
                    "river"_key           = null,
                    "ground_resource"_key = null,
                    "forest_resource"_key = null,
                    "irrigation"_key      = false,
                    "road"_key            = false,
                    "sea_lane"_key        = false,
                    "lost_city_rumor"_key = false,
                  },
              "w"_key =
                  table{
                    "surface"_key         = "water",
                    "ground"_key          = "arctic",
                    "overlay"_key         = null,
                    "river"_key           = null,
                    "ground_resource"_key = null,
                    "forest_resource"_key = null,
                    "irrigation"_key      = false,
                    "road"_key            = false,
                    "sea_lane"_key        = false,
                    "lost_city_rumor"_key = false,
                  },
              "s"_key =
                  table{
                    "surface"_key         = "water",
                    "ground"_key          = "arctic",
                    "overlay"_key         = null,
                    "river"_key           = null,
                    "ground_resource"_key = null,
                    "forest_resource"_key = null,
                    "irrigation"_key      = false,
                    "road"_key            = false,
                    "sea_lane"_key        = false,
                    "lost_city_rumor"_key = false,
                  },
              "e"_key =
                  table{
                    "surface"_key         = "water",
                    "ground"_key          = "arctic",
                    "overlay"_key         = null,
                    "river"_key           = null,
                    "ground_resource"_key = null,
                    "forest_resource"_key = null,
                    "irrigation"_key      = false,
                    "road"_key            = false,
                    "sea_lane"_key        = false,
                    "lost_city_rumor"_key = false,
                  },
            },
        "pacific_ocean_endpoints"_key = list{},
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
// static_assert( equality_comparable<MapState> );
// static_assert( equality_comparable<TerrainState> );
// static_assert( equality_comparable<RootState> );

TEST_CASE( "[game-state] some test" ) {
  cdr::converter conv;
  RootState root_def;
  value const v = conv.to( root_def );
  REQUIRE( v == cdr_game_state_default );
  // Round trip.
  REQUIRE( conv_from_bt<RootState>( conv, v ) == root_def );
  // From the original cdr.
  REQUIRE( conv_from_bt<RootState>(
               conv, cdr_game_state_default ) == root_def );
}

TEST_CASE( "[game-state] RootState convenience methods" ) {
  RootState l;
  RootState r;
  REQUIRE( root_states_equal( l, r ) );
  r.land_view.map_revealed = MapRevealed::entire{};
  REQUIRE_FALSE( root_states_equal( l, r ) );
  RootState old_r = r;
  assign_src_to_dst( as_const( l ), r );
  REQUIRE( root_states_equal( l, r ) );
  REQUIRE_FALSE( root_states_equal( r, old_r ) );
}

} // namespace
} // namespace rn
