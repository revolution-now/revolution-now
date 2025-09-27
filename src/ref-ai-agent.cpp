/****************************************************************
**ref-ai-agent.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-07-07.
*
* Description: Implementation of IAgent for AI REF players.
*
*****************************************************************/
#include "ref-ai-agent.hpp"

// Revolution Now
#include "capture-cargo.rds.hpp"
#include "co-wait.hpp"
#include "irand.hpp"
#include "society.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/nation.hpp"
#include "ss/player.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/query-enum.hpp"

// base
#include "base/logger.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::point;
using ::refl::cycle_enum;
using ::refl::enum_count;
using ::refl::enum_values;

}

/****************************************************************
** RefAIAgent::State
*****************************************************************/
struct RefAIAgent::State {
  vector<string> messages;
};

/****************************************************************
** RefAIAgent
*****************************************************************/
RefAIAgent::RefAIAgent( e_player const player, SS& ss,
                        IRand& rand )
  : IAgent( player ),
    ss_( ss ),
    rand_( rand ),
    colonial_player_(
        colonial_player_for( nation_for( player ) ) ),
    state_not_const_safe_( make_unique<State>() ) {
  CHECK( player != colonial_player_ );
}

RefAIAgent::~RefAIAgent() = default;

RefAIAgent::State& RefAIAgent::state() {
  return *state_not_const_safe_;
}

RefAIAgent::State const& RefAIAgent::state() const {
  return *state_not_const_safe_;
}

void RefAIAgent::dump_last_message() const {
  if( state().messages.empty() ) return;
  string_view const class_name = "RefAIAgent";
  lg.debug( "last agent message ({}): {}", class_name,
            state().messages.back() );
}

/****************************************************************
** Signals.
*****************************************************************/
using SignalHandlerT = RefAIAgent;

wait<maybe<int>> RefAIAgent::handle(
    signal::ChooseImmigrant const& ) {
  SHOULD_NOT_BE_HERE;
}

EMPTY_SIGNAL( ColonyDestroyedByNatives );
EMPTY_SIGNAL( ColonyDestroyedByStarvation );
EMPTY_SIGNAL( ColonySignal );
EMPTY_SIGNAL( ColonySignalTransient );
EMPTY_SIGNAL( ForestClearedNearColony );
EMPTY_SIGNAL( ImmigrantArrived );
EMPTY_SIGNAL( NoSpotForShip );
EMPTY_SIGNAL( PioneerExhaustedTools );
EMPTY_SIGNAL( PriceChange );
EMPTY_SIGNAL( RebelSentimentChanged );
EMPTY_SIGNAL( RefUnitAdded );
EMPTY_SIGNAL( ShipFinishedRepairs );
EMPTY_SIGNAL( TaxRateWillChange );
EMPTY_SIGNAL( TeaParty );
EMPTY_SIGNAL( TreasureArrived );
EMPTY_SIGNAL( TribeWipedOut );

/****************************************************************
** Named signals.
*****************************************************************/
wait<> RefAIAgent::message_box( string const& msg ) {
  auto& messages = state().messages;
  if( messages.size() > 1000 )
    messages = vector( messages.end() - 5, messages.end() );
  messages.push_back( msg );
  co_return;
}

wait<e_declare_war_on_natives>
RefAIAgent::meet_tribe_ui_sequence( MeetTribe const&,
                                    point const ) {
  co_return e_declare_war_on_natives::no;
}

wait<> RefAIAgent::show_woodcut( e_woodcut ) { co_return; }

wait<base::heap_value<CapturableCargoItems>>
RefAIAgent::select_commodities_to_capture(
    UnitId const, UnitId const, CapturableCargo const& ) {
  co_return {};
}

wait<> RefAIAgent::notify_captured_cargo( Player const&,
                                          Player const&,
                                          Unit const&,
                                          Commodity const& ) {
  co_return;
}

Player const& RefAIAgent::player() {
  return player_for_player_or_die( as_const( ss_.players ),
                                   player_type() );
}

bool RefAIAgent::human() const { return false; }

wait<> RefAIAgent::pan_tile( point const ) { co_return; }

wait<> RefAIAgent::pan_unit( UnitId const ) { co_return; }

wait<std::string> RefAIAgent::name_new_world() {
  SHOULD_NOT_BE_HERE;
}

wait<ui::e_confirm> RefAIAgent::should_king_transport_treasure(
    std::string const& ) {
  SHOULD_NOT_BE_HERE;
}

wait<chrono::microseconds> RefAIAgent::wait_for(
    chrono::milliseconds const us ) {
  co_return us;
}

wait<ui::e_confirm>
RefAIAgent::should_explore_ancient_burial_mounds() {
  co_return ui::e_confirm::no;
}

command RefAIAgent::ask_orders( UnitId const unit_id ) {
  Unit const& unit = ss_.units.unit_for( unit_id );
  auto const coord = ss_.units.maybe_coord_for( unit_id );
  if( !coord.has_value() ) return command::forfeight{};

  auto const find_random_surrounding =
      [&]( auto const& fn ) -> maybe<e_direction> {
    auto arr = enum_values<e_direction>;
    rand_.shuffle( arr );
    for( e_direction const d : arr ) {
      point const moved = coord->moved( d );
      if( !ss_.terrain.square_exists( moved ) ) continue;
      if( !fn( ss_.terrain.square_at( moved ), moved ) )
        continue;
      return d;
    }
    return nothing;
  };

  auto const find_surrounding =
      [&]( auto const& fn ) -> maybe<e_direction> {
    for( e_direction const d : enum_values<e_direction> ) {
      point const moved = coord->moved( d );
      if( !ss_.terrain.square_exists( moved ) ) continue;
      if( !fn( ss_.terrain.square_at( moved ), moved ) )
        continue;
      return d;
    }
    return nothing;
  };

  auto const d_travel = find_random_surrounding(
      [&]( MapSquare const& square, point const tile ) {
        if( unit.desc().ship &&
            square.surface != e_surface::water )
          return false;
        if( !unit.desc().ship &&
            square.surface == e_surface::water )
          return false;
        auto const society = society_on_square( ss_, tile );
        if( !society.has_value() ) return true;
        SWITCH( *society ) {
          CASE( european ) {
            return european.player == unit.player_type();
          }
          CASE( native ) { return false; }
        }
      } );

  if( !unit.desc().can_attack ) {
    if( d_travel.has_value() ) return command::move{ *d_travel };
    return command::forfeight{};
  }

  auto const d_attack_colony = find_surrounding(
      [&]( MapSquare const& square, point const tile ) {
        if( unit.desc().ship &&
            square.surface != e_surface::water )
          return false;
        if( !unit.desc().ship &&
            square.surface == e_surface::water )
          return false;
        auto const colony_id =
            ss_.colonies.maybe_from_coord( tile );
        if( !colony_id.has_value() ) return false;
        Colony const& colony =
            ss_.colonies.colony_for( *colony_id );
        if( colony.player == colonial_player_ ) return true;
        return false;
      } );

  auto const d_attack_unit = find_surrounding(
      [&]( MapSquare const& square, point const tile ) {
        if( unit.desc().ship &&
            square.surface != e_surface::water )
          return false;
        if( !unit.desc().ship &&
            square.surface == e_surface::water )
          return false;
        auto const society = society_on_square( ss_, tile );
        if( !society.has_value() ) return false;
        SWITCH( *society ) {
          CASE( european ) {
            return european.player == colonial_player_;
          }
          CASE( native ) { return false; }
        }
      } );

  if( d_attack_colony.has_value() )
    return command::move{ *d_attack_colony };
  if( d_attack_unit.has_value() )
    return command::move{ *d_attack_unit };
  if( d_travel.has_value() ) return command::move{ *d_travel };
  return command::forfeight{};
}

wait<ui::e_confirm> RefAIAgent::kiss_pinky_ring( string const&,
                                                 ColonyId,
                                                 e_commodity,
                                                 int const ) {
  co_return ui::e_confirm::yes;
}

wait<ui::e_confirm>
RefAIAgent::attack_with_partial_movement_points( UnitId const ) {
  co_return ui::e_confirm::yes;
}

wait<ui::e_confirm> RefAIAgent::should_attack_natives(
    e_tribe const ) {
  co_return ui::e_confirm::yes;
}

wait<maybe<int>> RefAIAgent::pick_dump_cargo(
    map<int /*slot*/, Commodity> const& ) {
  co_return nothing;
}

wait<e_native_land_grab_result>
RefAIAgent::should_take_native_land(
    string const&,
    refl::enum_map<e_native_land_grab_result, string> const&,
    refl::enum_map<e_native_land_grab_result, bool> const& ) {
  co_return e_native_land_grab_result::cancel;
}

wait<ui::e_confirm> RefAIAgent::confirm_disband_unit(
    UnitId const ) {
  co_return ui::e_confirm::yes;
}

wait<ui::e_confirm> RefAIAgent::confirm_build_inland_colony() {
  co_return ui::e_confirm::no;
}

wait<maybe<std::string>> RefAIAgent::name_colony() {
  co_return nothing;
}

wait<ui::e_confirm> RefAIAgent::should_make_landfall(
    bool const /*some_units_already_moved*/ ) {
  co_return ui::e_confirm::yes;
}

wait<ui::e_confirm> RefAIAgent::should_sail_high_seas() {
  co_return ui::e_confirm::no;
}

EvolveGoto RefAIAgent::evolve_goto( UnitId const ) {
  return EvolveGoto::abort{};
}

} // namespace rn
