/****************************************************************
**rpt.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-20.
*
* Description: Implementation of the Recruit/Purchase/Train
*              buttons in the harbor view.
*
*****************************************************************/
#include "rpt.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "conv.hpp"
#include "harbor-units.hpp"
#include "igui.hpp"
#include "immigration.hpp"
#include "ts.hpp"

// config
#include "config/harbor.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.hpp"
#include "ss/unit-type.rds.hpp"

// base
#include "base/keyval.hpp"
#include "base/string.hpp"

using namespace std;

namespace rn {

namespace {

int artillery_price( Player const& player ) {
  return config_harbor.purchases.artillery_initial_price +
         player.artillery_purchases *
             config_harbor.purchases.artillery_increase_rate;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
wait<> click_recruit( SS& ss, TS& ts, Player& player ) {
  CrossesCalculation const crosses =
      compute_crosses( ss.units, player.nation );
  int const price = cost_of_recruit(
      player, crosses.crosses_needed, ss.settings.difficulty );
  ChoiceConfig config{
      .msg = fmt::format(
          "The following individuals will accompany us to the "
          "new world if we pay their passage ([{} gold]).  "
          "Whom shall we recruit?",
          price ),
      .initial_selection = 0 };
  static string const kNone = "none";
  config.options.push_back(
      { .key = kNone, .display_name = "(None)" } );
  auto& pool = player.old_world.immigration.immigrants_pool;
  static_assert(
      tuple_size_v<remove_cvref_t<decltype( pool )>> == 3 );
  auto push = [&]( int n ) {
    e_unit_type const type         = pool[n];
    string const      key          = fmt::to_string( n );
    string const      display_name = base::capitalize_initials(
        unit_attr( type ).name_plural );
    bool const enabled = ( player.money >= price );
    config.options.push_back( { .key          = key,
                                .display_name = display_name,
                                .disabled     = !enabled } );
  };
  push( 0 );
  push( 1 );
  push( 2 );
  // FIXME: there's got to be a better way of getting an integer
  // back than manually creating stringified integer keys and
  // converting them back to ints.
  maybe<string> selected_str =
      co_await ts.gui.optional_choice( config );
  if( !selected_str.has_value() ) co_return;
  if( *selected_str == kNone ) co_return;
  UNWRAP_CHECK( selected_n,
                base::from_chars<int>( *selected_str ) );
  CHECK_GE( selected_n, 0 );
  CHECK_LT( selected_n, 3 );
  rush_recruit_next_immigrant( ss, ts, player, selected_n );
}

wait<> click_purchase( SS& ss, TS& ts, Player& player ) {
  ChoiceConfig config{ .msg = "What would you like to purchase?",
                       .initial_selection = 0 };
  static string const kNone = "none";
  config.options.push_back(
      { .key = kNone, .display_name = "None" } );
  auto& costs = config_harbor.purchases;
  unordered_map<e_unit_type, int> const prices{
      { e_unit_type::artillery, artillery_price( player ) },
      { e_unit_type::caravel, costs.caravel_cost },
      { e_unit_type::merchantman, costs.merchantman_cost },
      { e_unit_type::galleon, costs.galleon_cost },
      { e_unit_type::privateer, costs.privateer_cost },
      { e_unit_type::frigate, costs.frigate_cost },
  };
  auto push = [&]( e_unit_type type ) {
    string const key = string( refl::enum_value_name( type ) );
    UNWRAP_CHECK( price, base::lookup( prices, type ) );
    string const display_name = fmt::format(
        "{: <20}{: >20}", base::capitalize_initials( key ),
        fmt::format( "(Cost: {})", price ) );
    bool const enabled = ( player.money >= price );
    config.options.push_back( { .key          = key,
                                .display_name = display_name,
                                .disabled     = !enabled } );
  };
  push( e_unit_type::artillery );
  push( e_unit_type::caravel );
  push( e_unit_type::merchantman );
  push( e_unit_type::galleon );
  push( e_unit_type::privateer );
  push( e_unit_type::frigate );
  maybe<string> selected_str =
      co_await ts.gui.optional_choice( config );
  if( !selected_str.has_value() ) co_return;
  if( *selected_str == kNone ) co_return;
  UNWRAP_CHECK(
      selected_type,
      refl::enum_from_string<e_unit_type>( *selected_str ) );
  UNWRAP_CHECK( price, base::lookup( prices, selected_type ) );
  player.money -= price;
  CHECK_GE( player.money, 0 );
  create_unit_in_harbor( ss, player, selected_type );
  if( selected_type == e_unit_type::artillery )
    ++player.artillery_purchases;
}

wait<> click_train( SS& ss, TS& ts, Player& player ) {
  ChoiceConfig config{
      .msg =
          "The [Royal University] can provide us with "
          "specialists if we grease the right palms.  Which "
          "skill shall we request?",
      .initial_selection = 0 };
  static string const kNone = "none";
  config.options.push_back(
      { .key = kNone, .display_name = "None" } );
  auto&               costs = config_harbor.train.unit_prices;
  vector<e_unit_type> ordering;
  for( e_unit_type type : refl::enum_values<e_unit_type> )
    if( costs[type].has_value() ) ordering.push_back( type );
  sort( ordering.begin(), ordering.end(),
        [&]( e_unit_type l, e_unit_type r ) {
          UNWRAP_CHECK( cost_l, costs[l] );
          UNWRAP_CHECK( cost_r, costs[r] );
          return cost_l < cost_r;
        } );
  auto push = [&]( e_unit_type type ) {
    UNWRAP_CHECK( price, costs[type] );
    string const key = string( refl::enum_value_name( type ) );
    string const display_name =
        fmt::format( "{: <20}{: >20}",
                     base::capitalize_initials(
                         unit_attr( type ).name_plural ),
                     fmt::format( "(Cost: {})", price ) );
    bool const enabled = ( player.money >= price );
    config.options.push_back( { .key          = key,
                                .display_name = display_name,
                                .disabled     = !enabled } );
  };
  for( e_unit_type type : ordering ) push( type );
  maybe<string> selected_str =
      co_await ts.gui.optional_choice( config );
  if( !selected_str.has_value() ) co_return;
  if( *selected_str == kNone ) co_return;
  UNWRAP_CHECK(
      selected_type,
      refl::enum_from_string<e_unit_type>( *selected_str ) );
  UNWRAP_CHECK( price, costs[selected_type] );
  player.money -= price;
  CHECK_GE( player.money, 0 );
  create_unit_in_harbor( ss, player, selected_type );
}

} // namespace rn
