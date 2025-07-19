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

auto EuroAgents::map() const -> AgentsMap const& {
  return agents_;
}

void EuroAgents::update( e_player const player,
                         unique_ptr<IEuroAgent> agent ) {
  agents_[player] = std::move( agent );
}

/****************************************************************
** Public API.
*****************************************************************/
unique_ptr<IEuroAgent> create_euro_agent(
    IEngine& engine, SS& ss, Planes& planes, IGui& gui,
    e_player const player ) {
  switch( ss.players.players[player]->control ) {
    case e_player_control::ai: {
      if( is_ref( player ) )
        return make_unique<RefAIEuroAgent>( player, ss );
      else
        // TODO
        return make_unique<NoopEuroAgent>( ss.as_const, player );
    }
    case e_player_control::human: {
      return make_unique<HumanEuroAgent>( player, engine, ss,
                                          gui, planes );
    }
    case e_player_control::withdrawn: {
      return make_unique<NoopEuroAgent>( ss.as_const, player );
    }
  }
}

EuroAgents create_euro_agents( IEngine& engine, SS& ss,
                               Planes& planes, IGui& gui ) {
  unordered_map<e_player, unique_ptr<IEuroAgent>> holder;
  for( e_player const player : refl::enum_values<e_player> )
    if( ss.players.players[player].has_value() )
      holder[player] =
          create_euro_agent( engine, ss, planes, gui, player );
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
