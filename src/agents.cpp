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
** NativeMinds
*****************************************************************/
NativeMinds::NativeMinds( MindsMap minds )
  : minds_( std::move( minds ) ) {}

NativeMinds::~NativeMinds() = default;

NativeMinds& NativeMinds::operator=( NativeMinds&& ) noexcept =
    default;

INativeMind& NativeMinds::operator[]( e_tribe tribe ) const {
  auto iter = minds_.find( tribe );
  CHECK( iter != minds_.end(),
         "no INativeMind object for tribe {}.", tribe );
  unique_ptr<INativeMind> const& p_mind = iter->second;
  CHECK( p_mind != nullptr,
         "null INativeMind object for tribe {}.", tribe );
  return *p_mind;
}

/****************************************************************
** EuroMinds
*****************************************************************/
EuroMinds::EuroMinds( MindsMap minds )
  : minds_( std::move( minds ) ) {}

EuroMinds::~EuroMinds() = default;

EuroMinds& EuroMinds::operator=( EuroMinds&& ) noexcept =
    default;

IEuroMind& EuroMinds::operator[]( e_player player ) const {
  auto iter = minds_.find( player );
  CHECK( iter != minds_.end(),
         "no IEuroMind object for player {}.", player );
  unique_ptr<IEuroMind> const& p_mind = iter->second;
  CHECK( p_mind != nullptr,
         "null IEuroMind object for player {}.", player );
  return *p_mind;
}

/****************************************************************
** Public API.
*****************************************************************/
EuroMinds create_euro_minds( SS& ss, IGui& gui ) {
  unordered_map<e_player, unique_ptr<IEuroMind>> holder;
  for( e_player const player : refl::enum_values<e_player> ) {
    if( !ss.players.players[player].has_value() ) continue;
    switch( ss.players.players[player]->control ) {
      case e_player_control::ai: {
        // TODO
        holder[player] =
            make_unique<NoopEuroMind>( ss.as_const, player );
        break;
      }
      case e_player_control::human: {
        holder[player] =
            make_unique<HumanEuroMind>( player, ss, gui );
        break;
      }
      case e_player_control::withdrawn: {
        holder[player] =
            make_unique<NoopEuroMind>( ss.as_const, player );
        break;
      }
    }
  }
  return EuroMinds( std::move( holder ) );
}

NativeMinds create_native_minds( SS& ss, IRand& rand ) {
  unordered_map<e_tribe, unique_ptr<INativeMind>> holder;
  for( e_tribe const tribe : refl::enum_values<e_tribe> )
    holder[tribe] = make_unique<AiNativeMind>( ss, rand, tribe );
  return NativeMinds( std::move( holder ) );
}

} // namespace rn
