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
#include "colony-mgr.hpp"
#include "connectivity.hpp"
#include "harbor-units.hpp"
#include "iagent.hpp"
#include "igui.hpp"
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
#include "ss/land-view.rds.hpp"
#include "ss/nation.hpp"
#include "ss/natives.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/revolution.rds.hpp"
#include "ss/terrain.hpp"
#include "ss/turn.rds.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// C++ standard library
#include <numeric>
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
    case e_ref_landing_formation::_1_1_1: {
      static RefLandingForce const f{
        .regular   = 1,
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
    case e_ref_landing_formation::_3_0_0: {
      static RefLandingForce const f{
        .regular   = 3,
        .cavalry   = 0,
        .artillery = 0,
      };
      return f;
    }
  }
}

int colony_defense_metric( SSConst const& ss,
                           Colony const& colony ) {
  int metric = 1;

  // Colony musket contents. The OG doesn't seem to weigh in
  // horses.
  metric += colony.commodities[e_commodity::muskets] / 50;

  // Pull in the units in the cargo of ships as well.
  vector<GenericUnitId> const units =
      units_from_coord_recursive( ss.units, colony.location );
  for( GenericUnitId const generic_id : units ) {
    switch( ss.units.unit_kind( generic_id ) ) {
      case e_unit_kind::euro:
        break;
      case e_unit_kind::native:
        FATAL( "found native unit on colony tile {}",
               colony.location );
    }
    Unit const& unit = ss.units.euro_unit_for( generic_id );
    switch( unit.type() ) {
      case e_unit_type::scout:
      case e_unit_type::seasoned_scout:
        // These technically are military units but the OG does
        // not weight them in.
        break;

      case e_unit_type::soldier:
      case e_unit_type::veteran_soldier:
      case e_unit_type::dragoon:
      case e_unit_type::veteran_dragoon:
        metric += 2;
        break;

      case e_unit_type::continental_army:
      case e_unit_type::continental_cavalry:
      case e_unit_type::damaged_artillery:
        metric += 4;
        break;

      case e_unit_type::artillery:
      case e_unit_type::regular:
      case e_unit_type::cavalry:
        metric += 6;
        break;

      case e_unit_type::caravel:
      case e_unit_type::man_o_war:
      case e_unit_type::frigate:
      case e_unit_type::galleon:
      case e_unit_type::privateer:
      case e_unit_type::merchantman:
      case e_unit_type::elder_statesman:
      case e_unit_type::expert_cotton_planter:
      case e_unit_type::expert_farmer:
      case e_unit_type::expert_fisherman:
      case e_unit_type::expert_fur_trapper:
      case e_unit_type::expert_lumberjack:
      case e_unit_type::expert_ore_miner:
      case e_unit_type::expert_silver_miner:
      case e_unit_type::expert_sugar_planter:
      case e_unit_type::expert_teacher:
      case e_unit_type::expert_tobacco_planter:
      case e_unit_type::firebrand_preacher:
      case e_unit_type::free_colonist:
      case e_unit_type::hardy_colonist:
      case e_unit_type::hardy_pioneer:
      case e_unit_type::indentured_servant:
      case e_unit_type::jesuit_colonist:
      case e_unit_type::jesuit_missionary:
      case e_unit_type::master_blacksmith:
      case e_unit_type::master_carpenter:
      case e_unit_type::master_distiller:
      case e_unit_type::master_fur_trader:
      case e_unit_type::master_gunsmith:
      case e_unit_type::master_tobacconist:
      case e_unit_type::master_weaver:
      case e_unit_type::missionary:
      case e_unit_type::native_convert:
      case e_unit_type::petty_criminal:
      case e_unit_type::pioneer:
      case e_unit_type::seasoned_colonist:
      case e_unit_type::treasure:
      case e_unit_type::veteran_colonist:
      case e_unit_type::wagon_train:
        break;
    }
  }
  return metric;
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
namespace detail {

RefColonySelectionMetrics ref_colony_selection_metrics(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    ColonyId const colony_id ) {
  RefColonySelectionMetrics metrics;
  Colony const& colony = ss.colonies.colony_for( colony_id );
  e_player const colonial_player = colony.player;
  e_nation const nation          = nation_for( colonial_player );
  e_player const ref_player      = ref_player_for( nation );
  metrics.colony_id              = colony_id;
  metrics.defense_strength = colony_defense_metric( ss, colony );
  metrics.barricade        = barricade_for_colony( colony );
  metrics.population       = colony_population( colony );
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
    point const ship_tile = colony.location.to_gfx().moved( d );
    if( !ss.terrain.square_exists( ship_tile ) ) continue;
    MapSquare const& square = ss.terrain.square_at( ship_tile );
    if( !is_water( square ) ) continue;
    bool const ocean_access =
        water_square_has_ocean_access( connectivity, ship_tile );
    if( !ocean_access ) continue;
    vector<point> valid_adjacent;
    valid_adjacent.reserve( 8 );
    for( e_direction const d : enum_values<e_direction> ) {
      point const landing = ship_tile.moved( d );
      if( !ss.terrain.square_exists( landing ) ) continue;
      MapSquare const& square = ss.terrain.square_at( landing );
      if( is_water( square ) ) continue;
      if( !landing.direction_to( colony.location ).has_value() )
        // This should catch squares that are not adjacent to the
        // colony, including the colony square itself.
        continue;
      if( ss.colonies.maybe_from_coord( landing ).has_value() )
        // Should not happen in practice since colonies are not
        // supposed to be founded adjacent to each other. But
        // we'll be defensive here just in case.
        continue;
      if( ss.natives.maybe_dwelling_from_coord( landing )
              .has_value() )
        // In the OG a dwelling will not be unseated in order to
        // land, unlike units.
        continue;
      valid_adjacent.push_back( landing );
    }
    if( valid_adjacent.empty() ) continue;
    vector<RefLandingTile> landings;
    landings.reserve( valid_adjacent.size() );
    for( point const p : valid_adjacent )
      landings.push_back( make_ref_landing_tile( p ) );
    CHECK( !landings.empty() );
    metrics.valid_landings.push_back( RefColonyLandingTiles{
      .ship_tile = make_ref_landing_tile( ship_tile ),
      .landings  = std::move( landings ) } );
  }
  return metrics;
}

// The smaller the score, the more likely to be chosen.
maybe<int> ref_colony_selection_score(
    RefColonySelectionMetrics const& metrics ) {
  if( metrics.valid_landings.empty() ) return nothing;
  // The OG appears to select colonies in a very simplistic way:
  // there are only two buckets, undefended and defended. It
  // prefers undefended when available. Then within each bucket
  // it prefers colonies founded earlier. This seems too simplis-
  // tic, so we'll do a bit better by more properly computing
  // colony defense strength and also preferring colonies with
  // larger populations (all else being equal).
  int const barricade_multiplier =
      static_cast<int>( metrics.barricade ) + 1;
  int const defense_term =
      metrics.defense_strength * barricade_multiplier;
  int const population_term = -metrics.population;

  return defense_term + population_term;
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
    // First priority is to choose colonies with the lowest
    // score. Then for the colonies of equal score, fall back
    // take the one founded earliest. This conforms to the OG. It
    // might seem strange to use colony ID in this way, but this
    // leads to the behavior that the REF tends to prefer the
    // colonies founded earlier, which seems like a good strategy
    // since those are likely to be the most important.
    if( l->score != r->score ) return l->score < r->score;
    return l->metrics.colony_id < r->metrics.colony_id;
  } );
  RefColonyMetricsScored const* const p = sorted[0];
  CHECK( p );
  return p->metrics;
}

maybe<RefColonyLandingTiles> select_ref_landing_tiles(
    RefColonySelectionMetrics const& metrics ) {
  if( metrics.valid_landings.empty() ) return nothing;
  auto sorted = metrics.valid_landings;
  // Here we depart a bit from the OG's behavior. The OG always
  // chooses the tile (adjacent to the colony) with the largest
  // number of surrounding land tiles, regardless of whether
  // those surrounding land tiles are themselves adjacent to the
  // colony. But that is buggy because, depending on land geome-
  // try, it can lead to selecting ship positions that then have
  // nowhere to offload units (i.e. if it selects a water tile
  // where there are no accessible land tiles other than the
  // colony itself). Here is an example of that:
  //
  //                   0 1 2 3 4 5 6 7 8 9
  //                   --------------------+
  //                   L L _ L L L L L L L | 0
  //                   L L _ L L L L L L L | 1
  //                   _ _ _ _ _ _ L L L L | 2
  //                   L L L L C _ L L L L | 3
  //                   L L L L L _ _ _ _ _ | 4
  //                   L L L L L _ L L L L | 5
  //                   L L L L L _ L L L L | 6
  //                   L L L L L _ L L L L | 7
  //
  // With the colony at (4,3) there are five possible ship
  // landing tiles that the OG can choose. For each of those,
  // here are the number of surrounding land tiles:
  //
  //                   0 1 2 3 4 5 6 7 8 9
  //                   --------------------+
  //                   L L _ L L L L L L L | 0
  //                   L L _ L L L L L L L | 1
  //                   _ _ _ 5 5 6 L L L L | 2
  //                   L L L L C 4 L L L L | 3
  //                   L L L L L 5 _ _ _ _ | 4
  //                   L L L L L _ L L L L | 5
  //                   L L L L L _ L L L L | 6
  //                   L L L L L _ L L L L | 7
  //
  // The tile with the largest number is (5,2) with six sur-
  // rounding land tiles. So the OG will land the ship there with
  // no units onboard because it can't then find any land tiles
  // that are adjacent to the ship that are also adjacent to the
  // colony. So the game gets into this bad state where empty
  // ships keep coming and REF units do not get deployed.
  //
  // We of course are not going to reproduce that buggy behavior,
  // but we will nevertheless try to reproduce the intention of
  // the OG but in a non-buggy way. Namely, we will select the
  // ship landing site that has the largest number of eligible
  // landing tiles.
  rg::stable_sort( sorted, []( RefColonyLandingTiles const& l,
                               RefColonyLandingTiles const& r ) {
    return l.landings.size() > r.landings.size();
  } );
  CHECK( !sorted.empty() );
  return sorted[0];
}

void filter_ref_landing_tiles( RefColonyLandingTiles& tiles ) {
  auto& landings = tiles.landings;
  if( landings.empty() ) return;
  // Here we depart a bit from the OG's behavior. The OG will
  // look at all available landing tiles and will select the one
  // with the fewest number of units on it and unload all units
  // there. However, if the tile with the smallest number of
  // units is not unique then it will land units on each of those
  // tiles with that smallest number (which could be zero). To
  // see the problem with this, consider the following. Let's say
  // you have:
  //
  //   * Tiles 1, 2, 3: Four artillery each.
  //   * Tile 4:        Three artillery.
  //
  // then it will unload all REF on tile 4, capturing the three
  // artillery, which is ok.  But
  // then, if you just add one artillery onto tile 4, yielding:
  //
  //   * Tiles 1, 2, 3, 4: Four artillery each.
  //
  // now the behavior completely changes and it will deploy REF
  // to all four tiles and capture all 4*4=16 artillery. So just
  // adding one artillery unit caused the capture count to go
  // from 3 to 16, and caused the number of landing tiles to go
  // from 1 to 4. This discontinuity in behavior is kind of
  // strange, and we won't reproduce it here.
  //
  // What we will do is, if there are any tiles with zero units
  // then we will deploy onto all of them, capturing no units. If
  // there are no empty tiles then we will pick one tile with the
  // smallest number of units and deploy all units there.
  rg::stable_sort( landings, []( RefLandingTile const& l,
                                 RefLandingTile const& r ) {
    return l.captured_units.size() < r.captured_units.size();
  } );
  if( !landings[0].captured_units.empty() ) {
    CHECK_GE( landings.size(), 1u );
    landings.resize( 1 );
    return;
  }
  erase_if( landings, []( RefLandingTile const& tile ) {
    return !tile.captured_units.empty();
  } );
  CHECK( !landings.empty() );
}

// In the OG the game appears to do this by looking at the "last
// visited" status of the tiles around the colony. But since we
// don't really do that in this game, this seems sufficient.
bool is_initial_visit_to_colony(
    SSConst const& ss, RefColonySelectionMetrics const& metrics,
    IVisibility const& ref_viz ) {
  point const tile =
      ss.colonies.colony_for( metrics.colony_id ).location;
  return ref_viz.visible( tile ) == e_tile_visibility::hidden;
}

e_ref_manowar_availability ensure_manowar_availability(
    SSConst const& ss, e_nation const nation ) {
  e_player const ref_player_type = ref_player_for( nation );
  e_player const colonial_player_type =
      colonial_player_for( nation );
  UNWRAP_CHECK_T( Player const& colonial_player,
                  ss.players.players[colonial_player_type] );
  int const man_o_war =
      colonial_player.revolution.expeditionary_force.man_o_war;
  if( man_o_war > 0 )
    // When one is available in stock it can/will always be used,
    // even if there are ships already on the map.
    return e_ref_manowar_availability::available_in_stock;
  for( auto const& [unit_id, p_state] : ss.units.euro_all() ) {
    Unit const& unit = p_state->unit;
    if( unit.player_type() != ref_player_type ) continue;
    if( unit.type() == e_unit_type::man_o_war )
      return e_ref_manowar_availability::available_on_map;
  }
  if( config_revolution.ref_forces.ref_can_spawn_ships )
    return e_ref_manowar_availability::none_but_can_add;
  return e_ref_manowar_availability::none;
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
      { .start= 0, .name=_3_0_0 },
    }},
    {stockade, {
      { .start=30, .name=_2_2_2 },
      { .start=10, .name=_4_1_1 },
      { .start= 8, .name=_3_1_1 },
      { .start= 6, .name=_2_1_1 },
      { .start= 4, .name=_2_1_0 },
      { .start= 2, .name=_2_1_0 },
      { .start= 0, .name=_3_0_0 },
    }},
    {fort, {
      { .start=20, .name=_2_2_2 },
      { .start=10, .name=_4_1_1 },
      { .start= 8, .name=_4_1_1 },
      { .start= 6, .name=_4_1_1 },
      { .start= 4, .name=_2_1_1 },
      { .start= 2, .name=_2_1_0 },
      { .start= 0, .name=_3_0_0 },
    }},
    {fortress, {
      { .start=14, .name=_2_2_2 },
      { .start=10, .name=_4_1_1 },
      { .start= 8, .name=_4_1_1 },
      { .start= 6, .name=_4_1_1 },
      { .start= 4, .name=_4_1_1 },
      { .start= 2, .name=_2_1_1 },
      { .start= 0, .name=_3_0_0 },
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

RefLandingForce allocate_landing_units(
    SSConst const& ss, e_nation const nation,
    e_ref_landing_formation const formation ) {
  e_player const colonial_player_type =
      colonial_player_for( nation );
  UNWRAP_CHECK_T( Player const& colonial_player,
                  ss.players.players[colonial_player_type] );
  auto const& available =
      colonial_player.revolution.expeditionary_force;
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
    SS& ss, e_nation const nation, RefLandingPlan const& plan,
    ColonyId const colony_id ) {
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
  res.colony_id         = colony_id;
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

} // namespace detail

wait<> offboard_ref_units(
    SS& ss, IMapUpdater& map_updater, ILandViewPlane& land_view,
    IAgent& colonial_agent,
    RefLandingUnits const& landing_units ) {
  auto const euro_unit_capture =
      [&]( Unit const& capturer,
           Unit const& captured ) -> wait<> {
    // Bring the capturer to the front here and hold it while the
    // message box pops up otherwise the unit might go behind
    // e.g. an artillery that it is capturing.
    AnimationSequence const seq =
        anim_seq_unit_to_front( capturer.id() );
    // Need to use the "always" variant here because otherwise
    // this animation would be subject to the "show foreign
    // moves" option, which we don't want in this case.
    wait<> const front =
        land_view.animate_always_and_hold( seq );

    string_view const army_type =
        capturer.desc().ship ? "Navy" : "Army";
    string const msg = format( "[{}] captured by the Royal {}!",
                               captured.desc().name, army_type );
    co_await colonial_agent.message_box( "{}", msg );
    UnitOwnershipChanger( ss, captured.id() ).destroy();
  };
  auto const euro_unit_captures =
      [&]( Unit const& capturerer,
           vector<GenericUnitId> const& units ) -> wait<> {
    for( GenericUnitId const generic_id : units ) {
      if( !ss.units.exists( generic_id ) )
        // This can happen when multiple REF units are unloaded
        // onto the same tile with captured units. In that case
        // the units will be captured by the first unloaded REF,
        // so for subsequent landed units they will not exist.
        continue;
      e_unit_kind const kind = ss.units.unit_kind( generic_id );
      switch( kind ) {
        case e_unit_kind::euro: {
          Unit const& unit =
              ss.units.euro_unit_for( generic_id );
          co_await euro_unit_capture( capturerer, unit );
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
      ship_unit,
      landing_units.ship.landing_tile.captured_units );

  // Message.
  Colony const& colony =
      ss.colonies.colony_for( landing_units.colony_id );
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
    // Need to use the "always" variant here because otherwise
    // this animation would be subject to the "show foreign
    // moves" option, which we don't want in this case.
    co_await land_view.animate_always( seq );
    UnitOwnershipChanger( ss, unit_id )
        .change_to_map_non_interactive( map_updater,
                                        landing_tile.tile );
    Unit& unit = ss.units.unit_for( unit_id );
    unit.clear_orders();
    unit.forfeight_mv_points();
    // Destroy any units that are captured and give message.
    co_await euro_unit_captures( unit,
                                 landing_tile.captured_units );
  }
}

// NOTE: we don't need to check for uprising here, since this
// function is only called at the end of a turn after which an
// uprising could have happened. In the OG, the REF can and will
// forfeight even if an REF is possible, it it didn't do one on a
// given turn and the other surrender conditions are met. So in
// other words, we don't need to check if we will be able to do
// another uprising on the next turn.
maybe<e_forfeight_reason> ref_should_forfeight(
    SSConst const& ss, Player const& ref_player ) {
  CHECK( is_ref( ref_player.type ) );
  vector<ColonyId> const ref_colonies =
      ss.colonies.for_player( ref_player.type );
  if( !ref_colonies.empty() )
    // If the REF has captured any colonies, even if they are in-
    // land, then they don't forfeight.
    return nothing;
  unordered_map<UnitId, UnitState::euro const*> const&
      units_all = ss.units.euro_all();
  for( auto const [unit_id, p_euro] : units_all ) {
    Unit const& unit = p_euro->unit;
    if( unit.player_type() != ref_player.type ) continue;
    if( !unit.desc().ship )
      // We have at least one non-ship REF unit, so don't for-
      // feight. Note that this does not include REF units who
      // have not yet been transported to the new world, since
      // those are not yet real units. This means there is a real
      // REF land unit somewhere on the map.
      return nothing;
  }
  e_player const colonial_player_type =
      colonial_player_for( nation_for( ref_player.type ) );
  UNWRAP_CHECK_T( Player const& colonial_player,
                  ss.players.players[colonial_player_type] );
  auto const& stock =
      colonial_player.revolution.expeditionary_force;
  int const total_ships_in_stock = stock.man_o_war;
  int const total_land_units_in_stock =
      stock.regular + stock.cavalry + stock.artillery;
  // The REF has no colonies and no REF units on land. Therefore
  // forfeight depends on how many units there are in stock.
  if( total_land_units_in_stock == 0 )
    // If we have no more land units then it is impossible to
    // continue the war, regardless of the number of ships. NOTE:
    // In a normal game with default rules, this is the only
    // reason that the REF will forfeight in the NG.
    return e_forfeight_reason::no_more_land_units_in_stock;
  // NOTE: By default (as in the OG) ref_can_spawn_ships=true.
  if( !config_revolution.ref_forces.ref_can_spawn_ships &&
      total_ships_in_stock == 0 )
    // We have no more ships with which to transport the re-
    // maining REF land units and we are not going to spawn new
    // ones, so therefore the REF must forfeight.
    return e_forfeight_reason::no_more_ships;
  if( total_ships_in_stock > 0 )
    // We have some land units left, and some ships available to
    // transport them, so don't forfeight.
    return nothing;
  // At this point we have some land units left but no ships to
  // transport them. Because new ships are created every couple
  // of turns after they run out, it is not a deal breaker that
  // there are no ships.
  //
  // NOTE: In the OG it appears that when regulars hit zero (or a
  // low number, and some other conditions are met; it is not
  // clear what the exact behavior is) then the REF forfeights,
  // even if there are other REF land units on the map and/or in
  // europe. We don't reproduce that here because it doesn't seem
  // to make sense and the OG's behavior doesn't seem consistent
  // in that regard.
  return nothing;
}

void do_ref_forfeight( SS& ss, Player& ref_player ) {
  CHECK( is_ref( ref_player.type ) );
  e_player const colonial_player_type =
      colonial_player_for( nation_for( ref_player.type ) );
  UNWRAP_CHECK_T( Player & colonial_player,
                  ss.players.players[colonial_player_type] );

  ref_player.control                = e_player_control::inactive;
  colonial_player.revolution.status = e_revolution_status::won;

  // Destroy all remaining REF units on the map. Under default NG
  // rules, this would be expected to consist only of empty REF
  // Man-o-Wars. However, there could be some land units as well
  // if the regulars_determine_forfeight config flag is set.
  vector<UnitId> ref_units;
  for( auto const& [unit_id, p_state] : ss.units.euro_all() ) {
    Unit const& unit = p_state->unit;
    if( unit.player_type() != ref_player.type ) continue;
    ref_units.push_back( unit_id );
  }
  destroy_units( ss, ref_units );
}

wait<> ref_forfeight_ui_routine( SSConst const&, IGui& gui,
                                 Player const& /*ref_player*/ ) {
  co_await gui.message_box(
      "The REF has forfeighted! (TODO: add more here)" );
  co_await gui.message_box( "(game end routine, e.g. scoring)" );
}

int percent_ref_owned_population( SSConst const& ss,
                                  Player const& ref_player ) {
  CHECK( is_ref( ref_player.type ) );
  e_player const colonial_player_type =
      colonial_player_for( nation_for( ref_player.type ) );
  int const total_ref_population =
      total_colonies_population( ss, ref_player.type );
  int const total_colonial_population =
      total_colonies_population( ss, colonial_player_type );
  // If this were zero then the player would have zero colonies
  // which means zero coastal colonies and show we should have
  // caught it above.
  CHECK_GT( total_colonial_population, 0 );
  double const ref_owned_percent =
      double( total_ref_population ) /
      ( total_ref_population + total_colonial_population );
  return clamp( static_cast<int>( ref_owned_percent * 100 ), 0,
                100 );
}

maybe<e_ref_win_reason> ref_should_win(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    Player const& ref_player ) {
  CHECK( is_ref( ref_player.type ) );
  e_player const colonial_player_type =
      colonial_player_for( nation_for( ref_player.type ) );
  vector<ColonyId> const coastal = find_coastal_colonies(
      ss, connectivity, colonial_player_type );
  if( coastal.empty() )
    // This condition happens if either the player has never had
    // any port colonies since declaration (in which case they
    // lose immediately) or if the REF has captured them.
    return e_ref_win_reason::port_colonies_captured;
  if( percent_ref_owned_population( ss, ref_player ) >= 90 )
    return e_ref_win_reason::ref_controls_90_percent_population;
  return nothing;
}

void do_ref_win( SS& ss, Player const& ref_player ) {
  CHECK( is_ref( ref_player.type ) );
  e_player const colonial_player_type =
      colonial_player_for( nation_for( ref_player.type ) );
  UNWRAP_CHECK_T( Player & colonial_player,
                  ss.players.players[colonial_player_type] );

  colonial_player.revolution.status = e_revolution_status::lost;
}

wait<> ref_win_ui_routine( SSConst const&, IGui& gui,
                           Player const& /*ref_player*/,
                           e_ref_win_reason const /*reason*/ ) {
  co_await gui.message_box( "The REF has won." );
  co_await gui.message_box( "(game end routine, e.g. scoring)" );
}

int move_ref_harbor_ships_to_stock( SS& ss,
                                    Player& ref_player ) {
  CHECK( is_ref( ref_player.type ) );
  vector<UnitId> in_port = harbor_units_in_port(
      as_const( ss.units ), ref_player.type );
  erase_if( in_port, [&]( UnitId const unit_id ) {
    Unit const& unit = ss.units.unit_for( unit_id );
    CHECK_EQ( unit.player_type(), ref_player.type );
    return unit.type() != e_unit_type::man_o_war;
  } );
  // At this point we have all the man-o-wars in port.
  e_player const colonial_player_type =
      colonial_player_for( nation_for( ref_player.type ) );
  UNWRAP_CHECK_T( Player & colonial_player,
                  ss.players.players[colonial_player_type] );
  int const count = in_port.size();
  colonial_player.revolution.expeditionary_force.man_o_war +=
      count;
  destroy_units( ss, in_port );
  // This is so that the harbor view doesn't crash if we try to
  // open it as the REF player since one of the ships in port
  // will have been selected.
  update_harbor_selected_unit( ss, ref_player );
  return count;
}

// This is the full routine that creates the deployed REF troops,
// using the other functions in this module, which are exposed in
// the API so that they can be tested.
maybe<RefLandingUnits> produce_REF_landing_units(
    SS& ss, TerrainConnectivity const& connectivity,
    e_nation const nation ) {
  using namespace ::rn::detail;
  e_player const ref_player_type = ref_player_for( nation );
  e_player const colonial_player_type =
      colonial_player_for( nation );
  UNWRAP_CHECK_T( Player & colonial_player,
                  ss.players.players[colonial_player_type] );
  vector<ColonyId> const colonies =
      ss.colonies.for_player( colonial_player_type );
  vector<RefColonyMetricsScored> scored;
  scored.reserve( scored.size() );
  for( ColonyId const colony_id : colonies ) {
    RefColonySelectionMetrics metrics =
        ref_colony_selection_metrics( ss.as_const, connectivity,
                                      colony_id );
    auto const score = ref_colony_selection_score( metrics );
    if( !score.has_value() ) continue;
    scored.push_back( RefColonyMetricsScored{
      .metrics = std::move( metrics ), .score = *score } );
  }
  auto const metrics = select_ref_landing_colony( scored );
  if( !metrics.has_value() || metrics->valid_landings.empty() ) {
    // Either no coastal colonies or there are coastal colonies
    // but no available spots for the REF to land around them.
    // The former means that the REF has won, and that will be
    // detected later in the REF's turn. The latter should not
    // typically happen with the standard map generator, but
    // could happen with custom maps, e.g. if there is a colony
    // on a small island. In this case, the game just kind of
    // stalls since there is no way for the REF to deploy, and
    // the player can't add/remove colonies. The standard map
    // generator should prevent these scenarios, but we have to
    // handle them in case of custom maps.
    return nothing;
  }
  auto const landing_tiles = [&] {
    auto unfiltered_landing_tiles =
        select_ref_landing_tiles( *metrics );
    // This should not trigger because we checked that there are
    // valid_landings above.
    UNWRAP_CHECK( res, unfiltered_landing_tiles );
    filter_ref_landing_tiles( res );
    return res;
  }();
  auto const ref_viz =
      create_visibility_for( ss.as_const, ref_player_type );
  CHECK( ref_viz && ref_viz->player().has_value() &&
         is_ref( *ref_viz->player() ) );
  bool const initial_visit_to_colony =
      is_initial_visit_to_colony( ss.as_const, *metrics,
                                  *ref_viz );
  e_ref_landing_formation const formation =
      select_ref_formation( *metrics, initial_visit_to_colony );
  RefLandingForce const force =
      allocate_landing_units( ss.as_const, nation, formation );
  if( force.regular + force.cavalry + force.artillery == 0 )
    // No more units to deploy.
    return nothing;
  // NOTE: the following function may spawn a new Man-o-War to be
  // in stock if there are none, but we only want to do that if
  // there are units to transport, hence we check the total force
  // first above.
  e_ref_manowar_availability const manowar_availability =
      ensure_manowar_availability( ss, nation );
  switch( manowar_availability ) {
    case e_ref_manowar_availability::none:
      // No more left, and we are not adding any, so there is
      // nothing we can do here. Given that the REF hasn't sur-
      // rendered yet, it means that there are still REF units or
      // colonies on the map. But no new REF units can be deliv-
      // ered.
      return nothing;
    case e_ref_manowar_availability::none_but_can_add:
      ++colonial_player.revolution.expeditionary_force.man_o_war;
      // There were no more men-o-war left, but we've just added
      // one. Like the OG, when this happens, we wait one turn
      // before using it.
      return nothing;
    case e_ref_manowar_availability::available_on_map:
      // Wait for the ships on the map to return to europe.
      return nothing;
    case e_ref_manowar_availability::available_in_stock:
      break;
  }
  RefLandingPlan const landing_plan =
      make_ref_landing_plan( landing_tiles, force );
  return create_ref_landing_units( ss, nation, landing_plan,
                                   metrics->colony_id );
}

} // namespace rn
