/****************************************************************
**fathers.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-07.
*
* Description: Unit tests for the src/fathers.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/fathers.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/irand.hpp"

// Revolution Now
#include "src/mock/matchers.hpp"

// ss
#include "ss/player.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/terrain.hpp"
#include "ss/tribe.rds.hpp"
#include "ss/turn.hpp"
#include "ss/units.hpp"

// gfx
#include "gfx/iter.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::_;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_player( e_nation::dutch );
    add_player( e_nation::french );
    set_default_player( e_nation::dutch );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        // clang-format off
        L, L, L,
        L, L, L,
        L, _, _,
        // clang-format on
    };
    build_map( std::move( tiles ), 3 );
  }

  void create_large_map() {
    MapSquare const   O = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles;
    tiles.reserve( 20 * 20 );
    for( int y = 0; y < 20; ++y ) {
      for( int x = 0; x < 20; ++x ) {
        if( x == 0 || x == 19 )
          tiles.push_back( O );
        else
          tiles.push_back( L );
      }
    }
    build_map( std::move( tiles ), 20 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[fathers] has_all_fathers" ) {
  World   W;
  Player& player = W.default_player();

  REQUIRE_FALSE( has_all_fathers( player ) );

  player.fathers.has[e_founding_father::adam_smith] = true;
  REQUIRE_FALSE( has_all_fathers( player ) );

  FOR_ENUM( father, e_founding_father )
    player.fathers.has[father] = true;
  REQUIRE( has_all_fathers( player ) );

  player.fathers.has[e_founding_father::adam_smith] = false;
  REQUIRE_FALSE( has_all_fathers( player ) );
}

TEST_CASE( "[fathers] bells_needed_for_next_father" ) {
  World   W;
  Player& player = W.default_player();

  auto f = [&] {
    return bells_needed_for_next_father( W.ss(), player );
  };

  SECTION( "discoverer" ) {
    W.settings().difficulty = e_difficulty::discoverer;
    REQUIRE( f() == 48 / 2 );

    player.fathers.has[e_founding_father::adam_smith] = true;
    REQUIRE( f() == 48 * 2 + 1 );

    player.fathers.has[e_founding_father::hernan_cortes] = true;
    REQUIRE( f() == 48 * 3 + 1 );

    player.fathers.has[e_founding_father::hernando_de_soto] =
        true;
    REQUIRE( f() == 48 * 4 + 1 );

    FOR_ENUM( father, e_founding_father )
      player.fathers.has[father] = true;

    player.fathers.has[e_founding_father::adam_smith] = false;
    REQUIRE( f() == 48 * 25 + 1 );

    player.fathers.has[e_founding_father::adam_smith] = true;
    REQUIRE( f() == nothing );
  }

  SECTION( "conquistador" ) {
    W.settings().difficulty = e_difficulty::conquistador;
    REQUIRE( f() == 80 / 2 );

    player.fathers.has[e_founding_father::adam_smith] = true;
    REQUIRE( f() == 80 * 2 + 1 );

    player.fathers.has[e_founding_father::hernan_cortes] = true;
    REQUIRE( f() == 80 * 3 + 1 );

    player.fathers.has[e_founding_father::hernando_de_soto] =
        true;
    REQUIRE( f() == 80 * 4 + 1 );

    FOR_ENUM( father, e_founding_father )
      player.fathers.has[father] = true;

    player.fathers.has[e_founding_father::adam_smith] = false;
    REQUIRE( f() == 80 * 25 + 1 );

    player.fathers.has[e_founding_father::adam_smith] = true;
    REQUIRE( f() == nothing );
  }

  SECTION( "viceroy" ) {
    W.settings().difficulty = e_difficulty::viceroy;
    REQUIRE( f() == 112 / 2 );

    player.fathers.has[e_founding_father::adam_smith] = true;
    REQUIRE( f() == 112 * 2 + 1 );

    player.fathers.has[e_founding_father::hernan_cortes] = true;
    REQUIRE( f() == 112 * 3 + 1 );

    player.fathers.has[e_founding_father::hernando_de_soto] =
        true;
    REQUIRE( f() == 112 * 4 + 1 );

    FOR_ENUM( father, e_founding_father )
      player.fathers.has[father] = true;

    player.fathers.has[e_founding_father::adam_smith] = false;
    REQUIRE( f() == 112 * 25 + 1 );

    player.fathers.has[e_founding_father::adam_smith] = true;
    REQUIRE( f() == nothing );
  }
}

TEST_CASE( "[fathers] pick_founding_father_if_needed" ) {
  World   W;
  Player& player = W.default_player();

  auto f = [&] {
    wait<> w =
        pick_founding_father_if_needed( W.ss(), W.ts(), player );
    BASE_CHECK( !w.exception() );
    BASE_CHECK( w.ready() );
  };

  auto fathers_copy = player.fathers;

  SECTION( "in progress" ) {
    player.fathers.in_progress =
        e_founding_father::george_washington;
    fathers_copy = player.fathers;
    f();
    REQUIRE( player.fathers == fathers_copy );
  }

  SECTION( "not in progress, zero bells" ) {
    player.fathers.in_progress = nothing;
    fathers_copy               = player.fathers;
    f();
    REQUIRE( player.fathers == fathers_copy );

    // In-progress with father that player already has.
    player.fathers.has[e_founding_father::george_washington] =
        true;
    player.fathers.in_progress =
        e_founding_father::george_washington;
    fathers_copy = player.fathers;
    f();
    fathers_copy.in_progress = nothing;
    REQUIRE( player.fathers == fathers_copy );
  }

  player.fathers.in_progress = nothing;
  player.fathers.bells       = 1;

  auto expect_rand = [&]( int low, int high, int res ) {
    W.rand()
        .EXPECT__between_ints( low, high, e_interval::half_open )
        .returns( res );
  };

  SECTION( "fills pool and chooses, year 1500" ) {
    W.turn().time_point.year = 1500;
    // These ranges are sums of the relevant father weights for
    // the year in question.
    expect_rand( 0, 15, 2 );
    expect_rand( 0, 27, 1 );
    expect_rand( 0, 20, 16 );
    expect_rand( 0, 17, 12 );
    expect_rand( 0, 24, 0 );
    ChoiceConfig const config{
        .msg =
            "Which Founding Father shall we appoint as the next "
            "member of the Continental Congress?",
        .options = {
            ChoiceConfigOption{
                .key          = "peter_minuit",
                .display_name = "Peter Minuit (Trade Advisor)" },
            ChoiceConfigOption{
                .key = "ferdinand_magellan",
                .display_name =
                    "Ferdinand Magellan (Exploration Advisor)" },
            ChoiceConfigOption{
                .key = "francis_drake",
                .display_name =
                    "Francis Drake (Military Advisor)" },
            ChoiceConfigOption{
                .key = "benjamin_franklin",
                .display_name =
                    "Benjamin Franklin (Political Advisor)" },
            ChoiceConfigOption{
                .key = "william_brewster",
                .display_name =
                    "William Brewster (Religious Advisor)" },
        } };
    W.gui()
        .EXPECT__choice( config, e_input_required::yes )
        .returns( make_wait<maybe<string>>( "francis_drake" ) );

    fathers_copy = player.fathers;
    f();

    fathers_copy.pool[e_founding_father_type::trade] =
        e_founding_father::peter_minuit;
    fathers_copy.pool[e_founding_father_type::exploration] =
        e_founding_father::ferdinand_magellan;
    fathers_copy.pool[e_founding_father_type::military] =
        e_founding_father::francis_drake;
    fathers_copy.pool[e_founding_father_type::political] =
        e_founding_father::benjamin_franklin;
    fathers_copy.pool[e_founding_father_type::religious] =
        e_founding_father::william_brewster;
    fathers_copy.in_progress = e_founding_father::francis_drake;
    fathers_copy.bells       = 0;
    REQUIRE( player.fathers == fathers_copy );
  }

  SECTION(
      "fills pool and chooses, year 1600, one slot populated" ) {
    W.turn().time_point.year = 1600;
    player.fathers.pool[e_founding_father_type::political] =
        e_founding_father::benjamin_franklin;
    // These ranges are sums of the relevant father weights for
    // the year in question.
    expect_rand( 0, 24, 8 );
    expect_rand( 0, 31, 25 );
    expect_rand( 0, 25, 19 );
    // expect_rand( 0, 21, 0 ); // political already present.
    expect_rand( 0, 28, 0 );
    ChoiceConfig const config{
        .msg =
            "Which Founding Father shall we appoint as the next "
            "member of the Continental Congress?",
        .options = {
            ChoiceConfigOption{
                .key          = "jakob_fugger",
                .display_name = "Jakob Fugger (Trade Advisor)" },
            ChoiceConfigOption{
                .key = "henry_hudson",
                .display_name =
                    "Henry Hudson (Exploration Advisor)" },
            ChoiceConfigOption{
                .key = "john_paul_jones",
                .display_name =
                    "John Paul Jones (Military Advisor)" },
            ChoiceConfigOption{
                .key = "benjamin_franklin",
                .display_name =
                    "Benjamin Franklin (Political Advisor)" },
            ChoiceConfigOption{
                .key = "william_brewster",
                .display_name =
                    "William Brewster (Religious Advisor)" },
        } };
    W.gui()
        .EXPECT__choice( config, e_input_required::yes )
        .returns( make_wait<maybe<string>>( "henry_hudson" ) );

    fathers_copy = player.fathers;
    f();

    fathers_copy.pool[e_founding_father_type::trade] =
        e_founding_father::jakob_fugger;
    fathers_copy.pool[e_founding_father_type::exploration] =
        e_founding_father::henry_hudson;
    fathers_copy.pool[e_founding_father_type::military] =
        e_founding_father::john_paul_jones;
    fathers_copy.pool[e_founding_father_type::religious] =
        e_founding_father::william_brewster;
    fathers_copy.in_progress = e_founding_father::henry_hudson;
    fathers_copy.bells       = 0;
    REQUIRE( player.fathers == fathers_copy );
  }

  SECTION(
      "fills pool and chooses, year 1700, one slot populated" ) {
    W.turn().time_point.year = 1700;
    // These ranges are sums of the relevant father weights for
    // the year in question.
    expect_rand( 0, 32, 22 );
    expect_rand( 0, 25, 22 );
    expect_rand( 0, 25, 24 );
    expect_rand( 0, 28, 9 );
    expect_rand( 0, 17, 6 );
    ChoiceConfig const config{
        .msg =
            "Which Founding Father shall we appoint as the next "
            "member of the Continental Congress?",
        .options = {
            ChoiceConfigOption{
                .key          = "jan_de_witt",
                .display_name = "Jan de Witt (Trade Advisor)" },
            ChoiceConfigOption{
                .key = "sieur_de_la_salle",
                .display_name =
                    "Sieur de La Salle (Exploration Advisor)" },
            ChoiceConfigOption{
                .key = "john_paul_jones",
                .display_name =
                    "John Paul Jones (Military Advisor)" },
            ChoiceConfigOption{
                .key = "thomas_paine",
                .display_name =
                    "Thomas Paine (Political Advisor)" },
            ChoiceConfigOption{
                .key = "juan_de_sepulveda",
                .display_name =
                    "Juan de Sepulveda (Religious Advisor)" },
        } };
    W.gui()
        .EXPECT__choice( config, e_input_required::yes )
        .returns( make_wait<maybe<string>>( "thomas_paine" ) );

    fathers_copy = player.fathers;
    f();

    fathers_copy.pool[e_founding_father_type::trade] =
        e_founding_father::jan_de_witt;
    fathers_copy.pool[e_founding_father_type::exploration] =
        e_founding_father::sieur_de_la_salle;
    fathers_copy.pool[e_founding_father_type::military] =
        e_founding_father::john_paul_jones;
    fathers_copy.pool[e_founding_father_type::political] =
        e_founding_father::thomas_paine;
    fathers_copy.pool[e_founding_father_type::religious] =
        e_founding_father::juan_de_sepulveda;
    fathers_copy.in_progress = e_founding_father::thomas_paine;
    fathers_copy.bells       = 0;
    REQUIRE( player.fathers == fathers_copy );
  }

  SECTION( "one father left" ) {
    FOR_ENUM( father, e_founding_father )
      player.fathers.has[father] = true;
    player.fathers.has[e_founding_father::adam_smith] = false;
    W.turn().time_point.year                          = 1700;
    expect_rand( 0, 6, 2 );
    ChoiceConfig const config{
        .msg =
            "Which Founding Father shall we appoint as the next "
            "member of the Continental Congress?",
        .options = {
            ChoiceConfigOption{
                .key          = "adam_smith",
                .display_name = "Adam Smith (Trade Advisor)" },
        } };
    W.gui()
        .EXPECT__choice( config, e_input_required::yes )
        .returns( make_wait<maybe<string>>( "adam_smith" ) );
    fathers_copy = player.fathers;
    f();
    fathers_copy.pool[e_founding_father_type::trade] =
        e_founding_father::adam_smith;
    fathers_copy.in_progress = e_founding_father::adam_smith;
    fathers_copy.bells       = 0;
    REQUIRE( player.fathers == fathers_copy );
  }

  SECTION( "all fathers present" ) {
    FOR_ENUM( father, e_founding_father )
      player.fathers.has[father] = true;
    fathers_copy = player.fathers;
    f();
    REQUIRE( player.fathers == fathers_copy );
  }
}

TEST_CASE( "[fathers] check_founding_fathers" ) {
  World   W;
  Player& player = W.default_player();

  auto f = [&] {
    return check_founding_fathers( W.ss(), player );
  };

  auto fathers_copy = player.fathers;

  W.settings().difficulty = e_difficulty::conquistador;

  player.fathers.pool[e_founding_father_type::trade] =
      e_founding_father::jan_de_witt;
  player.fathers.pool[e_founding_father_type::exploration] =
      e_founding_father::sieur_de_la_salle;
  player.fathers.pool[e_founding_father_type::military] =
      e_founding_father::hernan_cortes;
  player.fathers.pool[e_founding_father_type::political] =
      e_founding_father::thomas_paine;
  player.fathers.pool[e_founding_father_type::religious] =
      e_founding_father::juan_de_sepulveda;

  player.fathers.in_progress = nothing;
  fathers_copy               = player.fathers;
  REQUIRE( f() == nothing );
  REQUIRE( player.fathers == fathers_copy );

  player.fathers.has[e_founding_father::hernando_de_soto] = true;
  player.fathers.in_progress = e_founding_father::hernan_cortes;
  // Takes 161 bells.

  player.fathers.bells = 1;
  fathers_copy         = player.fathers;
  REQUIRE( f() == nothing );

  player.fathers.bells = 160;
  fathers_copy         = player.fathers;
  REQUIRE( f() == nothing );

  player.fathers.bells = 162; // should leave one remaining.
  fathers_copy         = player.fathers;
  REQUIRE( f() == e_founding_father::hernan_cortes );
  fathers_copy.has[e_founding_father::hernan_cortes]  = true;
  fathers_copy.bells                                  = 1;
  fathers_copy.in_progress                            = nothing;
  fathers_copy.pool[e_founding_father_type::military] = nothing;
  REQUIRE( player.fathers == fathers_copy );
}

TEST_CASE( "[fathers] on_father_received: john_paul_jones" ) {
  World W;
  W.update_terrain_connectivity();
  Player&                 player = W.default_player();
  e_founding_father const father =
      e_founding_father::john_paul_jones;

  REQUIRE( W.units().all().size() == 0 );
  player.fathers.has[father] = true;
  on_father_received( W.ss(), W.ts(), player, father );
  REQUIRE( W.units().all().size() == 1 );
  EuroUnitState const& state =
      as_const( W.units() ).state_of( UnitId{ 1 } );
  Unit const& unit = state.unit;
  REQUIRE( unit.type() == e_unit_type::frigate );
  REQUIRE( unit.nation() == player.nation );
  maybe<UnitOwnership::world const&> world =
      state.ownership.get_if<UnitOwnership::world>();
  REQUIRE( world.has_value() );
  REQUIRE( world->coord == Coord{ .x = 1, .y = 2 } );
}

TEST_CASE(
    "[fathers] on_father_received: bartolome_de_las_casas" ) {
  World                   W;
  Player&                 player = W.default_player();
  e_founding_father const father =
      e_founding_father::bartolome_de_las_casas;

  W.add_unit_on_map( e_unit_type::native_convert,
                     { .x = 1, .y = 1 }, e_nation::dutch );
  W.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 1, .y = 1 }, e_nation::dutch );
  W.add_unit_on_map( e_unit_type::native_convert,
                     { .x = 1, .y = 1 }, e_nation::dutch );
  W.add_unit_on_map( e_unit_type::petty_criminal,
                     { .x = 1, .y = 1 }, e_nation::dutch );
  W.add_unit_on_map( e_unit_type::native_convert,
                     { .x = 1, .y = 0 }, e_nation::french );
  W.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 1, .y = 0 }, e_nation::french );
  W.add_unit_on_map( e_unit_type::native_convert,
                     { .x = 1, .y = 0 }, e_nation::french );

  REQUIRE( W.units().all().size() == 7 );
  REQUIRE( W.units().unit_for( UnitId{ 1 } ).type() ==
           e_unit_type::native_convert );
  REQUIRE( W.units().unit_for( UnitId{ 2 } ).type() ==
           e_unit_type::free_colonist );
  REQUIRE( W.units().unit_for( UnitId{ 3 } ).type() ==
           e_unit_type::native_convert );
  REQUIRE( W.units().unit_for( UnitId{ 4 } ).type() ==
           e_unit_type::petty_criminal );
  REQUIRE( W.units().unit_for( UnitId{ 5 } ).type() ==
           e_unit_type::native_convert );
  REQUIRE( W.units().unit_for( UnitId{ 6 } ).type() ==
           e_unit_type::free_colonist );
  REQUIRE( W.units().unit_for( UnitId{ 7 } ).type() ==
           e_unit_type::native_convert );

  player.fathers.has[father] = true;
  on_father_received( W.ss(), W.ts(), player, father );

  REQUIRE( W.units().all().size() == 7 );
  REQUIRE( W.units().unit_for( UnitId{ 1 } ).type() ==
           e_unit_type::free_colonist );
  REQUIRE( W.units().unit_for( UnitId{ 2 } ).type() ==
           e_unit_type::free_colonist );
  REQUIRE( W.units().unit_for( UnitId{ 3 } ).type() ==
           e_unit_type::free_colonist );
  REQUIRE( W.units().unit_for( UnitId{ 4 } ).type() ==
           e_unit_type::petty_criminal );
  REQUIRE( W.units().unit_for( UnitId{ 5 } ).type() ==
           e_unit_type::native_convert );
  REQUIRE( W.units().unit_for( UnitId{ 6 } ).type() ==
           e_unit_type::free_colonist );
  REQUIRE( W.units().unit_for( UnitId{ 7 } ).type() ==
           e_unit_type::native_convert );
}

TEST_CASE( "[fathers] on_father_received: jakob_fugger" ) {
  World                   W;
  Player&                 player = W.default_player();
  e_founding_father const father =
      e_founding_father::jakob_fugger;
  player.old_world.market.commodities[e_commodity::food]
      .boycott = true;
  player.old_world.market.commodities[e_commodity::fur].boycott =
      true;
  player.old_world.market.commodities[e_commodity::muskets]
      .boycott = true;
  W.french()
      .old_world.market.commodities[e_commodity::food]
      .boycott               = true;
  player.fathers.has[father] = true;
  on_father_received( W.ss(), W.ts(), player, father );
  for( auto& [comm, market_item] :
       player.old_world.market.commodities ) {
    INFO( fmt::format( "comm={}, market_item={}", comm,
                       market_item ) );
    REQUIRE_FALSE( market_item.boycott );
  }
  REQUIRE( W.french()
               .old_world.market.commodities[e_commodity::food]
               .boycott );
}

TEST_CASE(
    "[fathers] on_father_received: francisco_de_coronado" ) {
  World                   W;
  Player&                 dutch = W.dutch();
  e_founding_father const father =
      e_founding_father::francisco_de_coronado;
  W.create_large_map();
  Coord const kDutchColony{ .x = 6, .y = 6 };
  Coord const kFrenchColony{ .x = 18, .y = 18 };
  W.add_colony_with_new_unit( kDutchColony, e_nation::dutch );
  W.add_colony_with_new_unit( kFrenchColony, e_nation::french );

  // First make sure that the dutch player can only see the
  // squares around their colony.
  Rect expected_visible{ .x = 5, .y = 5, .w = 3, .h = 3 };
  UNWRAP_CHECK( dutch_map, W.terrain()
                               .player_terrain( e_nation::dutch )
                               .member( &PlayerTerrain::map ) );
  for( Rect r :
       gfx::subrects( W.terrain().world_rect_tiles() ) ) {
    INFO( fmt::format( "r={}", r ) );
    if( r.upper_left().is_inside( expected_visible ) ) {
      REQUIRE( dutch_map[r.upper_left()].has_value() );
    } else {
      REQUIRE_FALSE( dutch_map[r.upper_left()].has_value() );
    }
  }

  dutch.fathers.has[father] = true;
  on_father_received( W.ss(), W.ts(), dutch, father );

  Rect const expected_visible_1 =
      Rect{ .x = 1, .y = 1, .w = 11, .h = 11 };
  Rect const expected_visible_2 =
      Rect{ .x = 13, .y = 13, .w = 11, .h = 11 };
  for( Rect r :
       gfx::subrects( W.terrain().world_rect_tiles() ) ) {
    INFO( fmt::format( "r={}", r ) );
    if( r.upper_left().is_inside( expected_visible_1 ) ||
        r.upper_left().is_inside( expected_visible_2 ) ) {
      REQUIRE( dutch_map[r.upper_left()].has_value() );
    } else {
      REQUIRE_FALSE( dutch_map[r.upper_left()].has_value() );
    }
  }
}

TEST_CASE( "[fathers] on_father_received: sieur_de_la_salle" ) {
  World                   W;
  Player&                 dutch = W.dutch();
  e_founding_father const father =
      e_founding_father::sieur_de_la_salle;
  Coord const kDutchColony1{ .x = 0, .y = 0 };
  Coord const kDutchColony2{ .x = 2, .y = 0 };
  Coord const kFrenchColony{ .x = 0, .y = 2 };
  Colony&     dutch_colony1 = W.add_colony_with_new_unit(
      kDutchColony1, e_nation::dutch );
  Colony& dutch_colony2 = W.add_colony_with_new_unit(
      kDutchColony2, e_nation::dutch );
  Colony& french_colony = W.add_colony_with_new_unit(
      kFrenchColony, e_nation::french );
  W.add_unit_indoors( dutch_colony1.id, e_indoor_job::bells );
  W.add_unit_indoors( dutch_colony2.id, e_indoor_job::bells );
  W.add_unit_indoors( dutch_colony2.id, e_indoor_job::bells );
  W.add_unit_indoors( french_colony.id, e_indoor_job::bells );
  W.add_unit_indoors( french_colony.id, e_indoor_job::bells );
  W.add_unit_indoors( french_colony.id, e_indoor_job::bells );
  W.add_unit_indoors( french_colony.id, e_indoor_job::hammers );

  REQUIRE_FALSE(
      dutch_colony1.buildings[e_colony_building::stockade] );
  REQUIRE_FALSE(
      dutch_colony2.buildings[e_colony_building::stockade] );
  REQUIRE_FALSE(
      french_colony.buildings[e_colony_building::stockade] );

  dutch.fathers.has[father] = true;
  on_father_received( W.ss(), W.ts(), dutch, father );

  REQUIRE_FALSE(
      dutch_colony1.buildings[e_colony_building::stockade] );
  REQUIRE(
      dutch_colony2.buildings[e_colony_building::stockade] );
  REQUIRE_FALSE(
      french_colony.buildings[e_colony_building::stockade] );
}

TEST_CASE( "[fathers] on_father_received: pocahontas" ) {
  World              W;
  Player&            player = W.default_player();
  Tribe&             inca   = W.add_tribe( e_tribe::inca );
  Tribe&             arawak = W.add_tribe( e_tribe::arawak );
  Tribe&             tupi   = W.add_tribe( e_tribe::tupi );
  Tribe&             aztec  = W.add_tribe( e_tribe::aztec );
  TribeRelationship& inca_relationship =
      inca.relationship[player.nation];
  inca_relationship.encountered = true;
  TribeRelationship& arawak_relationship =
      arawak.relationship[player.nation];
  arawak_relationship.encountered = true;
  TribeRelationship& tupi_relationship =
      tupi.relationship[player.nation];
  tupi_relationship.encountered = true;
  auto& aztec_relationship = aztec.relationship[player.nation];

  auto f = [&] {
    on_father_received( W.ss(), W.ts(), player,
                        e_founding_father::pocahontas );
  };

  inca_relationship.tribal_alarm   = 1;
  arawak_relationship.tribal_alarm = 60;
  tupi_relationship.tribal_alarm   = 30;

  f();
  REQUIRE( inca_relationship.tribal_alarm == 1 );
  REQUIRE( arawak_relationship.tribal_alarm == 17 );
  REQUIRE( tupi_relationship.tribal_alarm == 17 );
  REQUIRE( !aztec_relationship.encountered );

  f();
  REQUIRE( inca_relationship.tribal_alarm == 1 );
  REQUIRE( arawak_relationship.tribal_alarm == 17 );
  REQUIRE( tupi_relationship.tribal_alarm == 17 );
  REQUIRE( !aztec_relationship.encountered );
}

TEST_CASE( "[fathers] on_father_received: william_brewster" ) {
  World W;
  W.settings().difficulty  = e_difficulty::conquistador;
  Player&           player = W.default_player();
  ImmigrationState& state  = player.old_world.immigration;
  player.fathers.has[e_founding_father::william_brewster] = true;

  auto f = [&] {
    on_father_received( W.ss(), W.ts(), player,
                        e_founding_father::william_brewster );
  };

  SECTION( "no change" ) {
    state.immigrants_pool[0] = e_unit_type::free_colonist;
    state.immigrants_pool[1] = e_unit_type::expert_fisherman;
    state.immigrants_pool[2] = e_unit_type::soldier;
    f();
    REQUIRE( state.immigrants_pool[0] ==
             e_unit_type::free_colonist );
    REQUIRE( state.immigrants_pool[1] ==
             e_unit_type::expert_fisherman );
    REQUIRE( state.immigrants_pool[2] == e_unit_type::soldier );
  }

  SECTION( "changes" ) {
    state.immigrants_pool[0] = e_unit_type::expert_farmer;
    state.immigrants_pool[1] = e_unit_type::petty_criminal;
    state.immigrants_pool[2] = e_unit_type::indentured_servant;
    // Note that the upper limit of the random double is tested
    // elsewhere.
    W.rand().EXPECT__between_doubles( 0, _ ).returns( 0.0 );
    W.rand().EXPECT__between_doubles( 0, _ ).returns( 2500.0 );
    f();
    REQUIRE( state.immigrants_pool[0] ==
             e_unit_type::expert_farmer );
    REQUIRE( state.immigrants_pool[1] ==
             e_unit_type::free_colonist );
    REQUIRE( state.immigrants_pool[2] == e_unit_type::pioneer );
  }
}

} // namespace
} // namespace rn
