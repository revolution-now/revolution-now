/****************************************************************
**meet-natives.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-09.
*
* Description: Unit tests for the src/meet-natives.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/meet-natives.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/ieuro-mind.hpp"
#include "test/mocks/igui.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/native-owned.hpp"

// ss
#include "ss/natives.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/unit-composition.hpp"
#include "ss/woodcut.rds.hpp"

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
    add_player( e_nation::english );
    add_player( e_nation::french );
    add_player( e_nation::spanish );
    add_player( e_nation::dutch );
    set_default_player( e_nation::english );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _, L, L, L,
      L, L, L, L, L, L,
      _, L, L, L, L, L,
      _, L, L, L, L, L,
      _, L, L, L, L, L,
      _, L, L, L, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 6 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[meet-natives] check_meet_tribes" ) {
  World             W;
  Coord             square;
  vector<MeetTribe> expected;

  auto f = [&] {
    return check_meet_tribes( W.ss(), W.default_player(),
                              square );
  };

  SECTION( "nothing" ) {
    square   = { .x = 0, .y = 1 };
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "one dwelling too far" ) {
    W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::inca );
    square = { .x = 1, .y = 3 };
    CHECK( W.terrain().square_at( square ).surface ==
           e_surface::land );
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "one dwelling meet" ) {
    W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::inca );
    W.add_dwelling( { .x = 4, .y = 4 }, e_tribe::inca );
    square   = { .x = 0, .y = 1 };
    expected = {
        MeetTribe{
            .nation        = e_nation::english,
            .tribe         = e_tribe::inca,
            .num_dwellings = 2,
            .land_awarded  = {},
        },
    };
    REQUIRE( f() == expected );
  }

  SECTION( "one dwelling already encountered" ) {
    W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::inca );
    W.add_dwelling( { .x = 4, .y = 4 }, e_tribe::inca );
    square      = { .x = 0, .y = 1 };
    Tribe& inca = W.natives().tribe_for( e_tribe::inca );
    inca.relationship[W.default_nation()].encountered = true;
    expected                                          = {};
    REQUIRE( f() == expected );
  }

  SECTION( "one dwelling from water no meet" ) {
    W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::inca );
    W.add_dwelling( { .x = 4, .y = 4 }, e_tribe::inca );
    square   = { .x = 0, .y = 2 };
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "two dwellings meet" ) {
    W.add_dwelling( { .x = 1, .y = 0 }, e_tribe::aztec );
    W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::inca );
    W.add_dwelling( { .x = 4, .y = 4 }, e_tribe::inca );
    square   = { .x = 0, .y = 1 };
    expected = {
        MeetTribe{
            .nation        = e_nation::english,
            .tribe         = e_tribe::aztec,
            .num_dwellings = 1,
            .land_awarded  = {},
        },
        MeetTribe{
            .nation        = e_nation::english,
            .tribe         = e_tribe::inca,
            .num_dwellings = 2,
            .land_awarded  = {},
        },
    };
    REQUIRE( f() == expected );
  }

  SECTION( "two same tribe meet once" ) {
    W.add_dwelling( { .x = 1, .y = 0 }, e_tribe::inca );
    W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::inca );
    W.add_dwelling( { .x = 4, .y = 4 }, e_tribe::inca );
    square   = { .x = 0, .y = 1 };
    expected = {
        MeetTribe{
            .nation        = e_nation::english,
            .tribe         = e_tribe::inca,
            .num_dwellings = 3,
            .land_awarded  = {},
        },
    };
    REQUIRE( f() == expected );
  }

  SECTION( "land owned no colonies" ) {
    Dwelling const& dwelling =
        W.add_dwelling( { .x = 2, .y = 2 }, e_tribe::inca );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 1, .y = 1 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 2, .y = 1 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 3, .y = 1 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 1, .y = 2 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 2, .y = 2 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 3, .y = 2 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 1, .y = 3 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 2, .y = 3 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 3, .y = 3 } );
    square   = { .x = 3, .y = 3 };
    expected = {
        MeetTribe{
            .nation        = e_nation::english,
            .tribe         = e_tribe::inca,
            .num_dwellings = 1,
            .land_awarded  = {},
        },
    };
    REQUIRE( f() == expected );
  }

  SECTION( "land owned with colonies" ) {
    Dwelling const& dwelling =
        W.add_dwelling( { .x = 2, .y = 2 }, e_tribe::inca );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 1, .y = 1 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 2, .y = 1 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 3, .y = 1 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 1, .y = 2 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 2, .y = 2 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 3, .y = 2 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 1, .y = 3 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 2, .y = 3 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 3, .y = 3 } );
    Colony& colony = W.add_colony( { .x = 1, .y = 3 } );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::food );
    square   = { .x = 3, .y = 3 };
    expected = {
        MeetTribe{ .nation        = e_nation::english,
                   .tribe         = e_tribe::inca,
                   .num_dwellings = 1,
                   .land_awarded  = { { .x = 1, .y = 3 },
                                      { .x = 2, .y = 3 } } },
    };
    REQUIRE( f() == expected );
  }

  SECTION( "land owned with colonies and minuit" ) {
    W.default_player()
        .fathers.has[e_founding_father::peter_minuit] = true;
    Dwelling const& dwelling =
        W.add_dwelling( { .x = 2, .y = 2 }, e_tribe::inca );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 1, .y = 1 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 2, .y = 1 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 3, .y = 1 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 1, .y = 2 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 2, .y = 2 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 3, .y = 2 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 1, .y = 3 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 2, .y = 3 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 3, .y = 3 } );
    Colony& colony = W.add_colony( { .x = 1, .y = 3 } );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::food );
    square   = { .x = 3, .y = 3 };
    expected = {
        MeetTribe{ .nation        = e_nation::english,
                   .tribe         = e_tribe::inca,
                   .num_dwellings = 1,
                   .land_awarded  = {} },
    };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[meet-natives] check_meet_europeans" ) {
  World             W;
  vector<MeetTribe> expected;
  // S . b d . .
  // D E F . . .

  auto f = [&]( Coord coord ) {
    return check_meet_europeans( W.ss(), e_tribe::apache,
                                 coord );
  };

  W.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 1, .y = 1 }, e_nation::english );
  W.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 2, .y = 1 }, e_nation::french );
  W.add_unit_on_map( e_unit_type::caravel, { .x = 0, .y = 0 },
                     e_nation::spanish );
  W.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 1 }, e_nation::dutch );

  DwellingId const apache_dwelling_id =
      W.add_dwelling( { .x = 3, .y = 0 }, e_tribe::apache ).id;
  W.add_native_unit_on_map( e_native_unit_type::brave,
                            { .x = 2, .y = 0 },
                            apache_dwelling_id );

  W.apache().relationship[e_nation::dutch].encountered = true;
  W.add_tribe( e_tribe::cherokee );
  W.cherokee().relationship[e_nation::english].encountered =
      true;

  // Set up english colonists on apache owned land.
  Colony const& colony =
      W.add_colony( { .x = 1, .y = 2 }, e_nation::english );
  W.add_unit_outdoors( colony.id, e_direction::n,
                       e_outdoor_job::food );
  W.natives().mark_land_owned( apache_dwelling_id,
                               { .x = 1, .y = 1 } );

  expected = {
      MeetTribe{ .nation        = e_nation::english,
                 .tribe         = e_tribe::apache,
                 .num_dwellings = 1,
                 .land_awarded  = { { .x = 1, .y = 1 } } },
      MeetTribe{ .nation        = e_nation::french,
                 .tribe         = e_tribe::apache,
                 .num_dwellings = 1 } };
  REQUIRE( f( { .x = 1, .y = 0 } ) == expected );
}

TEST_CASE( "[meet-natives] perform_meet_tribe" ) {
  World                    W;
  Player const&            player = W.default_player();
  MeetTribe                meet_tribe;
  e_declare_war_on_natives declare_war = {};
  e_tribe const            tribe       = e_tribe::cherokee;
  e_nation const           nation      = W.default_nation();

  auto f = [&] {
    perform_meet_tribe( W.ss(), W.default_player(), meet_tribe,
                        declare_war );
  };

  Dwelling const& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, tribe );

  REQUIRE( !W.natives()
                .tribe_for( tribe )
                .relationship[nation]
                .encountered );

  meet_tribe = { .nation        = e_nation::dutch,
                 .tribe         = tribe,
                 .num_dwellings = 3,
                 .land_awarded  = { { .x = 1, .y = 0 },
                                    { .x = 2, .y = 0 } } };
  W.natives().mark_land_owned( dwelling.id, { .x = 1, .y = 0 } );
  W.natives().mark_land_owned( dwelling.id, { .x = 2, .y = 0 } );
  W.natives().mark_land_owned( dwelling.id, { .x = 2, .y = 1 } );

  SECTION( "no declare war" ) {
    declare_war = e_declare_war_on_natives::no;
    f();
    REQUIRE(
        W.natives().tribe_for( tribe ).relationship[nation] ==
        TribeRelationship{ .encountered  = true,
                           .at_war       = false,
                           .tribal_alarm = 10 } );
    REQUIRE( !is_land_native_owned( W.ss(), player,
                                    { .x = 1, .y = 0 } ) );
    REQUIRE( !is_land_native_owned( W.ss(), player,
                                    { .x = 2, .y = 0 } ) );
    REQUIRE( is_land_native_owned( W.ss(), player,
                                   { .x = 2, .y = 1 } ) );
    REQUIRE( is_land_native_owned( W.ss(), player,
                                   { .x = 2, .y = 1 } ) );
  }

  SECTION( "declare war" ) {
    declare_war = e_declare_war_on_natives::yes;
    f();
    REQUIRE(
        W.natives().tribe_for( tribe ).relationship[nation] ==
        TribeRelationship{ .encountered  = true,
                           .at_war       = true,
                           .tribal_alarm = 10 } );
  }

  // The perform_meet_tribe function may receive some owned land
  // squares occupied by colonies, since those are among the ones
  // given to the player upon meeting. This test makes sure that
  // it doesn't check fail in that case.
  SECTION( "with colony" ) {
    declare_war = e_declare_war_on_natives::no;
    W.add_colony( { .x = 1, .y = 0 } );
    f();
    REQUIRE(
        W.natives().tribe_for( tribe ).relationship[nation] ==
        TribeRelationship{ .encountered  = true,
                           .at_war       = false,
                           .tribal_alarm = 10 } );
    REQUIRE( !is_land_native_owned( W.ss(), player,
                                    { .x = 1, .y = 0 } ) );
    REQUIRE( !is_land_native_owned( W.ss(), player,
                                    { .x = 2, .y = 0 } ) );
    REQUIRE( is_land_native_owned( W.ss(), player,
                                   { .x = 2, .y = 1 } ) );
    REQUIRE( is_land_native_owned( W.ss(), player,
                                   { .x = 2, .y = 1 } ) );
  }
}

// This one tests that when the relationship object is created
// the tribal alarm is initialized with the minimum value.
TEST_CASE( "[meet-natives] perform_meet_tribe arawak" ) {
  World                    W;
  MeetTribe                meet_tribe;
  e_declare_war_on_natives declare_war = {};
  e_tribe const            tribe       = e_tribe::arawak;
  Tribe const&             tribe_obj   = W.add_tribe( tribe );
  e_nation const           nation      = W.default_nation();

  auto f = [&] {
    perform_meet_tribe( W.ss(), W.default_player(), meet_tribe,
                        declare_war );
  };

  meet_tribe = { .nation        = e_nation::dutch,
                 .tribe         = tribe,
                 .num_dwellings = 3 };
  f();
  REQUIRE( tribe_obj.relationship[nation] ==
           TribeRelationship{ .encountered  = true,
                              .at_war       = false,
                              .tribal_alarm = 30 } );
}

TEST_CASE( "[meet-natives] perform_meet_tribe_ui_sequence" ) {
  World                    w;
  e_declare_war_on_natives expected = {};

  MeetTribe meet_tribe;

  auto f = [&] {
    return co_await_test( perform_meet_tribe_ui_sequence(
        w.ss(), w.euro_mind(), w.gui(), meet_tribe ) );
  };

  meet_tribe = {
      .nation        = w.default_nation(),
      .tribe         = e_tribe::tupi,
      .num_dwellings = 3,
      .land_awarded = { { .x = 1, .y = 1 }, { .x = 2, .y = 1 } },
  };

  SECTION( "tupi" ) {
    meet_tribe.tribe = e_tribe::tupi;
    w.euro_mind().EXPECT__show_woodcut(
        e_woodcut::meeting_the_natives );

    ChoiceConfig const choice_config{
        .msg =
            "The [Tupi] tribe is a celebrated nation of [3 "
            "camps].  In honor of our glorious future together "
            "we will generously give you all of the land that "
            "your colonies now occupy. Will you accept our "
            "peace treaty and agree to live in harmony with us?",
        .options = { { .key = "yes", .display_name = "Yes" },
                     { .key = "no", .display_name = "No" } },
        .sort    = false,
        .initial_selection = nothing };
    w.gui()
        .EXPECT__choice( choice_config )
        .returns<maybe<string>>( "yes" );
    w.gui().EXPECT__message_box(
        "Let us smoke a peace pipe to celebrate our purpetual "
        "friendship with the [English]." );
    w.gui().EXPECT__message_box(
        "We hope that you will send us your colonists and "
        "[Wagon Trains] to share knowledge and to trade." );
    expected = e_declare_war_on_natives::no;
    REQUIRE( f() == expected );
  }

  SECTION( "aztec" ) {
    meet_tribe.tribe = e_tribe::aztec;

    w.euro_mind().EXPECT__show_woodcut(
        e_woodcut::meeting_the_natives );
    w.euro_mind().EXPECT__show_woodcut(
        e_woodcut::meeting_the_aztec_empire );

    ChoiceConfig const choice_config{
        .msg =
            "The [Aztec] tribe is a celebrated nation of [3 "
            "cities].  In honor of our glorious future together "
            "we will generously give you all of the land that "
            "your colonies now occupy. Will you accept our "
            "peace treaty and agree to live in harmony with us?",
        .options = { { .key = "yes", .display_name = "Yes" },
                     { .key = "no", .display_name = "No" } },
        .sort    = false,
        .initial_selection = nothing };
    w.gui()
        .EXPECT__choice( choice_config )
        .returns<maybe<string>>( "yes" );
    w.gui().EXPECT__message_box(
        "Let us smoke a peace pipe to celebrate our purpetual "
        "friendship with the [English]." );
    w.gui().EXPECT__message_box(
        "We hope that you will send us your colonists and "
        "[Wagon Trains] to share knowledge and to trade." );
    expected = e_declare_war_on_natives::no;
    REQUIRE( f() == expected );
  }

  SECTION( "inca, post-declaration" ) {
    w.default_player().revolution_status =
        e_revolution_status::declared;
    meet_tribe.tribe = e_tribe::inca;

    w.euro_mind().EXPECT__show_woodcut(
        e_woodcut::meeting_the_natives );
    w.euro_mind().EXPECT__show_woodcut(
        e_woodcut::meeting_the_inca_nation );

    ChoiceConfig const choice_config{
        .msg =
            "The [Inca] tribe is a celebrated nation of [3 "
            "cities].  In honor of our glorious future together "
            "we will generously give you all of the land that "
            "your colonies now occupy. Will you accept our "
            "peace treaty and agree to live in harmony with us?",
        .options = { { .key = "yes", .display_name = "Yes" },
                     { .key = "no", .display_name = "No" } },
        .sort    = false,
        .initial_selection = nothing };
    w.gui()
        .EXPECT__choice( choice_config )
        .returns<maybe<string>>( "yes" );
    w.gui().EXPECT__message_box(
        "Let us smoke a peace pipe to celebrate our purpetual "
        "friendship with the [Rebels]." );
    w.gui().EXPECT__message_box(
        "We hope that you will send us your colonists and "
        "[Wagon Trains] to share knowledge and to trade." );
    expected = e_declare_war_on_natives::no;
    REQUIRE( f() == expected );
  }

  SECTION( "tupi, already met natives, declare war" ) {
    meet_tribe.tribe = e_tribe::tupi;

    w.default_player().woodcuts[e_woodcut::meeting_the_natives] =
        true;

    ChoiceConfig const choice_config{
        .msg =
            "The [Tupi] tribe is a celebrated nation of [3 "
            "camps].  In honor of our glorious future together "
            "we will generously give you all of the land that "
            "your colonies now occupy. Will you accept our "
            "peace treaty and agree to live in harmony with us?",
        .options = { { .key = "yes", .display_name = "Yes" },
                     { .key = "no", .display_name = "No" } },
        .sort    = false,
        .initial_selection = nothing };
    w.gui()
        .EXPECT__choice( choice_config )
        .returns<maybe<string>>( "no" );
    w.gui().EXPECT__message_box(
        "In that case the mighty [Tupi] will drive you into "
        "oblivion. Prepare for WAR!" );
    expected = e_declare_war_on_natives::yes;
    REQUIRE( f() == expected );
  }
}

} // namespace
} // namespace rn
