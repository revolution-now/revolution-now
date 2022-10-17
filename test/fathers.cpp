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

// ss
#include "ss/player.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/turn.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    create_default_map();
    add_default_player();
  }

  void create_default_map() {
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{ L, L, L };
    build_map( std::move( tiles ), 3 );
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
    EXPECT_CALL(
        W.rand(),
        between_ints( low, high, IRand::e_interval::half_open ) )
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
    EXPECT_CALL( W.gui(),
                 choice( config, e_input_required::yes ) )
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
    EXPECT_CALL( W.gui(),
                 choice( config, e_input_required::yes ) )
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
    EXPECT_CALL( W.gui(),
                 choice( config, e_input_required::yes ) )
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
    EXPECT_CALL( W.gui(),
                 choice( config, e_input_required::yes ) )
        .returns( make_wait<maybe<string>>( "adam_smith" ) );
    fathers_copy = player.fathers;
    f();
    fathers_copy.pool[e_founding_father_type::trade] =
        e_founding_father::adam_smith;
    fathers_copy.in_progress = e_founding_father::adam_smith;
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
  // TODO
}

} // namespace
} // namespace rn
