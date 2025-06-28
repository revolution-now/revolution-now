/****************************************************************
**minds-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-09-07.
*
* Description: Unit tests for the minds module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/agents.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocks/ieuro-agent.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/inative-agent.hpp"
#include "test/mocks/irand.hpp"

// Revolution Now
#include "src/ai-native-agent.hpp"
#include "src/human-euro-agent.hpp"

// ss
#include "src/ss/players.rds.hpp"

// refl
#include "src/refl/to-str.hpp"

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
    add_player( e_player::english );
    add_player( e_player::french );
    add_player( e_player::spanish );
    add_player( e_player::dutch );
    set_default_player_type( e_player::english );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      _, L, _, //
      L, L, L, //
      _, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Static checks.
*****************************************************************/
static_assert( !is_copy_assignable_v<EuroMinds> );
static_assert( is_move_assignable_v<EuroMinds> );
static_assert( is_nothrow_move_assignable_v<EuroMinds> );

static_assert( !is_copy_assignable_v<NativeMinds> );
static_assert( is_move_assignable_v<NativeMinds> );
static_assert( is_nothrow_move_assignable_v<NativeMinds> );

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[minds] create_euro_minds" ) {
  World W;

  W.english().control = e_player_control::human;
  W.french().control  = e_player_control::human;
  W.spanish().control = e_player_control::ai;
  W.dutch().control   = e_player_control::ai;

  auto f = [&] { return create_euro_minds( W.ss(), W.gui() ); };

  EuroMinds const minds = f();

  auto& english_mind = minds[e_player::english];
  auto& french_mind  = minds[e_player::french];
  auto& spanish_mind = minds[e_player::spanish];
  auto& dutch_mind   = minds[e_player::dutch];

  REQUIRE( english_mind.player_type() == e_player::english );
  REQUIRE( french_mind.player_type() == e_player::french );
  REQUIRE( spanish_mind.player_type() == e_player::spanish );
  REQUIRE( dutch_mind.player_type() == e_player::dutch );

  REQUIRE( dynamic_cast<HumanEuroMind*>( &english_mind ) !=
           nullptr );
  REQUIRE( dynamic_cast<HumanEuroMind*>( &french_mind ) !=
           nullptr );
  REQUIRE( dynamic_cast<HumanEuroMind*>( &spanish_mind ) ==
           nullptr );
  REQUIRE( dynamic_cast<HumanEuroMind*>( &dutch_mind ) ==
           nullptr );

  REQUIRE( dynamic_cast<NoopEuroMind*>( &english_mind ) ==
           nullptr );
  REQUIRE( dynamic_cast<NoopEuroMind*>( &french_mind ) ==
           nullptr );
  REQUIRE( dynamic_cast<NoopEuroMind*>( &spanish_mind ) !=
           nullptr );
  REQUIRE( dynamic_cast<NoopEuroMind*>( &dutch_mind ) !=
           nullptr );
}

TEST_CASE( "[minds] create_native_minds" ) {
  World W;

  auto f = [&] {
    return create_native_minds( W.ss(), W.rand() );
  };

  NativeMinds const minds = f();

  for( e_tribe const tribe_type : refl::enum_values<e_tribe> ) {
    INFO( fmt::format( "tribe: {}", tribe_type ) );
    REQUIRE( minds[tribe_type].tribe_type() == tribe_type );
    REQUIRE( dynamic_cast<AiNativeMind*>( &minds[tribe_type] ) !=
             nullptr );
  }
}

} // namespace
} // namespace rn
