/****************************************************************
**ref.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-18.
*
* Description: Handles the REF forces.
*
*****************************************************************/
#include "ref.hpp"

// Revolution Now
#include "anim-builders.hpp"
#include "co-wait.hpp"
#include "colony-buildings.hpp"
#include "connectivity.hpp"
#include "iagent.hpp"
#include "land-view.hpp"
#include "map-square.hpp"
#include "ref.rds.hpp"
#include "unit-mgr.hpp"
#include "unit-ownership.hpp"
#include "visibility.hpp"

// config
#include "config/revolution.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/nation.hpp"
#include "ss/natives.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/revolution.rds.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// C++ standard library
#include <ranges>

namespace rg = std::ranges;

namespace rn {

namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;
using ::gfx::e_direction;
using ::gfx::point;
using ::refl::enum_map;
using ::refl::enum_values;

struct Bucket {
  int start                    = {};
  e_ref_landing_formation name = {};

  // For enum_map.
  [[maybe_unused]] bool operator==( Bucket const& ) const =
      default;
};

maybe<e_expeditionary_force_type> from_unit_type(
    e_unit_type const unit_type ) {
  switch( unit_type ) {
    case e_unit_type::regular:
      return e_expeditionary_force_type::regular;
    case e_unit_type::cavalry:
      return e_expeditionary_force_type::cavalry;
    case e_unit_type::artillery:
      return e_expeditionary_force_type::artillery;
    case e_unit_type::man_o_war:
      return e_expeditionary_force_type::man_o_war;
    default:
      return nothing;
  }
}

RefLandingForce const& formation_unit_counts(
    e_ref_landing_formation const formation ) {
  switch( formation ) {
    case e_ref_landing_formation::_2_2_2: {
      static RefLandingForce const f{
        .regular   = 2,
        .cavalry   = 2,
        .artillery = 2,
      };
      return f;
    }
    case e_ref_landing_formation::_4_1_1: {
      static RefLandingForce const f{
        .regular   = 4,
        .cavalry   = 1,
        .artillery = 1,
      };
      return f;
    }
    case e_ref_landing_formation::_3_1_1: {
      static RefLandingForce const f{
        .regular   = 3,
        .cavalry   = 1,
        .artillery = 1,
      };
      return f;
    }
    case e_ref_landing_formation::_2_1_1: {
      static RefLandingForce const f{
        .regular   = 2,
        .cavalry   = 1,
        .artillery = 1,
      };
      return f;
    }
    case e_ref_landing_formation::_2_1_0: {
      static RefLandingForce const f{
        .regular   = 2,
        .cavalry   = 1,
        .artillery = 0,
      };
      return f;
    }
    case e_ref_landing_formation::_1_1_1: {
      static RefLandingForce const f{
        .regular   = 1,
        .cavalry   = 1,
        .artillery = 1,
      };
      return f;
    }
  }
}

} // namespace

/****************************************************************
** Royal Money.
*****************************************************************/
RoyalMoneyChange evolved_royal_money(
    e_difficulty const difficulty, int const royal_money ) {
  RoyalMoneyChange res;
  res.old_value         = royal_money;
  res.per_turn_increase = config_revolution.royal_money
                              .constant_per_turn[difficulty];
  res.new_value = res.old_value + res.per_turn_increase;
  int const threshold =
      config_revolution.royal_money.threshold_for_new_ref;
  if( res.new_value >= threshold ) {
    res.new_value -= threshold;
    res.new_unit_produced = true;
    res.amount_subtracted = threshold;
  }
  // NOTE: the new_value may still be larger than `threshold`
  // here, since we produce at most one new unit per turn.
  return res;
}

void apply_royal_money_change( Player& player,
                               RoyalMoneyChange const& change ) {
  player.royal_money = change.new_value;
}

/****************************************************************
** REF Unit Addition.
*****************************************************************/
e_expeditionary_force_type select_next_ref_type(
    ExpeditionaryForce const& force ) {
  using enum e_expeditionary_force_type;
  int const total = force.regular + force.cavalry +
                    force.artillery + force.man_o_war;
  if( total == 0 ) return man_o_war;
  enum_map<e_expeditionary_force_type, double> metrics;
  CHECK_GT( total, 0 );
  // Get a positive metric that measures how much in need one
  // unit type is relative to the others in order for the overall
  // distribution to look like the target.
  auto const metric = [&]( double& dst, int const n,
                           int const target ) {
    CHECK_GE( n, 0 );
    // Should have been verified when loading configs.
    CHECK_GE( target, 0 );
    double const ratio = double( n ) / total;
    dst                = target / ratio;
  };
  metric(
      metrics[regular], force.regular,
      config_revolution.ref_forces.target_ratios.ratio[regular]
          .percent );
  metric(
      metrics[cavalry], force.cavalry,
      config_revolution.ref_forces.target_ratios.ratio[cavalry]
          .percent );
  metric(
      metrics[artillery], force.artillery,
      config_revolution.ref_forces.target_ratios.ratio[artillery]
          .percent );
  metric(
      metrics[man_o_war], force.man_o_war,
      config_revolution.ref_forces.target_ratios.ratio[man_o_war]
          .percent );

  vector<pair<e_expeditionary_force_type, double>>
      sorted_metrics = metrics;
  // Sort largest to smallest.
  rg::stable_sort( sorted_metrics,
                   []( auto const& l, auto const& r ) {
                     return r.second < l.second;
                   } );

  CHECK( !sorted_metrics.empty() );
  auto const& [type, _] = sorted_metrics[0];
  return type;
}

void add_ref_unit( ExpeditionaryForce& force,
                   e_expeditionary_force_type const type ) {
  using enum e_expeditionary_force_type;
  switch( type ) {
    case regular: {
      ++force.regular;
      break;
    }
    case cavalry: {
      ++force.cavalry;
      break;
    }
    case artillery: {
      ++force.artillery;
      break;
    }
    case man_o_war: {
      ++force.man_o_war;
      break;
    }
  }
}

e_unit_type ref_unit_to_unit_type(
    e_expeditionary_force_type const type ) {
  using enum e_expeditionary_force_type;
  switch( type ) {
    case regular:
      return e_unit_type::regular;
    case cavalry:
      return e_unit_type::cavalry;
    case artillery:
      return e_unit_type::artillery;
    case man_o_war:
      return e_unit_type::man_o_war;
  }
}

wait<> add_ref_unit_ui_seq(
    IAgent& agent, e_expeditionary_force_type const ref_type ) {
  e_unit_type const unit_type =
      ref_unit_to_unit_type( ref_type );
  auto const& plural_name =
      config_unit_type.composition.unit_types[unit_type]
          .name_plural;
  string const msg = format(
      "The King has announced an increase to the Royal military "
      "budget. [{}] have been added to the Royal Expeditionary "
      "Force, causing alarm among colonists.",
      plural_name );
  co_await agent.signal( signal::RefUnitAdded{}, msg );
}

/****************************************************************
** REF Unit Deployment.
*****************************************************************/
vector<ColonyId> find_coastal_colonies(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    e_player const player ) {
  vector<ColonyId> colonies = ss.colonies.for_player( player );
  rg::sort( colonies );
  erase_if( colonies, [&]( ColonyId const colony_id ) {
    Colony const& colony = ss.colonies.colony_for( colony_id );
    bool const coastal   = colony_has_ocean_access(
        ss, connectivity, colony.location );
    return !coastal;
  } );
  return colonies;
}

RefColonySelectionMetrics ref_colony_selection_metrics(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    ColonyId const colony_id ) {
  RefColonySelectionMetrics metrics;
  Colony const& colony = ss.colonies.colony_for( colony_id );
  e_player const colonial_player = colony.player;
  e_nation const nation          = nation_for( colonial_player );
  e_player const ref_player      = ref_player_for( nation );
  metrics.colony_id              = colony_id;
  // TODO
  metrics.defense_strength = 1;
  metrics.barricade        = barricade_for_colony( colony );
  auto const make_ref_landing_tile = [&]( point const tile ) {
    auto const& units = ss.units.from_coord( tile );
    vector<GenericUnitId> captured;
    captured.reserve( units.size() );
    for( GenericUnitId const generic_id : units ) {
      if( ss.units.unit_kind( generic_id ) ==
          e_unit_kind::euro ) {
        Unit const& unit = ss.units.euro_unit_for( generic_id );
        if( unit.player_type() == ref_player ) continue;
      }
      captured.push_back( generic_id );
    }
    return RefLandingTile{
      .tile = tile, .captured_units = std::move( captured ) };
  };
  for( e_direction const d : enum_values<e_direction> ) {
    point const moved = colony.location.to_gfx().moved( d );
    if( !ss.terrain.square_exists( moved ) ) continue;
    MapSquare const& square = ss.terrain.square_at( moved );
    if( !is_water( square ) ) continue;
    bool const eligible =
        water_square_has_ocean_access( connectivity, moved );
    if( !eligible ) continue;
    vector<point> valid_adjacent;
    valid_adjacent.reserve( 8 );
    for( e_direction const d : enum_values<e_direction> ) {
      point const moved_again = moved.moved( d );
      if( !ss.terrain.square_exists( moved ) ) continue;
      MapSquare const& square =
          ss.terrain.square_at( moved_again );
      if( is_water( square ) ) continue;
      if( !moved_again.direction_to( colony.location )
               .has_value() )
        continue;
      if( ss.colonies.maybe_from_coord( moved_again )
              .has_value() )
        // Should not happen in practice since colonies are not
        // supposed to be founded adjacent to each other. But
        // we'll be defensive here just in case.
        continue;
      if( ss.natives.maybe_dwelling_from_coord( moved_again )
              .has_value() )
        // In the OG a dwelling will not be unseated in order to
        // land, unlike units.
        continue;
      valid_adjacent.push_back( moved_again );
    }
    if( valid_adjacent.empty() ) continue;
    vector<RefLandingTile> landings;
    landings.reserve( valid_adjacent.size() );
    for( point const p : valid_adjacent )
      landings.push_back( make_ref_landing_tile( p ) );
    CHECK( !landings.empty() );
    metrics.eligible_landings.push_back( RefColonyLandingTiles{
      .ship_tile = make_ref_landing_tile( moved ),
      .landings  = std::move( landings ) } );
  }
  // TODO
  metrics.num_colonies_on_continent = 1;
  // TODO
  metrics.distance_isolation = 1;
  return metrics;
}

maybe<int> ref_colony_selection_score(
    RefColonySelectionMetrics const& metrics ) {
  if( metrics.eligible_landings.empty() ) return nothing;
  // TODO
  return 1;
}

maybe<RefColonySelectionMetrics const&>
select_ref_landing_colony(
    vector<RefColonyMetricsScored> const& choices ) {
  if( choices.empty() ) return nothing;
  vector<RefColonyMetricsScored const*> sorted;
  sorted.reserve( choices.size() );
  for( auto const& choice : choices )
    sorted.push_back( &choice );
  rg::sort( sorted, []( RefColonyMetricsScored const* const l,
                        RefColonyMetricsScored const* const r ) {
    return l->score > r->score;
  } );
  RefColonyMetricsScored const* const p = sorted[0];
  CHECK( p );
  return p->metrics;
}

maybe<RefColonyLandingTiles const&> select_ref_landing_tiles(
    RefColonySelectionMetrics const& metrics
        ATTR_LIFETIMEBOUND ) {
  if( metrics.eligible_landings.empty() ) return nothing;
  // TODO
  return metrics.eligible_landings[0];
}

// In the OG the game appears to do this by looking at the "last
// visited" status of the tiles around the colony. But since we
// don't really do that in this game, this seems sufficient.
bool is_initial_visit_to_colony(
    SSConst const& ss, RefColonySelectionMetrics const& metrics,
    IVisibility const& ref_viz ) {
  point const tile =
      ss.colonies.colony_for( metrics.colony_id ).location;
  return ref_viz.visible( tile ) != e_tile_visibility::hidden;
}

e_ref_landing_formation select_ref_formation(
    RefColonySelectionMetrics const& metrics,
    bool const initial_visit_to_colony ) {
  using enum e_colony_barricade_type;
  using enum e_ref_landing_formation;
  using BucketsMap =
      enum_map<e_colony_barricade_type, vector<Bucket>>;
  static BucketsMap BUCKETS{
    // clang-format off
    {none, {
      { .start=30, .name=_2_2_2 },
      { .start=10, .name=_4_1_1 },
      { .start= 8, .name=_3_1_1 },
      { .start= 6, .name=_2_1_1 },
      { .start= 4, .name=_2_1_0 },
      { .start= 2, .name=_2_1_0 },
      { .start= 0, .name=_2_1_0 },
    }},
    {stockade, {
      { .start=30, .name=_2_2_2 },
      { .start=10, .name=_4_1_1 },
      { .start= 8, .name=_3_1_1 },
      { .start= 6, .name=_2_1_1 },
      { .start= 4, .name=_2_1_0 },
      { .start= 2, .name=_2_1_0 },
      { .start= 0, .name=_2_1_0 },
    }},
    {fort, {
      { .start=20, .name=_2_2_2 },
      { .start=10, .name=_4_1_1 },
      { .start= 8, .name=_4_1_1 },
      { .start= 6, .name=_4_1_1 },
      { .start= 4, .name=_2_1_1 },
      { .start= 2, .name=_2_1_0 },
      { .start= 0, .name=_2_1_0 },
    }},
    {fortress, {
      { .start=14, .name=_2_2_2 },
      { .start=10, .name=_4_1_1 },
      { .start= 8, .name=_4_1_1 },
      { .start= 6, .name=_4_1_1 },
      { .start= 4, .name=_4_1_1 },
      { .start= 2, .name=_2_1_1 },
      { .start= 0, .name=_2_1_0 },
    }},
    // clang-format on
  };
  e_ref_landing_formation const canonical = [&] {
    auto const& buckets = BUCKETS[metrics.barricade];
    for( auto const& [start, name] : buckets )
      if( metrics.defense_strength >= start ) return name;
    SHOULD_NOT_BE_HERE;
  }();
  if( canonical == _2_1_0 && !initial_visit_to_colony )
    return e_ref_landing_formation::_1_1_1;
  return canonical;
}

maybe<RefLandingForce> select_landing_units(
    SSConst const& ss, e_nation const nation,
    e_ref_landing_formation const formation ) {
  e_player const colonial_player_type =
      colonial_player_for( nation );
  UNWRAP_CHECK_T( Player const& colonial_player,
                  ss.players.players[colonial_player_type] );
  auto const& available =
      colonial_player.revolution.expeditionary_force;
  if( !available.man_o_war ) return nothing;
  RefLandingForce const& target_unit_counts =
      formation_unit_counts( formation );
  RefLandingForce res = target_unit_counts;
  RefLandingForce deficits;
  int total_deficit = 0;
  if( int const deficit = res.regular - available.regular;
      deficit > 0 ) {
    deficits.regular += deficit;
    total_deficit += deficit;
    res.regular = available.regular;
  }
  if( int const deficit = res.cavalry - available.cavalry;
      deficit > 0 ) {
    deficits.cavalry += deficit;
    total_deficit += deficit;
    res.cavalry = available.cavalry;
  }
  if( int const deficit = res.artillery - available.artillery;
      deficit > 0 ) {
    deficits.artillery += deficit;
    total_deficit += deficit;
    res.artillery = available.artillery;
  }
  if( total_deficit > 0 && deficits.regular == 0 ) {
    int const remaining = available.regular - res.regular;
    int const taken     = std::min( total_deficit, remaining );
    deficits.regular -= taken;
    total_deficit -= taken;
    res.regular += taken;
  }
  if( total_deficit > 0 && deficits.cavalry == 0 ) {
    int const remaining = available.cavalry - res.cavalry;
    int const taken     = std::min( total_deficit, remaining );
    deficits.cavalry -= taken;
    total_deficit -= taken;
    res.cavalry += taken;
  }
  if( total_deficit > 0 && deficits.artillery == 0 ) {
    int const remaining = available.artillery - res.artillery;
    int const taken     = std::min( total_deficit, remaining );
    deficits.artillery -= taken;
    total_deficit -= taken;
    res.artillery += taken;
  }
  return res;
}

RefLandingPlan make_ref_landing_plan(
    RefColonyLandingTiles const& landing_tiles,
    RefLandingForce const& force ) {
  RefLandingPlan res;
  CHECK( !landing_tiles.landings.empty() );
  res.ship_tile          = landing_tiles.ship_tile;
  int idx                = 0;
  auto const advance_idx = [&] {
    ++idx;
    idx %= landing_tiles.landings.size();
  };
  for( int i = 0; i < force.regular; ++i ) {
    CHECK( idx >= 0 );
    CHECK( idx < ssize( landing_tiles.landings ) );
    res.landing_units.push_back( pair{
      e_unit_type::regular, landing_tiles.landings[idx] } );
    advance_idx();
  }
  for( int i = 0; i < force.cavalry; ++i ) {
    CHECK( idx >= 0 );
    CHECK( idx < ssize( landing_tiles.landings ) );
    res.landing_units.push_back( pair{
      e_unit_type::cavalry, landing_tiles.landings[idx] } );
    advance_idx();
  }
  for( int i = 0; i < force.artillery; ++i ) {
    CHECK( idx >= 0 );
    CHECK( idx < ssize( landing_tiles.landings ) );
    res.landing_units.push_back( pair{
      e_unit_type::artillery, landing_tiles.landings[idx] } );
    advance_idx();
  }
  return res;
}

RefLandingUnits create_ref_landing_units(
    SS& ss, e_nation const nation, RefLandingPlan const& plan ) {
  RefLandingUnits res;
  e_player const ref_player_type = ref_player_for( nation );
  e_player const colonist_player_type =
      colonial_player_for( nation );
  UNWRAP_CHECK_T( Player & colonial_player,
                  ss.players.players[colonist_player_type] );
  UNWRAP_CHECK_T( Player const& ref_player,
                  ss.players.players[ref_player_type] );
  res.ship.unit_id      = create_free_unit( ss.units, ref_player,
                                            e_unit_type::man_o_war );
  res.ship.landing_tile = plan.ship_tile;
  auto const decrement_unit_type =
      [&]( e_unit_type const unit_type ) {
        auto const decrement = [&]( int& n ) {
          --n;
          CHECK_GE( n, 0 );
        };
        auto& force =
            colonial_player.revolution.expeditionary_force;
        UNWRAP_CHECK_T(
            e_expeditionary_force_type const exp_type,
            from_unit_type( unit_type ) );
        switch( exp_type ) {
          using enum e_expeditionary_force_type;
          case regular:
            decrement( force.regular );
            break;
          case cavalry:
            decrement( force.cavalry );
            break;
          case artillery:
            decrement( force.artillery );
            break;
          case man_o_war:
            decrement( force.man_o_war );
            break;
        }
      };
  decrement_unit_type( e_unit_type::man_o_war );
  for( auto const& [unit_type, landing_tile] :
       plan.landing_units ) {
    UnitId const held_id =
        create_free_unit( ss.units, ref_player, unit_type );
    decrement_unit_type( unit_type );
    UnitOwnershipChanger( ss, held_id )
        .change_to_cargo( res.ship.unit_id,
                          /*starting_slot=*/0 );
    res.landed_units.push_back( RefLandedUnit{
      .unit_id = held_id, .landing_tile = landing_tile } );
  }
  return res;
}

wait<> offboard_ref_units( SS& ss, IMapUpdater& map_updater,
                           ILandViewPlane& land_view,
                           IAgent& colonial_agent,
                           RefLandingUnits const& landing_units,
                           ColonyId const colony_id ) {
  auto const euro_unit_capture =
      [&]( Unit const& unit,
           string_view const capturerer ) -> wait<> {
    string const msg = format( "[{}] captured by the Royal {}!",
                               unit.desc().name, capturerer );
    co_await colonial_agent.message_box( "{}", msg );
    UnitOwnershipChanger( ss, unit.id() ).destroy();
  };
  auto const euro_unit_captures =
      [&]( vector<GenericUnitId> const& units,
           string_view const capturerer ) -> wait<> {
    for( GenericUnitId const generic_id : units ) {
      e_unit_kind const kind = ss.units.unit_kind( generic_id );
      switch( kind ) {
        case e_unit_kind::euro: {
          Unit const& unit =
              ss.units.euro_unit_for( generic_id );
          co_await euro_unit_capture( unit, capturerer );
          break;
        }
        case e_unit_kind::native: {
          NativeUnit const& unit =
              ss.units.native_unit_for( generic_id );
          // No need to show a message here, the OG doesn't.
          NativeUnitOwnershipChanger( ss, unit.id ).destroy();
          break;
        }
      }
    }
  };

  // Ship.
  point const ship_tile = landing_units.ship.landing_tile.tile;
  UnitOwnershipChanger( ss, landing_units.ship.unit_id )
      .change_to_map_non_interactive( map_updater, ship_tile );
  Unit& ship_unit =
      ss.units.unit_for( landing_units.ship.unit_id );
  ship_unit.clear_orders();
  ship_unit.forfeight_mv_points();
  co_await euro_unit_captures(
      landing_units.ship.landing_tile.captured_units, "Navy" );

  // Message.
  Colony const& colony = ss.colonies.colony_for( colony_id );
  co_await land_view.ensure_visible( ship_tile );
  co_await colonial_agent.message_box(
      "Royal Expeditionary Force lands near [{}]!",
      colony.name );

  // Cargo units.
  for( auto const& [unit_id, landing_tile] :
       landing_units.landed_units ) {
    UNWRAP_CHECK_T(
        e_direction const direction,
        ship_tile.direction_to( landing_tile.tile ) );
    AnimationSequence const seq = anim_seq_for_offboard_ref_unit(
        ss.as_const, landing_units.ship.unit_id, unit_id,
        direction );
    co_await land_view.animate_always( seq );
    UnitOwnershipChanger( ss, unit_id )
        .change_to_map_non_interactive( map_updater,
                                        landing_tile.tile );
    Unit& unit = ss.units.unit_for( unit_id );
    unit.clear_orders();
    unit.forfeight_mv_points();
    // TODO: bring the unit to the front here and hold it while
    // the message box pops up otherwise the unit might go behind
    // e.g. an artillery that it is capturing.

    // Destroy any units that are captured and give message.
    co_await euro_unit_captures( landing_tile.captured_units,
                                 "Army" );
  }
}

} // namespace rn
