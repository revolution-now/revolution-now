/****************************************************************
**game-setup.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-20.
*
* Description: Creates UI sequences used to game-setup the game.
*
*****************************************************************/
#include "game-setup.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "difficulty-screen.hpp"
#include "game-setup.rds.hpp"
#include "igui.hpp"
#include "irand.hpp"

// config
#include "config/map-gen.rds.hpp"
#include "config/nation.rds.hpp"
#include "config/options.rds.hpp"
#include "config/turn.rds.hpp"

// refl
#include "refl/query-enum.hpp"

// base
#include "base/logger.hpp"

namespace rn {

namespace {

using namespace std;

using ::base::valid;
using ::base::valid_or;
using ::gfx::size;
using ::refl::enum_map;
using ::refl::enum_values;

double density_for_landmass( IRand& rand,
                             e_land_mass const land_mass ) {
  auto const& range =
      config_map_gen.terrain_generation.land_layout.land_mass
          .densities[land_mass];
  return clamp( rand.uniform_double( range.lo, range.hi ), 0.0,
                1.0 );
}

wait<maybe<e_nation>> choose_human( IGui& gui ) {
  EnumChoiceConfig const config{
    .msg = "Select Your Nation:",
  };
  // TODO: make this into a plane.
  co_return co_await gui.optional_enum_choice<e_nation>(
      config );
}

// TODO: make this a transparent window, kind of like the OG.
wait<maybe<string>> choose_leader_name( IGui& gui,
                                        e_nation const nation ) {
  co_return co_await gui.optional_string_input(
      StringInputConfig{
        .msg          = "Please Enter Your Name:",
        .initial_text = config_nation.nations[nation]
                            .default_leader_name } );
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
wait<maybe<GameSetup>> create_classic_customized_game_setup(
    IEngine& engine, Planes& planes, IGui& gui, IRand& rand,
    ClassicCustomization const& classic_customization ) {
  GameSetup setup;

  // category: settings_state
  // ------------------------------------------------------------
  auto const difficulty =
      co_await choose_difficulty_screen( engine, planes );
  if( !difficulty.has_value() ) co_return nothing;
  setup.settings.difficulty = *difficulty;

  // category: rules
  // ------------------------------------------------------------
  setup.settings.rules = config_options.default_values;

  // category: map
  // ------------------------------------------------------------
  size const world_sz = config_map_gen.default_map_size;

  auto const& map_conf = config_map_gen.terrain_generation;

  // This seed is sufficient to generate the numbers that we want
  // because it happens that the three uint32_t values that con-
  // stitute the perlin seed fit within the 128 bits that are
  // provided by the rng::seed type, so we can read the seed bits
  // directly instead of generating random numbers.
  rng::seed perlin_entropy = rand.generate_deterministic_seed();
  PerlinSeed const perlin_seed{
    .offset_x = perlin_entropy.consume<uint32_t>(),
    .offset_y = perlin_entropy.consume<uint32_t>(),
    .base     = perlin_entropy.consume<uint32_t>(),
  };

  PerlinMapSettings const perlin_settings{
    .land_form =
        map_conf.land_layout.land_form
            .perlin_land_form[classic_customization.land_form],
    .edge_suppression =
        map_conf.land_layout.land_form.perlin_edge_suppression,
    .seed = perlin_seed,
  };

  SurfaceSanitizationSetup const surface_sanitization{
    .remove_crosses = map_conf.land_layout.remove_crosses,
    .remove_islands = true,
  };

  SurfaceGeneratorSetup const surface_generator{
    .perlin_settings = perlin_settings,
    .target_density  = density_for_landmass(
        rand, classic_customization.land_mass ),
    .sanitization = surface_sanitization,
  };

  e_temperature const temperature =
      rand.pick_from_weighted_values(
          map_conf.temperature.weights );
  e_climate const climate =
      rand.pick_from_weighted_values( map_conf.climate.weights );

  GroundTypeSetup const ground_types{
    .seed        = rand.generate_deterministic_seed(),
    .temperature = temperature,
    .climate     = climate,
  };

  ArcticSetup const arctic{
    .seed = rand.generate_deterministic_seed(),
    .enabled =
        map_conf.land_layout.arctic.enabled &&
        world_sz.h >= map_conf.land_layout.arctic.min_map_height,
    .density = map_conf.land_layout.arctic.density.fraction,
  };

  RiverSetup const rivers{
    .seed    = rand.generate_deterministic_seed(),
    .density = map_conf.rivers.density.fraction,
    .major_river_fraction =
        map_conf.rivers.major_river_fraction.fraction,
  };

  MountainsSetup const mountains{
    .seed    = rand.generate_deterministic_seed(),
    .density = map_conf.overlay.mountain_density.fraction,
    .range_probability =
        map_conf.overlay.mountain_range_probability.probability,
  };

  HillsSetup const hills{
    .seed    = rand.generate_deterministic_seed(),
    .density = map_conf.overlay.hills_density.fraction,
    .range_probability =
        map_conf.overlay.hills_range_probability.probability,
  };

  ForestSetup const forest{
    .seed    = rand.generate_deterministic_seed(),
    .density = map_conf.overlay.forest_density.fraction,
  };

  BonusesSetup const bonuses{
    .seed = rand.generate_deterministic_seed(),
  };

  GeneratedMapSetup const generated_map_setup{
    .size              = world_sz,
    .surface_generator = surface_generator,
    .ground_types      = ground_types,
    .arctic            = arctic,
    .rivers            = rivers,
    .mountains         = mountains,
    .hills             = hills,
    .forest            = forest,
    .bonuses           = bonuses,
  };

  setup.map = MapSetup{ .source = MapSource::generate_lua{
                          .setup = generated_map_setup } };

  // category: natives
  // ------------------------------------------------------------
  for( e_tribe const tribe_type : enum_values<e_tribe> ) {
    auto& tribe_setup =
        setup.natives.tribes[tribe_type].emplace();
    // TODO
    (void)tribe_setup;
  }

  // category: nations
  // ------------------------------------------------------------
  auto const human_nation = co_await choose_human( gui );
  if( !human_nation.has_value() ) co_return nothing;
  for( e_nation const nation : enum_values<e_nation> ) {
    using enum e_player_control;
    auto& o = setup.nations.nations[nation].emplace();
    CHECK( human_nation.has_value() );
    o.control = ( *human_nation == nation ) ? human : ai;
    o.name = config_nation.nations[nation].default_leader_name;
    if( o.control == human ) {
      auto const human_name =
          co_await choose_leader_name( gui, nation );
      if( !human_name.has_value() ) co_return nothing;
      o.name = *human_name;
    }
  }

  // category: turns
  // ------------------------------------------------------------
  setup.settings.starting_year =
      config_turn.game_start.starting_year;

  co_return setup;
}

wait<maybe<GameSetup>> create_america_game_setup(
    IEngine& engine, Planes& planes, IGui& gui, IRand& rand,
    lua::state& lua ) {
  maybe<GameSetup> setup;
  setup = co_await create_default_game_setup( engine, planes,
                                              gui, rand, lua );
  if( !setup.has_value() ) co_return setup;
  setup->map = MapSetup{
    .source =
        MapSource::load_from_file{
          // TODO: get this via a file selection dialog, and then
          // potentially cache it in the user settings.
          .path = "test/data/saves/classic/map/AMER2.MP",
          .type = { .epoch    = e_map_file_epoch::classic,
                    .format   = e_map_file_format::binary,
                    .contents = e_map_file_contents::map_only },
          // The OG seems hard-coded to detect when a file named
          // AMER2.MP is loaded and it will place a mountain on
          // all of the islands on the map (there appear to be
          // three of them), likely to prevent founding a colony
          // there which would allow cheating in a sense.
          .convert_islands_to_mountains = true,
          .bonuses =
              BonusesSetup{
                .seed = rand.generate_deterministic_seed() } },
  };

  // TODO: see if we need to override any settings related to the
  // natives, such as dwelling frequency.
  co_return setup;
}

wait<maybe<GameSetup>> create_default_game_setup(
    IEngine& engine, Planes& planes, IGui& gui, IRand& rand,
    lua::state& ) {
  auto const& map_conf = config_map_gen.terrain_generation;
  ClassicCustomization const params{
    .land_mass = rand.pick_from_weighted_values(
        map_conf.land_layout.land_mass.weights ),
    .land_form = rand.pick_from_weighted_values(
        map_conf.land_layout.land_form.weights ),
    .temperature = rand.pick_from_weighted_values(
        map_conf.temperature.weights ),
    .climate = rand.pick_from_weighted_values(
        map_conf.climate.weights ),
  };
  co_return co_await create_classic_customized_game_setup(
      engine, planes, gui, rand, params );
}

wait<maybe<GameSetup>> create_customized_game_setup(
    IEngine&, Planes&, IGui&, e_customization_mode const mode ) {
  switch( mode ) {
    using enum e_customization_mode;
    case classic: {
      // TODO
      NOT_IMPLEMENTED;
      // break;
    }
    case modern: {
      // TODO
      NOT_IMPLEMENTED;
      // break;
    }
    case extreme: {
      // TODO
      NOT_IMPLEMENTED;
      // break;
    }
  }
  co_return nothing;
}

valid_or<string> validate_game_setup( GameSetup const& setup ) {
  (void)setup;
  return valid;
}

} // namespace rn
