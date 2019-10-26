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
#include "fmt-helper.hpp"
#include "logging.hpp"
#include "lua.hpp"
#include "util.hpp"

// Flatbuffers
#include "fb/sg-player_generated.h"

// Abseil
#include "absl/container/node_hash_map.h"

using namespace std;
namespace rn {

namespace {

/****************************************************************
** Save-Game State
*****************************************************************/
struct SAVEGAME_STRUCT( Player ) {
  using PlayerMap_t = NodeMap<e_nation, Player>;

  // Fields that are actually serialized.

  // clang-format off
  SAVEGAME_MEMBERS( Player,
  ( PlayerMap_t, players ));
  // clang-format on

public:
  // Fields that are derived from the serialized fields.

private:
  SAVEGAME_FRIENDS( Player );
  SAVEGAME_SYNC() {
    // Sync all fields that are derived from serialized fields
    // and then validate (check invariants).

    // Check that players have the correct nation relative to
    // their key in the map.
    for( auto const& [nation, player] : players )
      UNXP_CHECK( player.nation() == nation,
                  "mismatch in player nations." );

    return xp_success_t{};
  }
};
SAVEGAME_IMPL( Player );

} // namespace

/****************************************************************
** Player
*****************************************************************/
Player::Player( e_nation nation, bool is_human, int money )
  : nation_( nation ), human_( is_human ), money_( money ) {}

/****************************************************************
** Public API
*****************************************************************/
Player& player_for_nation( e_nation nation ) {
  CHECK( SG().players.contains( nation ) );
  return SG().players[nation];
}

void set_players( Vec<e_nation> const& nations ) {
  SG().players.clear();
  for( auto nation : nations ) {
    SG().players.emplace( nation, Player(
                                      /*nation=*/nation, //
                                      /*is_human=*/true, //
                                      /*money=*/0        //
                                      ) );
  }
}

void linker_dont_discard_module_player() {}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( set_players, void,
        sol::as_table_t<Vec<e_nation>> const& nations ) {
  auto const& vec = nations.value();
  lg.info( "enabling nations: {}", FmtJsonStyleList{ vec } );
  set_players( vec );
}

} // namespace

} // namespace rn
