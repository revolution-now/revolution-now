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
#include "src/minds.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocks/ieuro-mind.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/inative-mind.hpp"
#include "test/mocks/irand.hpp"

// Revolution Now
#include "src/ai-native-mind.hpp"
#include "src/human-euro-mind.hpp"

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
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
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

  W.players().humans[e_nation::english] = true;
  W.players().humans[e_nation::french]  = true;
  W.players().humans[e_nation::spanish] = false;
  W.players().humans[e_nation::dutch]   = false;

  auto f = [&] { return create_euro_minds( W.ss(), W.gui() ); };

  EuroMinds const minds = f();

  auto& english_mind = minds[e_nation::english];
  auto& french_mind  = minds[e_nation::french];
  auto& spanish_mind = minds[e_nation::spanish];
  auto& dutch_mind   = minds[e_nation::dutch];

  REQUIRE( english_mind.nation() == e_nation::english );
  REQUIRE( french_mind.nation() == e_nation::french );
  REQUIRE( spanish_mind.nation() == e_nation::spanish );
  REQUIRE( dutch_mind.nation() == e_nation::dutch );

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
