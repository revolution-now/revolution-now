/****************************************************************
**fathers.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-26.
*
* Description: Api for querying properties of founding fathers.
*
*****************************************************************/
#include "fathers.hpp"

// Revolution Now
#include "alarm.hpp"
#include "co-wait.hpp"
#include "colony-mgr.hpp"
#include "harbor-units.hpp"
#include "igui.hpp"
#include "imap-updater.hpp"
#include "immigration.hpp"
#include "irand.hpp"
#include "logger.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"

// config
#include "config/fathers.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/natives.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/terrain.hpp"
#include "ss/turn.hpp"
#include "ss/units.hpp"

// gfx
#include "gfx/iter.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

maybe<e_founding_father> pick_next_father_for_type(
    SSConst const& ss, TS& ts, Player const& player,
    e_founding_father_type type ) {
  int const year   = ss.turn.time_point.year;
  auto      weight = [&]( e_founding_father father ) {
    auto& conf = config_fathers.fathers[father];
    if( year < 1600 )
      return conf.weight_1492_1600;
    else if( year < 1700 )
      return conf.weight_1600_1700;
    else
      return conf.weight_1700_plus;
  };
  vector<e_founding_father> available;
  // In practice the maximum weight is 10 and there are 5 fathers
  // per type, so this should be enough.
  available.reserve( 10 * 5 + 1 );
  // Each father has a weight (for a given century) that deter-
  // mines the probability of him getting selected relative to
  // the others in the same category. So what we will do here,
  // since the weights are small integers, is we'll just add a
  // given father to the list a number of times equal to his
  // weight. That way we can choose uniformly over the resulting
  // vector to effectively do a random weighted selection.
  for( e_founding_father father :
       founding_fathers_for_type( type ) ) {
    if( !player.fathers.has[father] ) {
      int const w = weight( father );
      for( int i = 0; i < w; ++i ) available.push_back( father );
    }
  }
  if( available.empty() ) return nothing;
  return ts.rand.pick_one( available );
}

// Ensure that the current pool of founding fathers is populated
// with one father of each type, if available.
void fill_father_selection( SSConst const& ss, TS& ts,
                            Player& player ) {
  for( auto& [type, father] : player.fathers.pool ) {
    if( father.has_value() ) {
      if( player.fathers.has[*father] )
        // This would only happen in cheat mode when giving a fa-
        // ther that is already in the pool.
        father = nothing;
      else
        continue;
    }
    // Could be nothing if there are no fathers left in this cat-
    // egory.
    father = pick_next_father_for_type( ss, ts, player, type );
  }
}

int father_count( Player const& player ) {
  int father_count = 0;
  for( auto& [father, has] : player.fathers.has )
    father_count += has ? 1 : 0;
  return father_count;
}

// A frigate is added to the player's navy.
void john_paul_jones( SS& ss, TS& ts, Player const& player ) {
  maybe<Coord> const loc =
      find_new_world_arrival_square( ss, ts, player,
                                     /*sailed_from=*/nothing );
  if( !loc.has_value() ) {
    lg.error(
        "cannot find a place on the map to put the new "
        "frigate." );
    return;
  }
  create_unit_on_map_non_interactive(
      ss, ts, player, e_unit_type::frigate, *loc );
}

// All currently existing indian converts are changed to free
// colonists.
void bartolome_de_las_casas( SS& ss, Player const& player ) {
  unordered_map<UnitId, EuroUnitState const*> const& units_all =
      ss.units.euro_all();
  auto free_colonist_type = e_unit_type::free_colonist;
  for( auto& [unit_id, state] : units_all ) {
    Unit& unit = ss.units.unit_for( unit_id );
    if( unit.nation() != player.nation ) continue;
    if( unit.type() != e_unit_type::native_convert ) continue;
    // We have a native convert of the appropriate nation.
    unit.change_type( player, free_colonist_type );
  }
}

// All boycotts currently in effect are forgiven without back-
// taxes.
void jakob_fugger( Player& player ) {
  for( auto& [commodity, market_item] :
       player.old_world.market.commodities )
    market_item.boycott = false;
}

// All existing colonies and the area around them become visible
// on the map.
void francisco_de_coronado( SS& ss, TS& ts,
                            Player const& player ) {
  unordered_map<ColonyId, Colony> const& colonies_all =
      ss.colonies.all();
  for( auto& [colony_id, colony] : colonies_all ) {
    Rect const to_reveal =
        Rect::from( colony.location, Delta{ .w = 1, .h = 1 } )
            .with_border_added( 5 );
    for( Rect const sub : gfx::subrects( to_reveal ) ) {
      Coord const tile = sub.upper_left();
      if( !ss.terrain.square_exists( tile ) ) continue;
      ts.map_updater.make_square_visible( tile, player.nation );
    }
  }
}

// La Salle gives all current and future colonies a stockade when
// the population reaches three. This will be checked at the
// start of each turn from here on, but we will make one pass im-
// mediately so that the player sees the stockades appear in the
// same turn as receiving Sieur de La Salle.
void sieur_de_la_salle( SS& ss, Player& player ) {
  unordered_map<ColonyId, Colony> const& colonies_all =
      ss.colonies.all();
  for( auto& [colony_id, _] : colonies_all ) {
    Colony& colony = ss.colonies.colony_for( colony_id );
    if( colony.nation != player.nation ) continue;
    give_stockade_if_needed( player, colony );
  }
}

// All tension levels between you and the natives are reduced to
// "content," and alarm is generated only half as fast afterward.
void pocahontas( SS& ss, Player const& player ) {
  int const max_new_alarm = max_tribal_alarm_after_pocahontas();
  for( e_tribe tribe : refl::enum_values<e_tribe> ) {
    if( !ss.natives.tribe_exists( tribe ) ) continue;
    Tribe& tribe_obj    = ss.natives.tribe_for( tribe );
    auto&  relationship = tribe_obj.relationship[player.nation];
    if( !relationship.encountered ) continue;
    // If the tribe already has an alarm lower than this then we
    // don't want to raise it.
    relationship.tribal_alarm =
        std::min( relationship.tribal_alarm, max_new_alarm );
  }
}

// No more criminals or servants appear on the docks, and you se-
// lect which immigrant in the recruitment pool will move to the
// docks.
//
// The one-time effect here, as in the OG, is that any criminals
// or servants that are currently in the recruitment pool will be
// removed and re-selected.
void william_brewster( SSConst const& ss, TS& ts,
                       Player& player ) {
  ImmigrationState& immigration_state =
      player.old_world.immigration;
  auto& pool            = immigration_state.immigrants_pool;
  auto  needs_replacing = []( e_unit_type type ) {
    return ( type == e_unit_type::petty_criminal ) ||
           ( type == e_unit_type::indentured_servant );
  };
  for( int i = 0; i < int( pool.size() ); ++i ) {
    if( !needs_replacing( pool[i] ) ) continue;
    e_unit_type const replacement =
        pick_next_unit_for_pool( ts.rand, player, ss.settings );
    CHECK_NEQ( replacement, e_unit_type::petty_criminal );
    CHECK_NEQ( replacement, e_unit_type::indentured_servant );
    take_immigrant_from_pool( immigration_state, i,
                              replacement );
  }
}

} // namespace

/****************************************************************
** e_founding_father
*****************************************************************/
string_view founding_father_name( e_founding_father father ) {
  return config_fathers.fathers[father].name;
}

/****************************************************************
** e_founding_father_type
*****************************************************************/
e_founding_father_type founding_father_type(
    e_founding_father father ) {
  return config_fathers.fathers[father].type;
}

vector<e_founding_father> const& founding_fathers_for_type(
    e_founding_father_type type ) {
  static auto const fathers = [] {
    refl::enum_map<e_founding_father_type,
                   vector<e_founding_father>>
        res;
    for( e_founding_father father :
         refl::enum_values<e_founding_father> )
      res[config_fathers.fathers[father].type].push_back(
          father );
    return res;
  }();
  CHECK( fathers[type].size() == 5 );
  return fathers[type];
}

string_view founding_father_type_name(
    e_founding_father_type type ) {
  return config_fathers.types[type].name;
}

/****************************************************************
** Father Selection
*****************************************************************/
bool has_all_fathers( Player const& player ) {
  return ( father_count( player ) ==
           refl::enum_count<e_founding_father> );
}

maybe<int> bells_needed_for_next_father( SSConst const& ss,
                                         Player const& player ) {
  int const incremental_cost =
      config_fathers.cost_in_bells[ss.settings.difficulty];
  int const count = father_count( player );
  if( count == refl::enum_count<e_founding_father> )
    // All fathers obtained.
    return nothing;
  int const total_cost = ( count + 1 ) * incremental_cost + 1;
  if( count == 0 )
    // First father costs half.
    return total_cost / 2;
  return total_cost;
}

wait<> pick_founding_father_if_needed( SSConst const& ss, TS& ts,
                                       Player& player ) {
  // Make sure that if we're working on someone that we don't al-
  // ready have him. We shouldn't normally encounter this situa-
  // tion, but we'll accomodate it anyway since it could arise
  // using cheat mode, lua console, save file editing while de-
  // bugging, etc.
  if( player.fathers.in_progress.has_value() &&
      player.fathers.has[*player.fathers.in_progress] )
    player.fathers.in_progress = nothing;
  // If we're still in progress then no need to pick a new one.
  if( player.fathers.in_progress.has_value() ) co_return;
  // This is technically optional, but it avoids asking the
  // player to select a founding father on the first turn before
  // they have any colonies producing bells.
  if( player.fathers.bells == 0 ) co_return;
  // At this point we have some bells but we're not working on
  // anyone, so we need to ask the player to pick someone. First
  // make sure the pool is filled (if possible).
  fill_father_selection( ss, ts, player );
  ChoiceConfig config{
      .msg =
          "Which Founding Father shall we appoint as the next "
          "member of the Continental Congress?" };
  for( auto& [type, father] : player.fathers.pool ) {
    if( !father.has_value() ) continue;
    if( player.fathers.has[*father] )
      // This should only happen via cheat mode when adding fa-
      // thers that are already in the pool.
      continue;
    string const label = fmt::format(
        "{} ({} Advisor)", config_fathers.fathers[*father].name,
        config_fathers.types[type].name );
    config.options.push_back(
        ChoiceConfigOption{ .key = fmt::to_string( *father ),
                            .display_name = label } );
  }
  if( config.options.empty() )
    // No founding fathers in the pool.
    co_return;
  string const choice_str =
      co_await ts.gui.required_choice( config );
  UNWRAP_CHECK(
      choice,
      refl::enum_from_string<e_founding_father>( choice_str ) );
  player.fathers.in_progress = choice;
  // We've now started working on someone, so reset the bells. If
  // we don't do this then we can have a situation where a large
  // number of bells accumulate (e.g. because there were no more
  // founding fathers available in the era) and then suddently
  // they become available and the first one is given immediately
  // because of the accumulation of bells, which feels wrong.
  player.fathers.bells = 0;
}

maybe<e_founding_father> check_founding_fathers(
    SSConst const& ss, Player& player ) {
  // If for whatever reason we're not working on someone then
  // we're done.
  if( !player.fathers.in_progress.has_value() ) return nothing;
  maybe<int> const needed =
      bells_needed_for_next_father( ss, player );
  if( !needed.has_value() )
    // We have all fathers.
    return nothing;
  if( player.fathers.bells < *needed ) return nothing;
  // We've got the next father.
  CHECK( player.fathers.in_progress.has_value() );
  e_founding_father const new_father =
      *player.fathers.in_progress;
  CHECK( !player.fathers.has[new_father] );
  player.fathers.has[new_father] = true;
  // The OG does not seem to keep any extra bells modulo the
  // amount needed for the father, but we will do so.
  player.fathers.bells = player.fathers.bells - *needed;
  CHECK_GE( player.fathers.bells, 0 );
  // We will select a new father and repopulate the pool on the
  // next turn.
  player.fathers.in_progress = nothing;
  player.fathers.pool[founding_father_type( new_father )] =
      nothing;
  return new_father;
}

wait<> play_new_father_cut_scene( TS& ts, Player const&,
                                  e_founding_father father ) {
  // TODO: temporary.
  co_await ts.gui.message_box(
      "[{}] has joined the Continental Congress!",
      config_fathers.fathers[father].name );
}

void on_father_received( SS& ss, TS& ts, Player& player,
                         e_founding_father father ) {
  lg.info( "performing one-time effects for {}.", father );
  switch( father ) {
    case e_founding_father::adam_smith:
    case e_founding_father::peter_minuit:
    case e_founding_father::peter_stuyvesant:
    case e_founding_father::jan_de_witt:
    case e_founding_father::ferdinand_magellan:
    case e_founding_father::hernando_de_soto:
    case e_founding_father::henry_hudson:
    case e_founding_father::hernan_cortes:
    case e_founding_father::george_washington:
    case e_founding_father::paul_revere:
    case e_founding_father::francis_drake:
    case e_founding_father::thomas_jefferson:
    case e_founding_father::thomas_paine:
    case e_founding_father::simon_bolivar:
    case e_founding_father::benjamin_franklin:
    case e_founding_father::william_penn:
    case e_founding_father::father_jean_de_brebeuf:
    case e_founding_father::juan_de_sepulveda:
      // The above fathers don't have any one-time effects.
      return;
    case e_founding_father::william_brewster:
      return william_brewster( ss, ts, player );
    case e_founding_father::bartolome_de_las_casas:
      return bartolome_de_las_casas( ss, player );
    case e_founding_father::francisco_de_coronado:
      return francisco_de_coronado( ss, ts, player );
    case e_founding_father::jakob_fugger:
      return jakob_fugger( player );
    case e_founding_father::john_paul_jones:
      return john_paul_jones( ss, ts, player );
    case e_founding_father::pocahontas:
      return pocahontas( ss, player );
    case e_founding_father::sieur_de_la_salle:
      return sieur_de_la_salle( ss, player );
  }
}

} // namespace rn
