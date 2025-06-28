/****************************************************************
**agents.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-09-07.
*
* Description: Stuff for managing agent implementations.
*
*****************************************************************/
#include "agents.hpp"

// Revolution Now
#include "ai-native-agent.hpp"
#include "human-euro-agent.hpp"
#include "ieuro-agent.hpp"
#include "inative-agent.hpp"

// ss
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"

// refl
#include "refl/to-str.hpp"

// C++ standard library
#include <memory>

using namespace std;

namespace rn {

/****************************************************************
** NativeAgents
*****************************************************************/
NativeAgents::NativeAgents( AgentsMap agents )
  : agents_( std::move( agents ) ) {}

NativeAgents::~NativeAgents() = default;

NativeAgents& NativeAgents::operator=(
    NativeAgents&& ) noexcept = default;

INativeAgent& NativeAgents::operator[]( e_tribe tribe ) const {
  auto iter = agents_.find( tribe );
  CHECK( iter != agents_.end(),
         "no INativeAgent object for tribe {}.", tribe );
  unique_ptr<INativeAgent> const& p_agent = iter->second;
  CHECK( p_agent != nullptr,
         "null INativeAgent object for tribe {}.", tribe );
  return *p_agent;
}

/****************************************************************
** EuroAgents
*****************************************************************/
EuroAgents::EuroAgents( AgentsMap agents )
  : agents_( std::move( agents ) ) {}

EuroAgents::~EuroAgents() = default;

EuroAgents& EuroAgents::operator=( EuroAgents&& ) noexcept =
    default;

IEuroAgent& EuroAgents::operator[]( e_player player ) const {
  auto iter = agents_.find( player );
  CHECK( iter != agents_.end(),
         "no IEuroAgent object for player {}.", player );
  unique_ptr<IEuroAgent> const& p_agent = iter->second;
  CHECK( p_agent != nullptr,
         "null IEuroAgent object for player {}.", player );
  return *p_agent;
}

/****************************************************************
** Public API.
*****************************************************************/
EuroAgents create_euro_agents( SS& ss, IGui& gui ) {
  unordered_map<e_player, unique_ptr<IEuroAgent>> holder;
  for( e_player const player : refl::enum_values<e_player> ) {
    if( !ss.players.players[player].has_value() ) continue;
    switch( ss.players.players[player]->control ) {
      case e_player_control::ai: {
        // TODO
        holder[player] =
            make_unique<NoopEuroAgent>( ss.as_const, player );
        break;
      }
      case e_player_control::human: {
        holder[player] =
            make_unique<HumanEuroAgent>( player, ss, gui );
        break;
      }
      case e_player_control::withdrawn: {
        holder[player] =
            make_unique<NoopEuroAgent>( ss.as_const, player );
        break;
      }
    }
  }
  return EuroAgents( std::move( holder ) );
}

NativeAgents create_native_agents( SS& ss, IRand& rand ) {
  unordered_map<e_tribe, unique_ptr<INativeAgent>> holder;
  for( e_tribe const tribe : refl::enum_values<e_tribe> )
    holder[tribe] =
        make_unique<AiNativeAgent>( ss, rand, tribe );
  return NativeAgents( std::move( holder ) );
}

} // namespace rn
