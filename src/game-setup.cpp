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

namespace rn {

namespace {

using namespace std;

using ::refl::enum_map;
using ::refl::enum_values;

double select_landmass(
    IRand& rand, enum_map<e_land_mass, int> const& weights ) {
  e_land_mass const land_mass =
      rand.pick_from_weighted_values( weights );
  double const avg = config_map_gen.land_generation
                         .named_landmass_density[land_mass]
                         .fraction;
  double const delta = config_map_gen.land_generation
                           .landmass_density_fluctuation;
  return clamp( avg + rand.uniform_double( -delta, delta ), 0.0,
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
wait<maybe<GameSetup>> create_default_game_setup(
    IEngine& engine, Planes& planes, IGui& gui, IRand& rand,
    lua::state& ) {
  GameSetup setup;

  // category: settings_state
  // ------------------------------------------------------------
  auto const difficulty =
      co_await choose_difficulty_screen( engine, planes );
  if( !difficulty.has_value() ) co_return nothing;
  setup.settings_state.difficulty = *difficulty;

  // category: rules
  // ------------------------------------------------------------
  setup.rules.values = config_options.default_values;

  // category: map
  // ------------------------------------------------------------
  // TODO: measure this.
  enum_map<e_temperature, int> const kTemperatureWeights{
    { e_temperature::cool, 20 },
    { e_temperature::temperate, 60 },
    { e_temperature::warm, 20 },
  };
  // TODO: measure this.
  enum_map<e_climate, int> const kClimateWeights{
    { e_climate::arid, 20 },
    { e_climate::normal, 60 },
    { e_climate::wet, 20 },
  };
  // TODO: measure this.
  static enum_map<e_land_mass, int> const kLandMassWeights{
    { e_land_mass::small, 20 },
    { e_land_mass::moderate, 60 },
    { e_land_mass::large, 20 },
  };
  auto const& map_conf = config_map_gen.land_generation;

  setup.map = MapSetup{
    .source =
        MapSource::generate{
          .terrain =
              GeneratedTerrainSetup{
                .size = { .w = 56, .h = 70 },
                .land_generator =
                    LandGeneratorSetup{
                      .seed           = 0,
                      .target_density = select_landmass(
                          rand, kLandMassWeights ),
                      .remove_Xs_probability =
                          map_conf.remove_x_probability.fraction,
                      .generator_algo =
                          LandGeneratorAlgorithm::seeded_organic{
                            .brush = e_land_brush_type::mixed,
                          },
                    },
                .temperature = rand.pick_from_weighted_values(
                    kTemperatureWeights ),
                .climate = rand.pick_from_weighted_values(
                    kClimateWeights ),
                .add_arctic_tiles = map_conf.add_arctic_tiles,
                .arctic_tile_density =
                    map_conf.arctic_tile_density.fraction,
                .river_density = map_conf.river_density.fraction,
                .major_river_fraction =
                    map_conf.major_river_fraction.fraction,
                .forest_density =
                    map_conf.forest_density.fraction,
                .mountain_density =
                    map_conf.mountain_density.fraction,
                .mountain_range_probability =
                    map_conf.mountain_range_probability
                        .probability,
                .hills_density = map_conf.hills_density.fraction,
                .hills_range_probability =
                    map_conf.hills_range_probability.probability,
                .prime_resources = PrimeResourceSetup::classic{},
                .lcr             = LcrSetup::classic{} } },
    .bonuses_seed = nothing };

  // category: natives
  // ------------------------------------------------------------
  setup.natives.dwelling_frequency = .14;
  // Should include all with default settings.
  setup.natives.tribes = {};

  // category: nations
  // ------------------------------------------------------------
  auto const human = co_await choose_human( gui );
  if( !human.has_value() ) co_return nothing;
  for( e_nation const nation : enum_values<e_nation> ) {
    auto& o = setup.nations.nations[nation].emplace();
    CHECK( human.has_value() );
    o.human = ( *human == nation );
    o.name  = config_nation.nations[nation].default_leader_name;
    if( o.human ) {
      auto const human_name =
          co_await choose_leader_name( gui, nation );
      if( !human_name.has_value() ) co_return nothing;
      o.name = *human_name;
    }
  }

  // category: turns
  // ------------------------------------------------------------
  setup.turns.starting_year =
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
        },
    .bonuses_seed = nothing,
  };

  // TODO: see if we need to override any settings related to the
  // natives, such as dwelling frequency.
  co_return setup;
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

} // namespace rn
