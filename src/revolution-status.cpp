/****************************************************************
**revolution-status.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-13.
*
* Description: Things related to changing revolution status.
*
*****************************************************************/
#include "revolution-status.hpp"

// config
#include "config/nation.rds.hpp"

// ss
#include "ss/player.rds.hpp"

using namespace std;

namespace rn {

namespace {

// TODO

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
string_view nation_possessive( Player const& player ) {
  if( player.revolution_status >=
      e_revolution_status::declared ) {
    static std::string const rebel = "Rebel";
    return rebel;
  }
  return config_nation.nations[player.nation]
      .possessive_pre_declaration;
}

string_view nation_display_name( Player const& player ) {
  if( player.revolution_status >=
      e_revolution_status::declared ) {
    static std::string const rebel = "Rebels";
    return rebel;
  }
  return config_nation.nations[player.nation]
      .display_name_pre_declaration;
}

} // namespace rn
