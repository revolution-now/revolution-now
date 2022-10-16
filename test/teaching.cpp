/****************************************************************
**teaching.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-19.
*
* Description: Unit tests for the src/teaching.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/teaching.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/irand.hpp"

// Revolution Now
#include "src/luapp/state.hpp" // FIXME: remove if not needed.

// refl
#include "refl/to-str.hpp"

// base
#include "to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::base::valid;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_default_player();
    create_default_map();
  }

  inline static Coord const kColonySquare =
      Coord{ .x = 1, .y = 1 };

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _,
      L, L, L,
      _, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[teaching] max_teachers_allowed" ) {
  World   W;
  Colony& colony = W.add_colony( World::kColonySquare );

  REQUIRE( max_teachers_allowed( colony ) == 0 );

  colony.buildings[e_colony_building::schoolhouse] = true;
  REQUIRE( max_teachers_allowed( colony ) == 1 );

  colony.buildings[e_colony_building::college] = true;
  REQUIRE( max_teachers_allowed( colony ) == 2 );

  colony.buildings[e_colony_building::university] = true;
  REQUIRE( max_teachers_allowed( colony ) == 3 );
}

TEST_CASE( "[teaching] sync_colony_teachers" ) {
  World   W;
  Colony& colony = W.add_colony( World::kColonySquare );
  colony.buildings[e_colony_building::university] = true;

  colony.teachers = { { 5, 3 }, { 6, 2 } };
  unordered_map<UnitId, int> expected;

  SECTION( "no teachers" ) {
    sync_colony_teachers( colony );
    expected = {};
    REQUIRE( colony.teachers == expected );
  }

  SECTION( "one new teacher" ) {
    colony.indoor_jobs[e_indoor_job::teacher] = { 1 };
    sync_colony_teachers( colony );
    expected = { { 1, 0 } };
    REQUIRE( colony.teachers == expected );
  }

  SECTION( "one existing teacher" ) {
    colony.indoor_jobs[e_indoor_job::teacher] = { 6 };
    sync_colony_teachers( colony );
    expected = { { 6, 2 } };
    REQUIRE( colony.teachers == expected );
  }

  SECTION( "mixed" ) {
    colony.indoor_jobs[e_indoor_job::teacher] = { 6, 4, 3 };
    sync_colony_teachers( colony );
    expected = { { 6, 2 }, { 4, 0 }, { 3, 0 } };
    REQUIRE( colony.teachers == expected );
  }
}

TEST_CASE( "[teaching] can_unit_teach" ) {
  e_school_type school_type = {};

  auto f = [&]( e_unit_type unit_type ) {
    return can_unit_teach_in_building( unit_type, school_type );
  };

  SECTION( "in a schoolhouse" ) {
    school_type = e_school_type::schoolhouse;

    // Expert types.
    REQUIRE( f( e_unit_type::expert_sugar_planter ) != valid );
    REQUIRE( f( e_unit_type::master_weaver ) != valid );
    REQUIRE( f( e_unit_type::expert_farmer ) == valid );
    REQUIRE( f( e_unit_type::elder_statesman ) != valid );
    REQUIRE( f( e_unit_type::master_carpenter ) == valid );
    REQUIRE( f( e_unit_type::hardy_colonist ) == valid );
    REQUIRE( f( e_unit_type::jesuit_colonist ) != valid );

    // Non-expert types.
    REQUIRE( f( e_unit_type::free_colonist ) != valid );
    REQUIRE( f( e_unit_type::indentured_servant ) != valid );
    REQUIRE( f( e_unit_type::petty_criminal ) != valid );
    REQUIRE( f( e_unit_type::native_convert ) != valid );

    // Derived types.
    REQUIRE( f( e_unit_type::dragoon ) != valid );
    REQUIRE( f( e_unit_type::veteran_soldier ) != valid );
    REQUIRE( f( e_unit_type::jesuit_missionary ) != valid );
    REQUIRE( f( e_unit_type::hardy_pioneer ) == valid );
  }

  SECTION( "in college" ) {
    school_type = e_school_type::college;

    // Expert types.
    REQUIRE( f( e_unit_type::expert_sugar_planter ) == valid );
    REQUIRE( f( e_unit_type::master_weaver ) == valid );
    REQUIRE( f( e_unit_type::expert_farmer ) == valid );
    REQUIRE( f( e_unit_type::elder_statesman ) != valid );
    REQUIRE( f( e_unit_type::master_carpenter ) == valid );
    REQUIRE( f( e_unit_type::hardy_colonist ) == valid );
    REQUIRE( f( e_unit_type::jesuit_colonist ) != valid );

    // Non-expert types.
    REQUIRE( f( e_unit_type::free_colonist ) != valid );
    REQUIRE( f( e_unit_type::indentured_servant ) != valid );
    REQUIRE( f( e_unit_type::petty_criminal ) != valid );
    REQUIRE( f( e_unit_type::native_convert ) != valid );

    // Derived types.
    REQUIRE( f( e_unit_type::dragoon ) != valid );
    REQUIRE( f( e_unit_type::veteran_soldier ) == valid );
    REQUIRE( f( e_unit_type::jesuit_missionary ) != valid );
    REQUIRE( f( e_unit_type::hardy_pioneer ) == valid );
  }

  SECTION( "in a university" ) {
    school_type = e_school_type::university;

    // Expert types.
    REQUIRE( f( e_unit_type::expert_sugar_planter ) == valid );
    REQUIRE( f( e_unit_type::master_weaver ) == valid );
    REQUIRE( f( e_unit_type::expert_farmer ) == valid );
    REQUIRE( f( e_unit_type::elder_statesman ) == valid );
    REQUIRE( f( e_unit_type::master_carpenter ) == valid );
    REQUIRE( f( e_unit_type::hardy_colonist ) == valid );
    REQUIRE( f( e_unit_type::jesuit_colonist ) == valid );

    // Non-expert types.
    REQUIRE( f( e_unit_type::free_colonist ) != valid );
    REQUIRE( f( e_unit_type::indentured_servant ) != valid );
    REQUIRE( f( e_unit_type::petty_criminal ) != valid );
    REQUIRE( f( e_unit_type::native_convert ) != valid );

    // Derived types.
    REQUIRE( f( e_unit_type::dragoon ) != valid );
    REQUIRE( f( e_unit_type::veteran_soldier ) == valid );
    REQUIRE( f( e_unit_type::jesuit_missionary ) == valid );
    REQUIRE( f( e_unit_type::hardy_pioneer ) == valid );
  }
}

TEST_CASE( "[teaching] evolve_teachers" ) {
  World   W;
  Colony& colony = W.add_colony( World::kColonySquare );
  colony.buildings[e_colony_building::university] = true;

  ColonyTeachingEvolution expected;
  // "teachers map".
  using TM = unordered_map<UnitId, int>;

  auto f = [&] {
    return evolve_teachers( W.ss(), W.ts(), W.default_player(),
                            colony );
  };

  SECTION( "teachers=0, teachable=0" ) {
    expected = {};
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{} );
  }

  SECTION( "teachers=0, teachable=1" ) {
    W.add_unit_outdoors( colony.id, e_direction::nw,
                         e_outdoor_job::fish,
                         e_unit_type::free_colonist );
    expected = {};
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{} );
  }

  SECTION( "teachers=1, teachable=0" ) {
    UnitId teacher1 =
        W.add_unit_indoors( colony.id, e_indoor_job::teacher,
                            e_unit_type::master_carpenter );
    expected = {
        .teachers = {
            { .teacher_unit_id = teacher1,
              .action = TeacherAction::in_progress{} } } };
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 1 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 2 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 3 } } );
    expected = {
        .teachers = {
            { .teacher_unit_id = teacher1,
              .action = TeacherAction::taught_no_one{} } } };
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 0 } } );
    // Another round.
    expected = {
        .teachers = {
            { .teacher_unit_id = teacher1,
              .action = TeacherAction::in_progress{} } } };
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 1 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 2 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 3 } } );
    expected = {
        .teachers = {
            { .teacher_unit_id = teacher1,
              .action = TeacherAction::taught_no_one{} } } };
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 0 } } );
  }

  SECTION( "teachers=1, teachable=1" ) {
    W.add_unit_outdoors( colony.id, e_direction::nw,
                         e_outdoor_job::fish,
                         e_unit_type::petty_criminal );
    UnitId teacher1 =
        W.add_unit_indoors( colony.id, e_indoor_job::teacher,
                            e_unit_type::master_carpenter );
    // First round goes to indentured servant.
    expected = {
        .teachers = {
            { .teacher_unit_id = teacher1,
              .action = TeacherAction::in_progress{} } } };
    REQUIRE( colony.teachers == TM{ { teacher1, 0 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 1 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 2 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 3 } } );
    expected = {
        .teachers = {
            { .teacher_unit_id = teacher1,
              .action          = TeacherAction::taught_unit{
                           .taught_id = 1,
                           .from_type = e_unit_type::petty_criminal,
                           .to_type =
                      e_unit_type::indentured_servant } } } };
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 0 } } );
    // Second round goes to free colonist.
    expected = {
        .teachers = {
            { .teacher_unit_id = teacher1,
              .action = TeacherAction::in_progress{} } } };
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 1 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 2 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 3 } } );
    expected = {
        .teachers = {
            { .teacher_unit_id = teacher1,
              .action          = TeacherAction::taught_unit{
                           .taught_id = 1,
                           .from_type = e_unit_type::indentured_servant,
                           .to_type = e_unit_type::free_colonist } } } };
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 0 } } );
    // Third round goes to master carpenter.
    expected = {
        .teachers = {
            { .teacher_unit_id = teacher1,
              .action = TeacherAction::in_progress{} } } };
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 1 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 2 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 3 } } );
    expected = {
        .teachers = {
            { .teacher_unit_id = teacher1,
              .action          = TeacherAction::taught_unit{
                           .taught_id = 1,
                           .from_type = e_unit_type::free_colonist,
                           .to_type =
                      e_unit_type::master_carpenter } } } };
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 0 } } );
    // Last round does nothing.
    expected = {
        .teachers = {
            { .teacher_unit_id = teacher1,
              .action = TeacherAction::in_progress{} } } };
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 1 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 2 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 3 } } );
    expected = {
        .teachers = {
            { .teacher_unit_id = teacher1,
              .action = TeacherAction::taught_no_one{} } } };
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers == TM{ { teacher1, 0 } } );
  }

  SECTION( "teachers=2, teachable=4" ) {
    UnitId teachable1 = W.add_unit_outdoors(
        colony.id, e_direction::n, e_outdoor_job::food,
        e_unit_type::indentured_servant );
    UnitId teachable2 = W.add_unit_outdoors(
        colony.id, e_direction::ne, e_outdoor_job::lumber,
        e_unit_type::free_colonist );
    UnitId teachable3 = W.add_unit_outdoors(
        colony.id, e_direction::e, e_outdoor_job::lumber,
        e_unit_type::free_colonist );
    // This one will take 4 turns.
    UnitId teacher1 =
        W.add_unit_indoors( colony.id, e_indoor_job::teacher,
                            e_unit_type::master_carpenter );
    // This one will take 8 turns.
    UnitId teacher2 =
        W.add_unit_indoors( colony.id, e_indoor_job::teacher,
                            e_unit_type::elder_statesman );
    // First round goes to indentured servant.
    expected = { .teachers = {
                     { .teacher_unit_id = teacher1,
                       .action = TeacherAction::in_progress{} },
                     { .teacher_unit_id = teacher2,
                       .action = TeacherAction::in_progress{} },
                 } };
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers ==
             TM{ { teacher1, 1 }, { teacher2, 1 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers ==
             TM{ { teacher1, 2 }, { teacher2, 2 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers ==
             TM{ { teacher1, 3 }, { teacher2, 3 } } );
    expected = {
        .teachers = {
            { .teacher_unit_id = teacher1,
              .action =
                  TeacherAction::taught_unit{
                      // Takes from the back.
                      .taught_id = teachable1,
                      .from_type =
                          e_unit_type::indentured_servant,
                      .to_type = e_unit_type::free_colonist } },
            { .teacher_unit_id = teacher2,
              .action          = TeacherAction::in_progress{} },
        } };
    // We want teachable2, teachable3, teachable1, and then it
    // takes from the back.
    expect_shuffle( W.rand(), { 1, 2, 0 } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers ==
             TM{ { teacher1, 0 }, { teacher2, 4 } } );
    // Next round will end up teaching the remaining two.
    expected = { .teachers = {
                     { .teacher_unit_id = teacher1,
                       .action = TeacherAction::in_progress{} },
                     { .teacher_unit_id = teacher2,
                       .action = TeacherAction::in_progress{} },
                 } };
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers ==
             TM{ { teacher1, 1 }, { teacher2, 5 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers ==
             TM{ { teacher1, 2 }, { teacher2, 6 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers ==
             TM{ { teacher1, 3 }, { teacher2, 7 } } );
    expected = {
        .teachers = {
            { .teacher_unit_id = teacher1,
              .action =
                  TeacherAction::taught_unit{
                      .taught_id = teachable2,
                      .from_type = e_unit_type::free_colonist,
                      .to_type =
                          e_unit_type::master_carpenter } },
            { .teacher_unit_id = teacher2,
              .action          = TeacherAction::taught_unit{
                           .taught_id = teachable1,
                           .from_type = e_unit_type::free_colonist,
                           .to_type =
                      e_unit_type::elder_statesman } } } };
    // We want teachable3, teachable1, teachable2, and then it
    // takes from the back.
    expect_shuffle( W.rand(), { 2, 0, 1 } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers ==
             TM{ { teacher1, 0 }, { teacher2, 0 } } );
    // Next round will teach the remaining colonist.
    expected = { .teachers = {
                     { .teacher_unit_id = teacher1,
                       .action = TeacherAction::in_progress{} },
                     { .teacher_unit_id = teacher2,
                       .action = TeacherAction::in_progress{} },
                 } };
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers ==
             TM{ { teacher1, 1 }, { teacher2, 1 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers ==
             TM{ { teacher1, 2 }, { teacher2, 2 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers ==
             TM{ { teacher1, 3 }, { teacher2, 3 } } );
    expected = {
        .teachers = {
            { .teacher_unit_id = teacher1,
              .action =
                  TeacherAction::taught_unit{
                      .taught_id = teachable3,
                      .from_type = e_unit_type::free_colonist,
                      .to_type =
                          e_unit_type::master_carpenter } },
            { .teacher_unit_id = teacher2,
              .action = TeacherAction::in_progress{} } } };
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers ==
             TM{ { teacher1, 0 }, { teacher2, 4 } } );
    // Last round will teach no one.
    expected = { .teachers = {
                     { .teacher_unit_id = teacher1,
                       .action = TeacherAction::in_progress{} },
                     { .teacher_unit_id = teacher2,
                       .action = TeacherAction::in_progress{} },
                 } };
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers ==
             TM{ { teacher1, 1 }, { teacher2, 5 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers ==
             TM{ { teacher1, 2 }, { teacher2, 6 } } );
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers ==
             TM{ { teacher1, 3 }, { teacher2, 7 } } );
    expected = {
        .teachers = {
            { .teacher_unit_id = teacher1,
              .action = TeacherAction::taught_no_one{} },
            { .teacher_unit_id = teacher2,
              .action = TeacherAction::taught_no_one{} } } };
    REQUIRE( f() == expected );
    REQUIRE( colony.teachers ==
             TM{ { teacher1, 0 }, { teacher2, 0 } } );
  }
}

} // namespace
} // namespace rn
