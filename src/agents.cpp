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
#include "human-agent.hpp"
#include "iagent.hpp"
#include "inative-agent.hpp"
#include "ref-ai-agent.hpp"

// ss
#include "ss/nation.hpp"
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
** Agents
*****************************************************************/
Agents::Agents( AgentsMap agents )
  : agents_( std::move( agents ) ) {}

Agents::~Agents() = default;

Agents& Agents::operator=( Agents&& ) noexcept = default;

IAgent& Agents::operator[]( e_player player ) const {
  auto iter = agents_.find( player );
  CHECK( iter != agents_.end(),
         "no IAgent object for player {}.", player );
  unique_ptr<IAgent> const& p_agent = iter->second;
  CHECK( p_agent != nullptr, "null IAgent object for player {}.",
         player );
  return *p_agent;
}

auto Agents::map() const -> AgentsMap const& { return agents_; }

void Agents::update( e_player const player,
                     unique_ptr<IAgent> agent ) {
  agents_[player] = std::move( agent );
}

/****************************************************************
** Public API.
*****************************************************************/
unique_ptr<IAgent> create_agent( IEngine& engine, SS& ss,
                                 Planes& planes, IGui& gui,
                                 e_player const player ) {
  switch( ss.players.players[player]->control ) {
    case e_player_control::ai: {
      if( is_ref( player ) )
        return make_unique<RefAIAgent>( player, ss );
      else
        // TODO
        return make_unique<NoopAgent>( ss.as_const, player );
    }
    case e_player_control::human: {
      return make_unique<HumanAgent>( player, engine, ss, gui,
                                      planes );
    }
    case e_player_control::inactive: {
      return make_unique<NoopAgent>( ss.as_const, player );
    }
  }
}

Agents create_agents( IEngine& engine, SS& ss, Planes& planes,
                      IGui& gui ) {
  unordered_map<e_player, unique_ptr<IAgent>> holder;
  for( e_player const player : refl::enum_values<e_player> )
    if( ss.players.players[player].has_value() )
      holder[player] =
          create_agent( engine, ss, planes, gui, player );
  return Agents( std::move( holder ) );
}

NativeAgents create_native_agents( SS& ss, IRand& rand ) {
  unordered_map<e_tribe, unique_ptr<INativeAgent>> holder;
  for( e_tribe const tribe : refl::enum_values<e_tribe> )
    holder[tribe] =
        make_unique<AiNativeAgent>( ss, rand, tribe );
  return NativeAgents( std::move( holder ) );
}

} // namespace rn
