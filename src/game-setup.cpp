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
#include "iengine.hpp"
#include "igui.hpp"
#include "irand.hpp"
#include "perlin-map.hpp"

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

[[nodiscard]] double sample_normal(
    IRand& rand, config::BoundedNormalDist const& params ) {
  return clamp( rand.normal( params.mean, params.stddev ),
                params.min, params.max );
}

wait<maybe<e_nation>> choose_human( IGui& gui ) {
  EnumChoiceConfig const config{
    .msg = "Select Your Nation:",
  };
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
GameSetup create_classic_game_setup(
    IRand& rand,
    ClassicGameSetupParamsEvaluated const& params ) {
  GameSetup setup;

  // category: settings_state
  // ------------------------------------------------------------
  setup.settings.difficulty = params.common.difficulty;

  // category: rules
  // ------------------------------------------------------------
  setup.settings.rules = config_options.default_values;

  // category: map
  // ------------------------------------------------------------
  size const world_sz = config_map_gen.default_map_size;

  auto const& map_conf = config_map_gen.terrain_generation;

  PerlinSeed const perlin_seed =
      generate_perlin_seed( rand.generate_deterministic_seed() );

  PerlinMapSettings const perlin_settings{
    .land_form = params.land_form,
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
    .arctic{
      .seed    = rand.generate_deterministic_seed(),
      .enabled = map_conf.land_layout.arctic.enabled &&
                 world_sz.h >=
                     map_conf.land_layout.arctic.min_map_height,
      .density = map_conf.land_layout.arctic.density.fraction,
    },
    .target_density = params.land_density,
    .sanitization   = surface_sanitization,
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
    .rivers            = rivers,
    .mountains         = mountains,
    .hills             = hills,
    .forest            = forest,
    .bonuses           = bonuses,
  };

  setup.map = MapSetup{ .source = MapSource::generate_native{
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
  e_nation const human_nation = params.common.player;
  for( e_nation const nation : enum_values<e_nation> ) {
    using enum e_player_control;
    auto& o   = setup.nations.nations[nation].emplace();
    o.control = ( human_nation == nation ) ? human : ai;
    o.name = config_nation.nations[nation].default_leader_name;
    if( o.control == human ) o.name = params.common.player_name;
  }

  // category: turns
  // ------------------------------------------------------------
  setup.settings.starting_year =
      config_turn.game_start.starting_year;

  return setup;
}

GameSetup create_default_game_setup(
    IRand& rand, ClassicGameSetupParamsCommon const& common ) {
  auto const& map_conf = config_map_gen.terrain_generation;
  PerlinLandForm const perlin_land_form{
    .scale = sample_normal(
        rand,
        map_conf.land_layout.land_form.fully_random.scale ),
    .fractal =
        map_conf.land_layout.land_form.fully_random.fractal };

  ClassicGameSetupParamsEvaluated const params{
    .common       = common,
    .land_density = sample_normal(
        rand, map_conf.land_layout.land_mass.fully_random ),
    .land_form   = perlin_land_form,
    .temperature = rand.pick_from_weighted_values(
        map_conf.temperature.weights ),
    .climate = rand.pick_from_weighted_values(
        map_conf.climate.weights ),
  };
  return create_classic_game_setup( rand, params );
}

GameSetup create_classic_customized_game_setup(
    IRand& rand, ClassicGameSetupParams const& params ) {
  auto const& map_conf      = config_map_gen.terrain_generation;
  double const land_density = sample_normal(
      rand, map_conf.land_layout.land_mass
                .customized[params.custom.land_mass] );
  PerlinLandForm const perlin_land_form{
    .scale = sample_normal(
        rand, map_conf.land_layout.land_form
                  .customized[params.custom.land_form]
                  .scale ),
    .fractal = map_conf.land_layout.land_form
                   .customized[params.custom.land_form]
                   .fractal };

  ClassicGameSetupParamsEvaluated const evaluated{
    .common       = params.common,
    .land_density = land_density,
    .land_form    = perlin_land_form,
    .temperature  = params.custom.temperature,
    .climate      = params.custom.climate,
  };

  return create_classic_game_setup( rand, evaluated );
}

GameSetup create_america_game_setup(
    IRand& rand, ClassicGameSetupParamsCommon const& common ) {
  GameSetup setup = create_default_game_setup( rand, common );

  setup.map = MapSetup{
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
  return setup;
}

wait<maybe<ClassicGameSetupParamsCommon>>
create_classic_game_common_params( IEngine& engine,
                                   Planes& planes, IGui& gui ) {
  auto const difficulty =
      co_await choose_difficulty_screen( engine, planes );
  if( !difficulty.has_value() ) co_return nothing;

  auto const nation = co_await choose_human( gui );
  if( !nation.has_value() ) co_return nothing;

  auto const human_name =
      co_await choose_leader_name( gui, *nation );
  if( !human_name.has_value() ) co_return nothing;

  co_return ClassicGameSetupParamsCommon{
    .difficulty  = *difficulty,
    .player      = *nation,
    .player_name = *human_name };
}

wait<maybe<ClassicGameSetupParamsCustom>>
create_classic_game_custom_params( IEngine&, Planes&,
                                   IGui& gui ) {
  ClassicGameSetupParamsCustom params;
  {
    EnumChoiceConfig const config{
      .msg = "Select Land Mass:",
    };
    UNWRAP_CO_RETURN(
        params.land_mass,
        co_await gui.optional_enum_choice<e_land_mass>(
            config ) );
  }
  {
    EnumChoiceConfig const config{
      .msg = "Select Land Form:",
    };
    UNWRAP_CO_RETURN(
        params.land_form,
        co_await gui.optional_enum_choice<e_land_form>(
            config ) );
  }
  {
    EnumChoiceConfig const config{
      .msg = "Select Temperature:",
    };
    UNWRAP_CO_RETURN(
        params.temperature,
        co_await gui.optional_enum_choice<e_temperature>(
            config ) );
  }
  {
    EnumChoiceConfig const config{
      .msg = "Select Climate:",
    };
    UNWRAP_CO_RETURN(
        params.climate,
        co_await gui.optional_enum_choice<e_climate>( config ) );
  }
  co_return params;
}

wait<maybe<GameSetup>> create_customized_game_setup(
    IEngine& engine, Planes& planes, IGui& gui,
    e_customization_mode const mode ) {
  switch( mode ) {
    using enum e_customization_mode;
    case classic: {
      ClassicGameSetupParams params;
      UNWRAP_CO_RETURN(
          params.custom,
          co_await create_classic_game_custom_params(
              engine, planes, gui ) );
      UNWRAP_CO_RETURN(
          params.common,
          co_await create_classic_game_common_params(
              engine, planes, gui ) );
      co_return create_classic_customized_game_setup(
          engine.rand(), params );
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
