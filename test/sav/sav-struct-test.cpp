/****************************************************************
**sav-struct-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-03.
*
* Description: Unit tests for the sav/sav-struct module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/sav/sav-struct.hpp"

// cdr
#include "src/cdr/converter.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace sav {
namespace {

using namespace std;

/****************************************************************
** Static checks.
*****************************************************************/
static_assert( base::Show<ColonySAV> );

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[sav/sav-struct] construction" ) {
  ColonySAV sav;

  // Test a couple random fields.
  REQUIRE( sav.header.turn == 0 );
  REQUIRE( sav.header.game_options.autosave == false );
  REQUIRE( sav.header.dwelling_count == 0 );
  REQUIRE( sav.header.event.the_fountain_of_youth == false );
  REQUIRE( sav.tribe[1].tech == tech_type::semi_nomadic );
}

TEST_CASE( "[sav/sav-struct] to_str" ) {
  string expected;

  // Metadata (enum) type.
  {
    unit_type const o = unit_type::man_o_war;
    expected          = "Man-O-War";
    REQUIRE( base::to_str( o ) == expected );
  }

  // Bit struct type.
  {
    NationInfo const o{
        .nation_id      = nation_4bit_type::cherokee,
        .vis_to_english = true,
        .vis_to_french  = false,
        .vis_to_spanish = true,
        .vis_to_dutch   = false,
    };
    expected =
        "NationInfo{nation_id=Cherokee,vis_to_english=true,vis_"
        "to_french=false,vis_to_spanish=true,vis_to_dutch="
        "false}";
    REQUIRE( base::to_str( o ) == expected );
  }

  // Struct type.
  {
    BackupForce const o{ .regulars   = 1,
                         .dragoons   = 0,
                         .man_o_wars = 2,
                         .artillery  = 65535 };
    expected =
        "BackupForce{regulars=1,dragoons=0,man_o_wars=2,"
        "artillery=65535}";
    REQUIRE( base::to_str( o ) == expected );
  }

  // Bit field with bits<> type.
  {
    GameOptions const o{
        .unused01                    = 33,
        .tutorial_hints              = true,
        .disable_water_color_cycling = false,
        .combat_analysis             = true,
        .autosave                    = false,
        .end_of_turn                 = false,
        .fast_piece_slide            = true,
        .cheats_enabled              = false,
        .show_foreign_moves          = true,
        .show_indian_moves           = false,
    };
    expected =
        "GameOptions{unused01=0100001,tutorial_hints=true,"
        "disable_water_color_cycling=false,combat_analysis=true,"
        "autosave=false,end_of_turn=false,fast_piece_slide=true,"
        "cheats_enabled=false,show_foreign_moves=true,show_"
        "indian_moves=false}";
    REQUIRE( base::to_str( o ) == expected );
  }
}

TEST_CASE( "[sav/sav-struct] unrecognized enum value" ) {
  cdr::converter  conv;
  unit_type const o = static_cast<unit_type>( 255 );
  REQUIRE( base::to_str( o ) == "<unrecognized>" );
  REQUIRE( conv.to( o ) == cdr::null );
  REQUIRE(
      conv.from<unit_type>( "aaa" ) ==
      conv.err(
          "unrecognized value for enum unit_type: 'aaa'" ) );
}

} // namespace
} // namespace sav
