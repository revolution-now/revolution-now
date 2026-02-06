/****************************************************************
**test-map.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-01-04.
*
* Description: Generates maps for testing and prints them to the
*              console.
*
*****************************************************************/
#include "test-map.hpp"

// Revolution Now
#include "ascii-map.hpp"
#include "connectivity.hpp"
#include "create-game.hpp"
#include "game-setup.hpp"
#include "iengine.hpp"
#include "irand.hpp"
#include "lua.hpp"
#include "map-updater.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/root.hpp"

// luapp
#include "luapp/ext-refl.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// rcl
#include "rcl/parse.hpp"

// cdr
#include "cdr/ext-base.hpp"
#include "cdr/ext-builtin.hpp"
#include "cdr/ext-std.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

// C++ standard library
#include <iostream>

namespace rn {

namespace {

using namespace std;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
void load_testing_game_setup( GameSetup& setup ) {
  UNWRAP_CHECK_T( rcl::doc const& doc,
                  rcl::parse_file( "keys/game-exchange-key.rcl",
                                   rcl::ProcessingOptions{} ) );
  cdr::converter::options const options{
    .allow_unrecognized_fields        = true,
    .default_construct_missing_fields = false,
  };
  UNWRAP_CHECK_T( setup,
                  cdr::run_conversion_from_canonical<GameSetup>(
                      doc.top_val(), options ) );
}

void testing_map_gen( IEngine& engine ) {
  lua::state st;
  lua_init( engine, st );
  SS ss;
  st["ROOT"] = ss.root;
  st["SS"]   = ss;
  TerrainConnectivity connectivity;
  NonRenderingMapUpdater non_rendering_map_updater(
      ss, connectivity );
  st["IMapUpdater"] =
      static_cast<IMapUpdater&>( non_rendering_map_updater );
  IRand& rand = engine.rand();
  st["IRand"] = static_cast<IRand&>( rand );

  // ------------------------------------------------------------
  // GameSetup
  // ------------------------------------------------------------
  GameSetup setup;
  load_testing_game_setup( setup );
#if 1
  auto entropy = rand.generate_deterministic_seed();
  PerlinSeed const perlin_seed{
    .offset_x = entropy.consume<uint32_t>(),
    .offset_y = entropy.consume<uint32_t>(),
    .base     = entropy.consume<uint32_t>(),
  };
  setup.map.source.get_if<MapSource::generate_native>()
      ->setup.surface_generator.perlin_settings.seed =
      perlin_seed;
#endif

  // ------------------------------------------------------------
  // Generate map.
  // ------------------------------------------------------------
  CHECK_HAS_VALUE(
      create_game_from_setup( ss, rand, st, setup ) );

  // ------------------------------------------------------------
  // Print map.
  // ------------------------------------------------------------
  print_ascii_map( ss.terrain.real_terrain(), cout );
}

} // namespace rn
