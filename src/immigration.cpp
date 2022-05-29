/****************************************************************
**immigration.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-28.
*
* Description: All things immigration.
*
*****************************************************************/
#include "immigration.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "gs-settings.hpp"
#include "igui.hpp"
#include "lua.hpp"
#include "player.hpp"
#include "rand-enum.hpp"
#include "utype.hpp"

// config
#include "config/immigration.rds.hpp"

// Rds
#include "old-world-state.rds.hpp"

// luapp
#include "luapp/state.hpp"

using namespace std;

namespace rn {

namespace {

using WeightsMap = refl::enum_map<e_unit_type, double>;

WeightsMap immigrant_weights_for_level( int level ) {
  WeightsMap const& scaling =
      config_immigration.difficulty_factor_per_level;
  // Any unit types that are not in the config will have default
  // values of 0.0 in the below enum_map which is what we want,
  // since that will prevent them from being selected.
  WeightsMap weights = config_immigration.base_weights;
  for( e_unit_type type : refl::enum_values<e_unit_type> )
    weights[type] *= pow( scaling[type], double( level ) );
  return weights;
}

} // namespace

wait<maybe<int>> ask_player_to_choose_immigrant(
    IGui& gui, ImmigrationState const& immigration,
    string msg ) {
  array<e_unit_type, 3> const& pool =
      immigration.immigrants_pool;
  vector<ChoiceConfigOption> options{
      { .key = "0", .display_name = unit_attr( pool[0] ).name },
      { .key = "1", .display_name = unit_attr( pool[1] ).name },
      { .key = "2", .display_name = unit_attr( pool[2] ).name },
  };
  ChoiceConfig config{
      .msg           = std::move( msg ),
      .options       = options,
      .key_on_escape = "-",
  };

  std::string res = co_await gui.choice( config );
  if( res == "0" ) co_return 0;
  if( res == "1" ) co_return 1;
  if( res == "2" ) co_return 2;
  if( res == "-" ) co_return nothing;
  FATAL(
      "unexpected selection result: {} (should be '0', '1', or "
      "'2', or '-')",
      res );
}

e_unit_type take_immigrant_from_pool(
    ImmigrationState& immigration, int n,
    e_unit_type replacement ) {
  CHECK_GE( n, 0 );
  CHECK_LE( n, 2 );
  DCHECK( immigration.immigrants_pool.size() >= 3 );
  e_unit_type taken = immigration.immigrants_pool[n];
  immigration.immigrants_pool[n] = replacement;
  return taken;
}

e_unit_type pick_next_unit_for_pool(
    Player const& player, SettingsState const& settings ) {
  WeightsMap weights =
      immigrant_weights_for_level( settings.difficulty );

  // Having William Brewster prevents criminals and servants from
  // showing up on the docks.
  bool has_brewster =
      player.has_father( e_founding_father::william_brewster );
  if( has_brewster ) {
    weights[e_unit_type::petty_criminal]     = 0.0;
    weights[e_unit_type::indentured_servant] = 0.0;
  }

  return rng::pick_from_weighted_enum_values( weights );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( pick_next_unit_for_pool, e_unit_type,
        Player const& player, SettingsState const& settings ) {
  return pick_next_unit_for_pool( player, settings );
};

} // namespace

} // namespace rn
