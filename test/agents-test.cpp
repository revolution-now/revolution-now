/****************************************************************
**agents-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-09-07.
*
* Description: Unit tests for the agents module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/agents.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocks/iagent.hpp"
#include "test/mocks/iengine.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/inative-agent.hpp"
#include "test/mocks/irand.hpp"

// Revolution Now
#include "src/ai-native-agent.hpp"
#include "src/human-agent.hpp"
#include "src/ref-ai-agent.hpp"

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
    add_player( e_player::ref_english );
    add_player( e_player::ref_french );
    add_player( e_player::ref_spanish );
    add_player( e_player::ref_dutch );
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
static_assert( !is_copy_assignable_v<Agents> );
static_assert( is_move_assignable_v<Agents> );
static_assert( is_nothrow_move_assignable_v<Agents> );

static_assert( !is_copy_assignable_v<NativeAgents> );
static_assert( is_move_assignable_v<NativeAgents> );
static_assert( is_nothrow_move_assignable_v<NativeAgents> );

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[agents] create_agent" ) {
  World W;

  W.english().control     = e_player_control::human;
  W.french().control      = e_player_control::human;
  W.spanish().control     = e_player_control::ai;
  W.dutch().control       = e_player_control::ai;
  W.ref_english().control = e_player_control::human;
  W.ref_french().control  = e_player_control::human;
  W.ref_spanish().control = e_player_control::ai;
  W.ref_dutch().control   = e_player_control::ai;

  auto f = [&]( e_player const p ) {
    return create_agent( W.engine(), W.ss(), W.map_updater(),
                         W.planes(), W.gui(), W.rand(), p );
  };

  auto const english_agent     = f( e_player::english );
  auto const french_agent      = f( e_player::french );
  auto const spanish_agent     = f( e_player::spanish );
  auto const dutch_agent       = f( e_player::dutch );
  auto const ref_english_agent = f( e_player::ref_english );
  auto const ref_french_agent  = f( e_player::ref_french );
  auto const ref_spanish_agent = f( e_player::ref_spanish );
  auto const ref_dutch_agent   = f( e_player::ref_dutch );

  REQUIRE( english_agent->player_type() == e_player::english );
  REQUIRE( french_agent->player_type() == e_player::french );
  REQUIRE( spanish_agent->player_type() == e_player::spanish );
  REQUIRE( dutch_agent->player_type() == e_player::dutch );
  REQUIRE( ref_english_agent->player_type() ==
           e_player::ref_english );
  REQUIRE( ref_french_agent->player_type() ==
           e_player::ref_french );
  REQUIRE( ref_spanish_agent->player_type() ==
           e_player::ref_spanish );
  REQUIRE( ref_dutch_agent->player_type() ==
           e_player::ref_dutch );

  REQUIRE( dynamic_cast<HumanAgent*>( english_agent.get() ) !=
           nullptr );
  REQUIRE( dynamic_cast<HumanAgent*>( french_agent.get() ) !=
           nullptr );
  REQUIRE( dynamic_cast<HumanAgent*>( spanish_agent.get() ) ==
           nullptr );
  REQUIRE( dynamic_cast<HumanAgent*>( dutch_agent.get() ) ==
           nullptr );
  REQUIRE( dynamic_cast<HumanAgent*>(
               ref_english_agent.get() ) != nullptr );
  REQUIRE( dynamic_cast<HumanAgent*>( ref_french_agent.get() ) !=
           nullptr );
  REQUIRE( dynamic_cast<HumanAgent*>(
               ref_spanish_agent.get() ) == nullptr );
  REQUIRE( dynamic_cast<HumanAgent*>( ref_dutch_agent.get() ) ==
           nullptr );

  REQUIRE( dynamic_cast<NoopAgent*>( english_agent.get() ) ==
           nullptr );
  REQUIRE( dynamic_cast<NoopAgent*>( french_agent.get() ) ==
           nullptr );
  REQUIRE( dynamic_cast<NoopAgent*>( spanish_agent.get() ) !=
           nullptr );
  REQUIRE( dynamic_cast<NoopAgent*>( dutch_agent.get() ) !=
           nullptr );
  REQUIRE( dynamic_cast<NoopAgent*>( ref_english_agent.get() ) ==
           nullptr );
  REQUIRE( dynamic_cast<NoopAgent*>( ref_french_agent.get() ) ==
           nullptr );
  REQUIRE( dynamic_cast<RefAIAgent*>(
               ref_spanish_agent.get() ) != nullptr );
  REQUIRE( dynamic_cast<RefAIAgent*>( ref_dutch_agent.get() ) !=
           nullptr );
}

TEST_CASE( "[agents] create_agents" ) {
  World W;

  W.english().control     = e_player_control::human;
  W.french().control      = e_player_control::human;
  W.spanish().control     = e_player_control::ai;
  W.dutch().control       = e_player_control::ai;
  W.ref_english().control = e_player_control::human;
  W.ref_french().control  = e_player_control::human;
  W.ref_spanish().control = e_player_control::ai;
  W.ref_dutch().control   = e_player_control::ai;

  auto f = [&] {
    return create_agents( W.engine(), W.ss(), W.map_updater(),
                          W.planes(), W.gui(), W.rand() );
  };

  Agents const agents = f();

  auto& english_agent     = agents[e_player::english];
  auto& french_agent      = agents[e_player::french];
  auto& spanish_agent     = agents[e_player::spanish];
  auto& dutch_agent       = agents[e_player::dutch];
  auto& ref_english_agent = agents[e_player::ref_english];
  auto& ref_french_agent  = agents[e_player::ref_french];
  auto& ref_spanish_agent = agents[e_player::ref_spanish];
  auto& ref_dutch_agent   = agents[e_player::ref_dutch];

  REQUIRE( english_agent.player_type() == e_player::english );
  REQUIRE( french_agent.player_type() == e_player::french );
  REQUIRE( spanish_agent.player_type() == e_player::spanish );
  REQUIRE( dutch_agent.player_type() == e_player::dutch );
  REQUIRE( ref_english_agent.player_type() ==
           e_player::ref_english );
  REQUIRE( ref_french_agent.player_type() ==
           e_player::ref_french );
  REQUIRE( ref_spanish_agent.player_type() ==
           e_player::ref_spanish );
  REQUIRE( ref_dutch_agent.player_type() ==
           e_player::ref_dutch );

  REQUIRE( dynamic_cast<HumanAgent*>( &english_agent ) !=
           nullptr );
  REQUIRE( dynamic_cast<HumanAgent*>( &french_agent ) !=
           nullptr );
  REQUIRE( dynamic_cast<HumanAgent*>( &spanish_agent ) ==
           nullptr );
  REQUIRE( dynamic_cast<HumanAgent*>( &dutch_agent ) ==
           nullptr );
  REQUIRE( dynamic_cast<HumanAgent*>( &ref_english_agent ) !=
           nullptr );
  REQUIRE( dynamic_cast<HumanAgent*>( &ref_french_agent ) !=
           nullptr );
  REQUIRE( dynamic_cast<HumanAgent*>( &ref_spanish_agent ) ==
           nullptr );
  REQUIRE( dynamic_cast<HumanAgent*>( &ref_dutch_agent ) ==
           nullptr );

  REQUIRE( dynamic_cast<NoopAgent*>( &english_agent ) ==
           nullptr );
  REQUIRE( dynamic_cast<NoopAgent*>( &french_agent ) ==
           nullptr );
  REQUIRE( dynamic_cast<NoopAgent*>( &spanish_agent ) !=
           nullptr );
  REQUIRE( dynamic_cast<NoopAgent*>( &dutch_agent ) != nullptr );
  REQUIRE( dynamic_cast<NoopAgent*>( &ref_english_agent ) ==
           nullptr );
  REQUIRE( dynamic_cast<NoopAgent*>( &ref_french_agent ) ==
           nullptr );
  REQUIRE( dynamic_cast<RefAIAgent*>( &ref_spanish_agent ) !=
           nullptr );
  REQUIRE( dynamic_cast<RefAIAgent*>( &ref_dutch_agent ) !=
           nullptr );
}

TEST_CASE( "[agents] create_native_agents" ) {
  World W;

  auto f = [&] {
    return create_native_agents( W.ss(), W.rand() );
  };

  NativeAgents const agents = f();

  for( e_tribe const tribe_type : refl::enum_values<e_tribe> ) {
    INFO( fmt::format( "tribe: {}", tribe_type ) );
    REQUIRE( agents[tribe_type].tribe_type() == tribe_type );
    REQUIRE( dynamic_cast<AiNativeAgent*>(
                 &agents[tribe_type] ) != nullptr );
  }
}

} // namespace
} // namespace rn
