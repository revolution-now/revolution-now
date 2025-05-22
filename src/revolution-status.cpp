/****************************************************************
**revolution-status.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-13.
*
* Description: Things that change depending on revolution status.
*
*****************************************************************/
#include "revolution-status.hpp"

// config
#include "config/nation.rds.hpp"

// ss
#include "ss/player.rds.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API.
*****************************************************************/
string_view player_possessive( Player const& player ) {
  if( player.revolution.status >=
      e_revolution_status::declared ) {
    static std::string const rebel = "Rebel";
    return rebel;
  }
  return config_nation.players[player.type]
      .possessive_pre_declaration;
}

string_view player_display_name( Player const& player ) {
  if( player.revolution.status >=
      e_revolution_status::declared ) {
    static std::string const rebel = "Rebels";
    return rebel;
  }
  return config_nation.players[player.type]
      .display_name_pre_declaration;
}

} // namespace rn
