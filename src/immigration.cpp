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
#include "harbor-units.hpp"
#include "igui.hpp"
#include "logger.hpp"
#include "rand-enum.hpp"
#include "ts.hpp"

// config
#include "config/immigration.rds.hpp"
#include "config/nation.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/old-world-state.rds.hpp"
#include "ss/player.hpp"
#include "ss/ref.hpp"
#include "ss/settings.hpp"
#include "ss/unit-type.hpp"
#include "ss/units.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

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

struct UnitCounts {
  int total_units                    = 0;
  int units_on_dock                  = 0;
  int units_in_cargo_of_harbor_ships = 0;
};

UnitCounts unit_counts( UnitsState const& units_state,
                        e_nation          nation ) {
  UnitCounts counts;
  for( auto const& [id, state] : units_state.all() ) {
    Unit const& unit = state.unit;
    if( unit.nation() != nation ) continue;
    ++counts.total_units;
    if( auto harbor =
            state.ownership.get_if<UnitOwnership::harbor>();
        harbor.has_value() ) {
      if( !unit.desc().ship ) {
        ++counts.units_on_dock;
      } else if( harbor->st.port_status
                     .holds<PortStatus::in_port>() ) {
        maybe<vector<UnitId>> cargo_units =
            unit.units_in_cargo();
        if( cargo_units.has_value() )
          counts.units_in_cargo_of_harbor_ships +=
              cargo_units->size();
      }
    }
  }
  return counts;
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
      .msg     = std::move( msg ),
      .options = options,
  };

  maybe<string> const res =
      co_await gui.optional_choice( config );
  if( !res.has_value() ) co_return nothing;
  if( res == "0" ) co_return 0;
  if( res == "1" ) co_return 1;
  if( res == "2" ) co_return 2;
  FATAL(
      "unexpected selection result: {} (should be '0', '1', or "
      "'2')",
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
    IRand& rand, Player const& player,
    SettingsState const& settings ) {
  WeightsMap weights = immigrant_weights_for_level(
      static_cast<int>( settings.difficulty ) );

  // Having William Brewster prevents criminals and servants from
  // showing up on the docks.
  bool has_brewster =
      player.fathers.has[e_founding_father::william_brewster];
  if( has_brewster ) {
    weights[e_unit_type::petty_criminal]     = 0.0;
    weights[e_unit_type::indentured_servant] = 0.0;
  }

  return pick_from_weighted_enum_values( rand, weights );
}

CrossesCalculation compute_crosses(
    UnitsState const& units_state, e_nation nation ) {
  // First compute the crosses bonus from the dock.
  //
  // The original game gives an extra two crosses per turn when
  // the dock is empty, and subtracts two crosses per turn for
  // each unit on the dock. That means that if no units are on
  // the dock we get +2, but if 1 unit is on the dock we get -2
  // (because we both lose the empty-dock bonus and incur the
  // penalty of one unit on the dock).
  //
  // For us though it is a bit more complicated, because (unlike
  // the original game) we have a concept of a unit being in the
  // cargo of a ship while it is in the harbor (and colony). So,
  // in the spirit of the original game, we will consider units
  // that are in the cargo of ships in harbor to count as units
  // on the dock. This will also prevent the player from circum-
  // venting the unit dock penalty by keeping a ship in harbor
  // and just moving units onto it each time one appears on dock
  // to immediately regain the dock bonus.
  auto const [total_units, units_on_dock, harbor_cargo_units] =
      unit_counts( units_state, nation );
  int const effective_units_on_dock =
      units_on_dock + harbor_cargo_units;
  int const dock_crosses_bonus =
      ( effective_units_on_dock == 0 )
          ? 2
          : ( -effective_units_on_dock * 2 );
  DCHECK( dock_crosses_bonus != 0 );

  // Next compute the total number of crosses needed for the next
  // immigration. The formula given in the Colonization 1
  // strategy guide is this:
  //
  //   8 + 2*(units in colonies + units (people?) in new world)
  //
  // But this does not seem to be right.  This page:
  //
  //   civilization.fandom.com/wiki/Colonization_tips
  //
  // gives the correct formula, which is:
  //
  //   8 + 2*(units + units-on-dock)
  //
  // where 'units' are all owned units of any kind, including the
  // ones on the dock, and units-on-dock are the units on the
  // dock (and so they are counted twice here). Note that "dock"
  // here refers only to the non-ship units on the dock. As de-
  // scribed in the above link, the Col 1 debug view will not in-
  // clude the units-on-dock when it prints the number of crosses
  // needed on the Religion Advisor page, but it still includes
  // them in the calculation.
  //
  // And again, as above, note that we are counting units in the
  // cargo of harbor ships as being on dock for the purposes of
  // this calculation.
  int const default_crosses_needed =
      8 + 2 * ( total_units + effective_units_on_dock );

  // This is what will incorporate the English's special ability
  // to more quickly attract immigrants.
  int const crosses_needed = std::lround(
      default_crosses_needed * config_nation.abilities[nation]
                                   .crosses_needed_multiplier );

  return CrossesCalculation{
      .dock_crosses_bonus = dock_crosses_bonus,
      .crosses_needed     = crosses_needed };
}

void add_player_crosses( Player& player,
                         int     total_colonies_cross_production,
                         int     dock_crosses_bonus ) {
  // This bit of logic is important: the total colonies' produc-
  // tion must be added to the dock bonus before adding them to
  // the player's total so that we can make sure that the differ-
  // ential is not negative. That could happen e.g. when the
  // player has only one colony producing a single cross but has
  // one or more units waiting on the dock.
  int const delta =
      total_colonies_cross_production + dock_crosses_bonus;
  if( delta < 0 ) return;
  lg.debug( "{} crosses increased by {}.", player.nation,
            delta );
  player.crosses += delta;
}

wait<maybe<UnitId>> check_for_new_immigrant(
    SS& ss, TS& ts, Player& player, int crosses_needed ) {
  CHECK_GE( crosses_needed, 0 );
  if( player.crosses < crosses_needed ) co_return nothing;
  player.crosses -= crosses_needed;
  DCHECK( player.crosses >= 0 );
  int immigrant_idx = {};
  if( player.fathers.has[e_founding_father::william_brewster] ) {
    string msg =
        "Word of religious freedom has spread! New immigrants "
        "are ready to join us in the New World.  Which of the "
        "following shall we choose?";
    maybe<int> const maybe_immigrant_idx =
        co_await ask_player_to_choose_immigrant(
            ts.gui, player.old_world.immigration, msg );
    if( !maybe_immigrant_idx.has_value() ) co_return nothing;
    immigrant_idx = *maybe_immigrant_idx;
    CHECK_GE( immigrant_idx, 0 );
    CHECK_LE( immigrant_idx, 2 );
  } else {
    immigrant_idx =
        ts.rand.between_ints( 0, 2, IRand::e_interval::closed );
    string msg = fmt::format(
        "Word of religious freedom has spread! A new immigrant "
        "(@[H]{}@[]) has arrived on the docks.",
        unit_attr( player.old_world.immigration
                       .immigrants_pool[immigrant_idx] )
            .name );
    co_await ts.gui.message_box( msg );
  }
  e_unit_type replacement =
      pick_next_unit_for_pool( ts.rand, player, ss.settings );
  e_unit_type type = take_immigrant_from_pool(
      player.old_world.immigration, immigrant_idx, replacement );
  co_return create_unit_in_harbor( ss.units, player, type );
}

int cost_of_recruit( Player const& player, int crosses_needed,
                     e_difficulty difficulty ) {
  // The formula that the OG appears to use to compute the cost
  // of a rushed recruitment is:
  //
  //   cost = max( ((T-X)/T)*(M+P*N)+B, I )
  //
  // where:
  //
  //   T: total crosses needed for immigrant.
  //   X: crosses accumulated since last immigrant.
  //   M: a difficulty dependent term.
  //   P: amount by which the starting price increases as a
  //      result of each additional rushed recruit.
  //   N: number of rushed recuits to date.
  //   B: fixed baseline cost.
  //   I: minimum cost.
  //
  int const B = config_immigration.rush_cost_baseline;
  int const P = config_immigration
                    .multiplier_increase_per_rushed_immigrant;
  int const M = config_immigration
                    .rush_cost_crosses_multiplier[difficulty];
  int const T = crosses_needed;
  int const X = player.crosses;
  int const N = player.old_world.immigration.num_recruits_rushed;
  int const I = config_immigration.rush_cost_min;

  CHECK_GT( T, 0 );
  int const cost =
      lround( ( ( T - X ) / double( T ) ) * ( M + P * N ) + B );
  return std::max( cost, I );
}

void rush_recruit_next_immigrant( SS& ss, TS& ts, Player& player,
                                  int slot_selected ) {
  auto& pool = player.old_world.immigration.immigrants_pool;
  e_unit_type const selected_type = pool[slot_selected];
  e_unit_type const replacement =
      pick_next_unit_for_pool( ts.rand, player, ss.settings );
  CHECK( take_immigrant_from_pool(
             player.old_world.immigration, slot_selected,
             replacement ) == selected_type );
  CrossesCalculation const crosses =
      compute_crosses( ss.units, player.nation );
  player.money -= cost_of_recruit(
      player, crosses.crosses_needed, ss.settings.difficulty );
  CHECK_GE( player.money, 0 );
  ++player.old_world.immigration.num_recruits_rushed;
  player.crosses = 0;
  create_unit_in_harbor( ss.units, player, selected_type );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( pick_next_unit_for_pool, e_unit_type,
        Player const& player, SettingsState const& settings ) {
  IRand& rand = st["ROOT_TS"]["rand"].as<IRand&>();
  return pick_next_unit_for_pool( rand, player, settings );
};

} // namespace

} // namespace rn
