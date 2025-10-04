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
#include "src/ss/terrain-enums.rds.hpp"

// gfx
#include "gfx/cdr-matrix.hpp"

// refl
#include "refl/cdr.hpp"
#include "refl/to-str.hpp"

// cdr
#include "cdr/converter.hpp"
#include "cdr/ext-base.hpp"
#include "cdr/ext-builtin.hpp"
#include "cdr/ext-std.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace ::std;
using namespace ::cdr::literals;

using ::Catch::Contains;
using ::cdr::list;
using ::cdr::null;
using ::cdr::table;
using ::cdr::value;
using ::cdr::testing::conv_from_bt;
using ::gfx::Matrix;
using ::gfx::point;
using ::gfx::size;

value cdr_game_state_default = table{
  "version"_key =
      table{
        "major"_key = 0,
        "minor"_key = 0,
        "patch"_key = 0,
      },
  "settings"_key =
      table{
        "game_setup_options"_key =
            table{
              "difficulty"_key = "discoverer",
              "customized_rules"_key = table{
                "enable_war_of_succession"_key = false,
                "ref_can_spawn_ships"_key = false,
                "withdraw_on_declaration"_key = false,
                "deadline_for_winning"_key = null,
              },
            },
        "in_game_options"_key =
            table{
              "game_menu_options"_key =
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
  "events"_key =
      table{
        "war_of_succession_done"_key = null,
        "tutorial_hints"_key         = table{},
        "one_time_help"_key =
            table{
              "showed_no_sail_high_seas_during_war"_key = false,
            },
        "ref_captured_colony"_key = false,
      },
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
              "dutch"_key       = null,
              "english"_key     = null,
              "french"_key      = null,
              "spanish"_key     = null,
              "ref_dutch"_key   = null,
              "ref_english"_key = null,
              "ref_french"_key  = null,
              "ref_spanish"_key = null,
            },
        "old_world"_key =
            table{
              "dutch"_key =
                  table{
                    "harbor_state"_key =
                        table{
                          "selected_unit"_key = null,
                        },
                    "immigration"_key =
                        table{
                          "immigrants_pool"_key =
                              list{
                                "petty_criminal",
                                "petty_criminal",
                                "petty_criminal",
                              },
                          "num_recruits_rushed"_key = 0,
                        },
                    "taxes"_key =
                        table{
                          "tax_rate"_key              = 0,
                          "next_tax_event_turn"_key   = 0,
                          "king_remarriage_count"_key = 0,
                        },
                    "market"_key =
                        table{
                          "commodities"_key =
                              table{
                                "food"_key =
                                    table{
                                      "bid_price"_key        = 0,
                                      "intrinsic_volume"_key = 0,
                                      "player_traded_volume"_key = 0,
                                      "boycott"_key = false,
                                    },
                                "sugar"_key       = table{
                                      "bid_price"_key        = 0,
                                      "intrinsic_volume"_key = 0,
                                      "player_traded_volume"_key = 0,
                                      "boycott"_key = false,
                                },
                                "tobacco"_key     = table{
                                      "bid_price"_key        = 0,
                                      "intrinsic_volume"_key = 0,
                                      "player_traded_volume"_key = 0,
                                      "boycott"_key = false,
                                },
                                "cotton"_key      = table{
                                      "bid_price"_key        = 0,
                                      "intrinsic_volume"_key = 0,
                                      "player_traded_volume"_key = 0,
                                      "boycott"_key = false,
                                },
                                "furs"_key        = table{
                                      "bid_price"_key        = 0,
                                      "intrinsic_volume"_key = 0,
                                      "player_traded_volume"_key = 0,
                                      "boycott"_key = false,
                                },
                                "lumber"_key      = table{
                                      "bid_price"_key        = 0,
                                      "intrinsic_volume"_key = 0,
                                      "player_traded_volume"_key = 0,
                                      "boycott"_key = false,
                                },
                                "ore"_key         = table{
                                      "bid_price"_key        = 0,
                                      "intrinsic_volume"_key = 0,
                                      "player_traded_volume"_key = 0,
                                      "boycott"_key = false,
                                },
                                "silver"_key      = table{
                                      "bid_price"_key        = 0,
                                      "intrinsic_volume"_key = 0,
                                      "player_traded_volume"_key = 0,
                                      "boycott"_key = false,
                                },
                                "horses"_key      = table{
                                      "bid_price"_key        = 0,
                                      "intrinsic_volume"_key = 0,
                                      "player_traded_volume"_key = 0,
                                      "boycott"_key = false,
                                },
                                "rum"_key         = table{
                                      "bid_price"_key        = 0,
                                      "intrinsic_volume"_key = 0,
                                      "player_traded_volume"_key = 0,
                                      "boycott"_key = false,
                                },
                                "cigars"_key      = table{
                                      "bid_price"_key        = 0,
                                      "intrinsic_volume"_key = 0,
                                      "player_traded_volume"_key = 0,
                                      "boycott"_key = false,
                                },
                                "cloth"_key       = table{
                                      "bid_price"_key        = 0,
                                      "intrinsic_volume"_key = 0,
                                      "player_traded_volume"_key = 0,
                                      "boycott"_key = false,
                                },
                                "coats"_key       = table{
                                      "bid_price"_key        = 0,
                                      "intrinsic_volume"_key = 0,
                                      "player_traded_volume"_key = 0,
                                      "boycott"_key = false,
                                },
                                "trade_goods"_key = table{
                                      "bid_price"_key        = 0,
                                      "intrinsic_volume"_key = 0,
                                      "player_traded_volume"_key = 0,
                                      "boycott"_key = false,
                                },
                                "tools"_key       = table{
                                      "bid_price"_key        = 0,
                                      "intrinsic_volume"_key = 0,
                                      "player_traded_volume"_key = 0,
                                      "boycott"_key = false,
                                },
                                "muskets"_key     = table{
                                      "bid_price"_key        = 0,
                                      "intrinsic_volume"_key = 0,
                                      "player_traded_volume"_key = 0,
                                      "boycott"_key = false,
                                },
                              },
                        },
                  },
              "english"_key     = null,
              "french"_key      = null,
              "spanish"_key     = null,
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
              "dutch"_key       = null,
              "english"_key     = null,
              "french"_key      = null,
              "spanish"_key     = null,
              "ref_dutch"_key   = null,
              "ref_english"_key = null,
              "ref_french"_key  = null,
              "ref_spanish"_key = null,
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

void init_cdr_data() {
  auto& old_world =
      cdr_game_state_default["players"]["old_world"];
  CHECK( old_world["english"] == null );
  // Save typing above by just keeping one of the players
  // (dutch), since they are all the same.
  auto const dutch     = old_world["dutch"];
  old_world["english"] = dutch;
  old_world["french"]  = dutch;
  old_world["spanish"] = dutch;
}

TEST_CASE( "[ss/ref] some test" ) {
  init_cdr_data();
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

TEST_CASE( "[ss/ref] RootState convenience methods" ) {
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

TEST_CASE( "[ss/ref] validate_full_game_state" ) {
  SS ss;
  RootState& root = ss.root;

  auto const f = [&] [[clang::noinline]] {
    return ss.as_const.validate_full_game_state();
  };

  REQUIRE( f() == valid );

  root.zzz_terrain.modify_entire_map( []( RealTerrain& trn ) {
    trn.map = Matrix<MapSquare>( size{ .w = 2, .h = 2 } );
  } );
  REQUIRE( f() == valid );

  root.zzz_terrain.mutable_square_at( point{ .x = 0, .y = 1 } ) =
      MapSquare{ .surface = e_surface::water, .road = false };
  REQUIRE( f() == valid );

  root.zzz_terrain.mutable_square_at( point{ .x = 0, .y = 1 } ) =
      MapSquare{ .surface = e_surface::water, .road = true };
  REQUIRE_THAT( f().error(),
                Contains( "water tiles cannot have a road" ) );

  root.turn.time_point.year = -1;
  REQUIRE_THAT( f().error(),
                Contains( "game year must be >= 0" ) );

  root.version.major = -1;
  REQUIRE_THAT(
      f().error(),
      Contains( "major version number must be >= 0" ) );
}

TEST_CASE( "[ss/ref] validate_non_terrain_game_state" ) {
  SS ss;
  RootState& root = ss.root;

  auto const f = [&] [[clang::noinline]] {
    return ss.as_const.validate_non_terrain_game_state();
  };

  REQUIRE( f() == valid );

  root.zzz_terrain.modify_entire_map( []( RealTerrain& trn ) {
    trn.map = Matrix<MapSquare>( size{ .w = 2, .h = 2 } );
  } );
  REQUIRE( f() == valid );

  root.zzz_terrain.mutable_square_at( point{ .x = 0, .y = 1 } ) =
      MapSquare{ .surface = e_surface::water, .road = false };
  REQUIRE( f() == valid );

  root.zzz_terrain.mutable_square_at( point{ .x = 0, .y = 1 } ) =
      MapSquare{ .surface = e_surface::water, .road = true };
  // NOTE: terrain validation is not run.
  REQUIRE( f() == valid );

  root.turn.time_point.year = -1;
  REQUIRE_THAT( f().error(),
                Contains( "game year must be >= 0" ) );

  root.version.major = -1;
  REQUIRE_THAT(
      f().error(),
      Contains( "major version number must be >= 0" ) );
}

} // namespace
} // namespace rn
