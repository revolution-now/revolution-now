/****************************************************************
**player.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-29.
*
* Description: Data structure representing a human or AI player.
*
*****************************************************************/
#include "player.hpp"

// Revolution Now
#include "aliases.hpp"
#include "util.hpp"

// Abseil
#include "absl/container/node_hash_map.h"

using namespace std;
namespace rn {

namespace {

/****************************************************************
** Global State
*****************************************************************/
NodeMap<e_nation, Player> g_players;

} // namespace

/****************************************************************
** Player
*****************************************************************/
Player::Player( NationDesc const& nation, bool is_human,
                int money )
  : nation_( nation ), human_( is_human ), money_( money ) {}

/****************************************************************
** Public API
*****************************************************************/
Player& player_for_nation( e_nation nation ) {
  if( !g_players.contains( nation ) ) {
    g_players.emplace( nation,
                       Player(
                           /*nation=*/nation_obj( nation ), //
                           /*is_human=*/false,              //
                           /*money=*/0                      //
                           ) );
  }
  return val_or_die( g_players, nation );
}

} // namespace rn
