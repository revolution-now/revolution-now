/****************************************************************
**minds.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-09-07.
*
* Description: Stuff for managing mind implementations.
*
*****************************************************************/
#include "minds.hpp"

// Revolution Now
#include "ai-native-mind.hpp"
#include "human-euro-mind.hpp"
#include "ieuro-mind.hpp"
#include "inative-mind.hpp"

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

IEuroMind& EuroMinds::operator[]( e_nation nation ) const {
  auto iter = minds_.find( nation );
  CHECK( iter != minds_.end(),
         "no IEuroMind object for nation {}.", nation );
  unique_ptr<IEuroMind> const& p_mind = iter->second;
  CHECK( p_mind != nullptr,
         "null IEuroMind object for nation {}.", nation );
  return *p_mind;
}

/****************************************************************
** Public API.
*****************************************************************/
EuroMinds create_euro_minds( SS& ss, IGui& gui ) {
  unordered_map<e_nation, unique_ptr<IEuroMind>> holder;
  for( e_nation const nation : refl::enum_values<e_nation> ) {
    if( ss.players.humans[nation] )
      holder[nation] =
          make_unique<HumanEuroMind>( nation, ss, gui );
    else
      holder[nation] = make_unique<NoopEuroMind>( nation );
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
