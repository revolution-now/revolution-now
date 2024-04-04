/****************************************************************
**colony-evolve.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-04.
*
* Description: Evolves one colony one turn.
*
*****************************************************************/
#include "colony-evolve.hpp"

// Revolution Now
#include "colony-buildings.hpp"
#include "colony-mgr.hpp"
#include "colony.hpp"
#include "commodity.hpp"
#include "construction.hpp"
#include "custom-house.hpp"
#include "depletion.hpp"
#include "fathers.hpp"
#include "irand.hpp"
#include "on-map.hpp"
#include "production.hpp"
#include "promotion.hpp"
#include "sons-of-liberty.hpp"
#include "teaching.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
#include "unit-ownership.hpp"

// config
#include "config/unit-type.hpp"

// ss
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/unit-type.hpp"
#include "ss/units.hpp"

// config
#include "config/colony.rds.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

void check_ran_out_of_raw_materials( ColonyEvolution& ev ) {
  auto check =
      [&]( e_commodity what, e_indoor_job job,
           RawMaterialAndProduct const& raw_and_product ) {
        if( raw_and_product.raw_consumed_theoretical > 0 &&
            raw_and_product.raw_consumed_actual == 0 )
          // This means that the raw good could have had some
          // amount consumed, but none was actually consumed,
          // meaning that there was none produced and none in the
          // warehouse.
          ev.notifications.push_back(
              ColonyNotification::run_out_of_raw_material{
                  .what = what, .job = job } );
      };

  check( e_commodity::sugar, e_indoor_job::rum,
         ev.production.sugar_rum );
  check( e_commodity::tobacco, e_indoor_job::cigars,
         ev.production.tobacco_cigars );
  check( e_commodity::cotton, e_indoor_job::cloth,
         ev.production.cotton_cloth );
  check( e_commodity::furs, e_indoor_job::coats,
         ev.production.fur_coats );
  check( e_commodity::lumber, e_indoor_job::hammers,
         ev.production.lumber_hammers );
  check( e_commodity::ore, e_indoor_job::tools,
         ev.production.ore_tools );
  check( e_commodity::tools, e_indoor_job::muskets,
         ev.production.tools_muskets );
}

// This will check for spoilage given warehouse capacity. If any-
// thing spoils then it will be removed from the commodity store
// and a notification will be returned.
maybe<ColonyNotification::spoilage> check_spoilage(
    Colony& colony ) {
  int const warehouse_capacity =
      colony_warehouse_capacity( colony );

  vector<Commodity>                 spoiled;
  refl::enum_map<e_commodity, int>& commodities =
      colony.commodities;
  for( e_commodity c : refl::enum_values<e_commodity> ) {
    if( !config_colony.warehouses
             .commodities_with_warehouse_limit[c] )
      continue;
    int const max = warehouse_capacity;
    if( commodities[c] > max ) {
      int spoilage = commodities[c] - max;
      spoiled.push_back(
          Commodity{ .type = c, .quantity = spoilage } );
      commodities[c] = max;
    }
  }

  if( spoiled.empty() ) return nothing;
  return ColonyNotification::spoilage{
      .spoiled = std::move( spoiled ) };
}

config::colony::construction_requirements materials_needed(
    Construction const& construction ) {
  switch( construction.to_enum() ) {
    using e = Construction::e;
    case e::building: {
      auto& o = construction.get<Construction::building>();
      return config_colony.requirements_for_building[o.what];
    }
    case e::unit: {
      auto& o = construction.get<Construction::unit>();
      maybe<config::colony::construction_requirements> const&
          materials =
              config_colony.requirements_for_unit[o.type];
      CHECK( materials.has_value(),
             "a colony is constructing unit {}, but that unit "
             "is not buildable.",
             o.type );
      return *materials;
    }
  }
}

void check_create_or_starve_colonist(
    SS& ss, TS& ts, Player const& player, Colony& colony,
    ColonyProduction const& pr, bool& colony_disappeared,
    vector<ColonyNotification>& notifications ) {
  if( pr.food_horses.colonist_starved ) {
    vector<UnitId> const units_in_colony =
        colony_units_all( colony );
    CHECK( !units_in_colony.empty() );
    if( units_in_colony.size() == 1 ) {
      // If we are here then center colony square is not able to
      // produce enough food to support even a single colonist
      // (which can happen on some difficulty levels and on some
      // tiles, such as arctic). The original game will starve
      // (delete) the colonist and delete the colony.
      colony_disappeared = true;
    } else {
      UnitId      unit_id = ts.rand.pick_one( units_in_colony );
      e_unit_type type    = ss.units.unit_for( unit_id ).type();
      UnitOwnershipChanger( ss, unit_id ).destroy();
      notifications.emplace_back(
          ColonyNotification::colonist_starved{ .type = type } );
    }
    // At this point we may as well return because we can't have
    // a new colonist created in the same turn as one starved.
    return;
  }

  // Check for a colonist created. After all is said and done, if
  // the colony will have enough food, then we can potentially
  // produce a new colonist.
  int const current_food = colony.commodities[e_commodity::food];
  int const food_needed_for_creation =
      config_colony.food_for_creating_new_colonist;

  if( current_food >= food_needed_for_creation ) {
    int& current_food = colony.commodities[e_commodity::food];
    current_food -= food_needed_for_creation;
    UnitId unit_id = create_free_unit(
        ss.units, player, e_unit_type::free_colonist );
    UnitOwnershipChanger( ss, unit_id )
        .change_to_map_non_interactive( ts, colony.location );
    notifications.emplace_back(
        ColonyNotification::new_colonist{ .id = unit_id } );

    // One final sanity check.
    CHECK_GE( colony.commodities[e_commodity::food], 0 );
    return;
  }

  // Check if the colony is heading toward starvation and warn
  // the player. The following is a heuristic that seems sensi-
  // ble.
  if( pr.food_horses.food_delta_final < 0 ) {
    int const food_loss = -pr.food_horses.food_delta_final;
    int const food_before_loss = current_food + food_loss;
    CHECK_GT( food_before_loss, current_food );
    double const percent_loss =
        double( food_loss ) / food_before_loss;
    if( percent_loss >= .25 )
      notifications.push_back(
          ColonyNotification::colony_starving{} );
  }
}

void check_construction( SS& ss, TS& ts, Player const& player,
                         Colony& colony, ColonyEvolution& ev ) {
  if( !colony.construction.has_value() ) return;
  Construction const& construction = *colony.construction;

  // First check if it's a building that the colony already has.
  if( auto building =
          construction.get_if<Construction::building>();
      building.has_value() ) {
    if( colony.buildings[building->what] ) {
      // We already have the building, but we will only notify
      // the player (to minimize spam) if there are active car-
      // penter's building.
      if( !colony.indoor_jobs[e_indoor_job::hammers].empty() )
        ev.notifications.emplace_back(
            ColonyNotification::construction_already_finished{
                .what = construction } );
      return;
    }
  }

  auto const requirements = materials_needed( construction );

  if( colony_population( colony ) <
      requirements.minimum_population ) {
    ev.notifications.emplace_back(
        ColonyNotification::construction_lacking_population{
            .what = construction,
            .required_population =
                requirements.minimum_population } );
    return;
  }

  if( requirements.required_building.has_value() &&
      !colony_has_building_level(
          colony, *requirements.required_building ) ) {
    ev.notifications.emplace_back(
        ColonyNotification::construction_lacking_building{
            .what = construction,
            .required_building =
                *requirements.required_building } );
    return;
  }

  if( colony.hammers < requirements.hammers ) return;

  int const have_tools = colony.commodities[e_commodity::tools];

  if( colony.commodities[e_commodity::tools] <
      requirements.tools ) {
    ev.notifications.emplace_back(
        ColonyNotification::construction_missing_tools{
            .what       = construction,
            .have_tools = have_tools,
            .need_tools = requirements.tools } );
    return;
  }

  // In the original game, when a construction finishes it resets
  // the hammers to zero, even if we've accumulated more than we
  // need (waiting for tools). If we didn't do this then it might
  // not give the player an incentive to get the tools in a
  // timely manner, since they would never lose hammers. That
  // said, it does appear to allow hammers to accumulate after a
  // construction (while we are waiting for the player to change
  // construction) and those hammers can then be reused on the
  // next construction (to some extent, depending on difficulty
  // level).
  colony.hammers = 0;
  colony.commodities[e_commodity::tools] -= requirements.tools;
  CHECK_GE( colony.commodities[e_commodity::tools], 0 );

  ev.notifications.emplace_back(
      ColonyNotification::construction_complete{
          .what = construction } );

  switch( construction.to_enum() ) {
    using e = Construction::e;
    case e::building: {
      auto& o = construction.get<Construction::building>();
      add_colony_building( colony, o.what );
      DCHECK( !ev.built.has_value() );
      ev.built = o.what;
      return;
    }
    case e::unit: {
      auto& o = construction.get<Construction::unit>();
      // We need a real map updater here because e.g. theoreti-
      // cally we could construct a unit that has a two-square
      // sighting radius and that might cause new squares to be-
      // come visible, which would have to be rendered. That
      // said, in the original game, no unit that can be con-
      // structed in this manner has a sighting radius of more
      // than one.
      create_unit_on_map_non_interactive( ss, ts, player, o.type,
                                          colony.location );
      break;
    }
  }
}

void apply_commodity_increase(
    Colony& colony, e_commodity what, int delta,
    vector<ColonyNotification>& notifications ) {
  int const old_value      = colony.commodities[what];
  int const new_value      = old_value + delta;
  colony.commodities[what] = new_value;
  bool const new_full_cargo =
      ( old_value < 100 && new_value >= 100 );
  if( !new_full_cargo ) return;
  // Don't report food because food is not typically something
  // that gets sold. Typically the player will want to allow it
  // to grow beyond 100 to 200 (which it can always do because
  // there are no warehouse limits on food) in order to produce a
  // new colonist.
  if( what == e_commodity::food ) return;
  // Don't report a full cargo on something that is being managed
  // by the custom house, since the player will be notified sepa-
  // rately when the custom house sells it.
  if( colony.custom_house[what] ) return;
  // Notify the player of a full cargo since they will likely
  // want to quickly move a ship into the colony to pick it up to
  // sell it, or take some other action.
  notifications.emplace_back(
      ColonyNotification::full_cargo{ .what = what } );
}

void apply_bells_for_founding_fathers( Player& player,
                                       int     bells_produced ) {
  if( has_all_fathers( player ) ) {
    // When all fathers have been obtained we want to stop accu-
    // mulating bells for them, otherwise this number would keep
    // increasing indefinitely for no purpose.
    player.fathers.bells = 0;
    return;
  }
  player.fathers.bells += bells_produced;
}

void evolve_sons_of_liberty(
    Player const& player, int bells_produced, Colony& colony,
    vector<ColonyNotification>& notifications ) {
  SonsOfLiberty& sol        = colony.sons_of_liberty;
  int const      population = colony_population( colony );
  // This won't happen in practice because the game does not
  // allow zero-population colonies, but it is useful for unit
  // tests where that something happens, and which would other-
  // wise cause code below to check-fail. Conceptually it makes
  // sense either way, because a population zero colony cannot
  // have any sons of liberty.
  if( population == 0 ) {
    sol.num_rebels_from_bells_only            = 0.0;
    sol.last_sons_of_liberty_integral_percent = 0;
    return;
  }

  sol.num_rebels_from_bells_only =
      evolve_num_rebels_from_bells_only(
          sol.num_rebels_from_bells_only, bells_produced,
          population );
  int const new_sons_of_liberty_integral_percent =
      compute_sons_of_liberty_integral_percent(
          compute_sons_of_liberty_percent(
              sol.num_rebels_from_bells_only, population,
              player.fathers
                  .has[e_founding_father::simon_bolivar] ) );
  for( int const threshold :
       config_colony.notifications
           .sons_of_liberty_notification_thresholds ) {
    if( sol.last_sons_of_liberty_integral_percent < threshold &&
        new_sons_of_liberty_integral_percent >= threshold ) {
      notifications.push_back(
          ColonyNotification::sons_of_liberty_increased{
              .from = sol.last_sons_of_liberty_integral_percent,
              .to   = new_sons_of_liberty_integral_percent } );
      break;
    }
    if( sol.last_sons_of_liberty_integral_percent >= threshold &&
        new_sons_of_liberty_integral_percent < threshold ) {
      notifications.push_back(
          ColonyNotification::sons_of_liberty_decreased{
              .from = sol.last_sons_of_liberty_integral_percent,
              .to   = new_sons_of_liberty_integral_percent } );
      break;
    }
  }
  sol.last_sons_of_liberty_integral_percent =
      new_sons_of_liberty_integral_percent;
}

void apply_production_to_colony(
    Colony& colony, ColonyProduction const& production,
    vector<ColonyNotification>& notifications ) {
  for( e_commodity c : refl::enum_values<e_commodity> ) {
    int delta =
        final_production_delta_for_commodity( production, c );
    apply_commodity_increase( colony, c, delta, notifications );
  }

  colony.hammers +=
      production.lumber_hammers.product_delta_final;

  for( e_commodity c : refl::enum_values<e_commodity> ) {
    CHECK( colony.commodities[c] >= 0,
           "colony {} has a negative quantity ({}) of {}.",
           colony.name, colony.commodities[c], c );
  }
}

void check_colonist_on_the_job_training(
    SS& ss, TS& ts, Colony& colony, ColonyProduction const& pr,
    vector<ColonyNotification>& notifications ) {
  vector<OnTheJobPromotionResult> const res =
      workers_to_promote_for_on_the_job_training( ss, ts,
                                                  colony );
  for( auto const& [unit_id, promoted_to] : res ) {
    CHECK( as_const( ss.units )
               .ownership_of( unit_id )
               .holds<UnitOwnership::colony>() );
    bool should_promote = true;
    if( !config_colony.on_the_job_training
             .can_promote_with_zero_production ) {
      // We're not promoting the unit if they're producing zero
      // of what they are attempting to produce. E.g., if a
      // colonist is working as a cotton planter on a grassland
      // tile they'll produce zero. In that case, we don't want
      // to promote them since that would be strange. This de-
      // parts from the behavior of the original game, but it
      // seems sensible.
      //
      // Currently we will only check this for outdoor units,
      // since 1) in practice those are the only units that are
      // eligible for promotion, and 2) is it very rare that an
      // indoor unit produces nothing; e.g. you'd need a petty
      // criminal working in a building with a tory penalty.
      // FIXME: this should be fixed at some point for the sake
      // of completeness; in order to do this we should have the
      // colony production data structure record the quantity
      // produce by each unit.
      //
      // Default to allowing the unit to be promoted, unless we
      // specifically find that the unit is outdoors and is pro-
      // ducing nothing.
      for( auto const& [direction, outdoor_unit] :
           colony.outdoor_jobs ) {
        if( !outdoor_unit.has_value() )
          // No unit working on this square.
          continue;
        auto const [outdoor_unit_id, job] = *outdoor_unit;
        if( outdoor_unit_id != unit_id ) continue;
        // We've found the unit.
        if( pr.land_production[direction].quantity == 0 )
          should_promote = false;
        break;
      }
    }
    if( should_promote ) {
      Unit& unit = ss.units.unit_for( unit_id );
      change_unit_type( ss, ts, unit, promoted_to );
      notifications.push_back( ColonyNotification::unit_promoted{
          .promoted_to = promoted_to } );
    }
  }
}

void check_colonists_teaching(
    SS& ss, TS& ts, Colony& colony,
    vector<ColonyNotification>& notifications ) {
  ColonyTeachingEvolution const ev =
      evolve_teachers( ss, ts, colony );
  CHECK_LE( int( ev.teachers.size() ), 3 );

  // First check if we have teachers but no one teachable.
  for( TeacherEvolution const& tev : ev.teachers ) {
    switch( tev.action.to_enum() ) {
      case TeacherAction::e::in_progress:
        break;
      case TeacherAction::e::taught_unit: {
        auto& o = tev.action.get<TeacherAction::taught_unit>();
        notifications.push_back( ColonyNotification::unit_taught{
            .from = o.from_type, .to = o.to_type } );
        break;
      }
      case TeacherAction::e::taught_no_one: {
        // Note that this message ("taught no one") will only be
        // triggered when the teach finishes one teaching cycle,
        // i.e. it won't appear each turn.
        notifications.push_back(
            ColonyNotification::teacher_but_no_students{
                .teacher_type =
                    ss.units.unit_for( tev.teacher_unit_id )
                        .type() } );
        break;
      }
    }
  }
}

void process_custom_house( SS& ss, Player& player,
                           Colony&          colony,
                           ColonyEvolution& ev ) {
  if( !colony.buildings[e_colony_building::custom_house] )
    return;
  CustomHouseSales sales =
      compute_custom_house_sales( ss, player, colony );

  if( !sales.invoices.empty() ) {
    apply_custom_house_sales( ss, player, colony, sales );
    ev.notifications.push_back(
        ColonyNotification::custom_house_sales{
            .what = std::move( sales.invoices ) } );
  }

  if( !sales.boycotted.empty() ) {
    ev.notifications.push_back(
        ColonyNotification::custom_house_selling_boycotted_good{
            .what = std::move( sales.boycotted ) } );
  }
}

void check_prime_resource_depletion(
    SS& ss, TS& ts, Colony const& colony,
    vector<ColonyNotification>& notifications ) {
  vector<DepletionEvent> const events =
      advance_depletion_state( ss, ts.rand, colony );
  update_depleted_tiles( ts.map_updater, events );
  for( DepletionEvent const& event : events )
    notifications.push_back(
        ColonyNotification::prime_resource_depleted{
            .what = event.resource_from,
            .partial_depletion =
                event.resource_to.has_value() } );
}

/****************************************************************
** RealColonyEvolver
*****************************************************************/
struct RealColonyEvolver : public IColonyEvolver {
  RealColonyEvolver( SS& ss, TS& ts ) : ss_( ss ), ts_( ts ) {}

 public: // IColonyEvolver
  ColonyEvolution evolve_one_turn(
      Colony& colony ) const override;

  ColonyNotificationMessage generate_notification_message(
      Colony const&             colony,
      ColonyNotification const& notification ) const override;

 private:
  SS& ss_;
  TS& ts_;
};

ColonyNotificationMessage
RealColonyEvolver::generate_notification_message(
    Colony const&             colony,
    ColonyNotification const& notification ) const {
  ColonyNotificationMessage res{
      // We shouldn't ever use this, but give a fallback to help
      // debugging if we miss something.
      .msg       = base::to_str( notification ),
      .transient = false };

  switch( notification.to_enum() ) {
    case ColonyNotification::e::new_colonist: {
      res.msg = fmt::format(
          "The [{}] colony has produced a new colonist.",
          colony.name );
      break;
    }
    case ColonyNotification::e::colony_starving: {
      res.msg = fmt::format(
          "The [{}] colony is rapidly running out of food.  If "
          "we don't address this soon, some colonists will "
          "starve.",
          colony.name );
      break;
    }
    case ColonyNotification::e::colonist_starved: {
      auto& o = notification
                    .get<ColonyNotification::colonist_starved>();
      res.msg = fmt::format(
          "The [{}] colony has run out of food.  As a result, a "
          "colonist ([{}]) has starved.",
          colony.name, unit_attr( o.type ).name );
      break;
    }
    case ColonyNotification::e::spoilage: {
      auto& o = notification.get<ColonyNotification::spoilage>();
      CHECK( !o.spoiled.empty() );
      if( o.spoiled.size() == 1 ) {
        Commodity const& spoiled = o.spoiled[0];

        res.msg = fmt::format(
            "The store of [{}] in [{}] has exceeded its "
            "warehouse capacity.  [{}] tons have been thrown "
            "out.",
            spoiled.type, colony.name, spoiled.quantity );
      } else { // multiple
        res.msg = fmt::format(
            "Some goods in [{}] have exceeded their warehouse "
            "capacities and have been thrown out.",
            colony.name );
      }
      break;
    }
    case ColonyNotification::e::full_cargo: {
      auto& o =
          notification.get<ColonyNotification::full_cargo>();
      res.msg = fmt::format(
          "A new cargo of [{}] is available in [{}]!",
          lowercase_commodity_display_name( o.what ),
          colony.name );
      break;
    }
    case ColonyNotification::e::construction_missing_tools: {
      auto& o = notification.get<
          ColonyNotification::construction_missing_tools>();
      res.msg = fmt::format(
          "[{}] is in need of [{}] more [tools] to complete its "
          "construction work on the [{}].",
          colony.name, o.need_tools - o.have_tools,
          construction_name( o.what ) );
      break;
    }
    case ColonyNotification::e::construction_complete: {
      auto& o =
          notification
              .get<ColonyNotification::construction_complete>();
      res.msg = fmt::format(
          "[{}] has completed its construction of the [{}]!",
          colony.name, construction_name( o.what ) );
      break;
    }
    case ColonyNotification::e::construction_already_finished: {
      auto& o = notification.get<
          ColonyNotification::construction_already_finished>();
      res.msg = fmt::format(
          "[{}]'s construction of the [{}] has already "
          "completed, we should change its production to "
          "something else.",
          colony.name, construction_name( o.what ) );
      break;
    }
    case ColonyNotification::e::construction_lacking_building: {
      auto& o = notification.get<
          ColonyNotification::construction_lacking_building>();
      res.msg = fmt::format(
          "[{}]'s construction of the [{}] requires the "
          "presence of the [{}] as a prerequisite, and thus "
          "cannot be completed.",
          colony.name, construction_name( o.what ),
          construction_name( Construction::building{
              .what = o.required_building } ) );
      break;
    }
      // clang-format off
    case ColonyNotification::e::construction_lacking_population: {
      // clang-format on
      auto& o = notification.get<
          ColonyNotification::construction_lacking_population>();
      res.msg = fmt::format(
          "[{}]'s construction of the [{}] requires a minimum "
          "population of {}, but only has a population of {}.",
          colony.name, construction_name( o.what ),
          o.required_population, colony_population( colony ) );
      break;
    }
    case ColonyNotification::e::run_out_of_raw_material: {
      auto& o = notification.get<
          ColonyNotification::run_out_of_raw_material>();
      res.msg = fmt::format(
          "[{}] has run out of [{}], Your Excellency.  Our {} "
          "cannot continue production until the supply is "
          "increased.",
          colony.name, o.what,
          config_colony.worker_names_plural[o.job] );
      break;
    }
    case ColonyNotification::e::sons_of_liberty_increased: {
      auto& o = notification.get<
          ColonyNotification::sons_of_liberty_increased>();
      res.msg = fmt::format(
          "Sons of Liberty membership has increased to [{}%] in "
          "[{}]!",
          o.to, colony.name );
      if( o.from < 50 && o.to >= 50 )
        res.msg += fmt::format(
            "  All colonists will now receive a production "
            "bonus: [+{}] for non-expert workers and [+{}] for "
            "expert workers.",
            config_colony.sons_of_liberty_50_bonus_non_expert,
            config_colony.sons_of_liberty_50_bonus_expert );
      if( o.from < 100 && o.to == 100 )
        res.msg += fmt::format(
            "  All colonists will now receive a production "
            "bonus: [+{}] for non-expert workers and [+{}] for "
            "expert workers.",
            config_colony.sons_of_liberty_100_bonus_non_expert,
            config_colony.sons_of_liberty_100_bonus_expert );
      break;
    }
    case ColonyNotification::e::sons_of_liberty_decreased: {
      auto& o = notification.get<
          ColonyNotification::sons_of_liberty_decreased>();
      res.msg = fmt::format(
          "Sons of Liberty membership has decreased to [{}%] in "
          "[{}]!",
          o.to, colony.name );
      if( o.from == 100 && o.to < 100 )
        res.msg += fmt::format(
            "  The production bonus afforded to each colonist "
            "is now reduced." );
      if( o.from >= 50 && o.to < 50 )
        res.msg +=
            "  Colonists will no longer receive any production "
            "bonuses.";
      break;
    }
    case ColonyNotification::e::unit_promoted: {
      auto& o =
          notification.get<ColonyNotification::unit_promoted>();
      res.msg = fmt::format(
          "A colonist in [{}] has learned the specialty "
          "profession [{}]!",
          colony.name, unit_attr( o.promoted_to ).name );
      break;
    }
    case ColonyNotification::e::unit_taught: {
      auto& o =
          notification.get<ColonyNotification::unit_taught>();
      switch( o.from ) {
        case e_unit_type::petty_criminal:
          CHECK( o.to == e_unit_type::indentured_servant );
          res.msg = fmt::format(
              "A [Petty Criminal] in [{}] has been promoted to "
              "[Indentured Servant] through education.",
              colony.name );
          break;
        case e_unit_type::indentured_servant:
          CHECK( o.to == e_unit_type::free_colonist );
          res.msg = fmt::format(
              "An [Indentured Servant] in [{}] has been "
              "promoted to [Free Colonist] through education.",
              colony.name );
          break;
        default:
          res.msg = fmt::format(
              "A [Free Colonist] in [{}] has learned the "
              "specialty profession [{}] through education.",
              colony.name, unit_attr( o.to ).name );
          break;
      }
      break;
    }
    case ColonyNotification::e::teacher_but_no_students: {
      auto& o = notification.get<
          ColonyNotification::teacher_but_no_students>();
      res.msg = fmt::format(
          "We have a teacher in [{}] that is teaching the "
          "specialty profession [{}], but there are no "
          "colonists available to teach.",
          colony.name, unit_attr( o.teacher_type ).name );
      break;
    }
    case ColonyNotification::e::custom_house_sales: {
      auto& o =
          notification
              .get<ColonyNotification::custom_house_sales>();
      string goods;
      for( Invoice const& invoice : o.what )
        goods += fmt::format(
            "{} {} for {} at a {}% charge yielding [{}], ",
            invoice.what.quantity, invoice.what.type,
            invoice.money_delta_before_taxes, invoice.tax_rate,
            invoice.money_delta_final );
      // Remove trailing comma.
      goods.resize( goods.size() - 2 );
      goods += '.';
      res.msg = fmt::format(
          "The [Custom House] in [{}] has sold the following "
          "goods: {}",
          colony.name, goods );
      res.transient = true;
      break;
    }
    case ColonyNotification::e::
        custom_house_selling_boycotted_good: {
      auto& o =
          notification
              .get<ColonyNotification::
                       custom_house_selling_boycotted_good>();
      string goods;
      for( e_commodity const comm : o.what )
        goods += fmt::format(
            "[{}], ", lowercase_commodity_display_name( comm ) );
      // Remove trailing comma.
      goods.resize( goods.size() - 2 );
      res.msg = fmt::format(
          "The [Custom House] in [{}] is unable to sell the "
          "following boycotted goods: {}.  We should stop the "
          "Custom House from attempting to sell them until the "
          "boycott is lifted.",
          colony.name, goods );
      break;
    }
    case ColonyNotification::e::prime_resource_depleted: {
      auto& o = notification.get<
          ColonyNotification::prime_resource_depleted>();
      string_view const extent =
          o.partial_depletion ? "partially" : "fully";
      res.msg = fmt::format(
          "A prime [{}] resource near [{}] has been {} "
          "depleted.",
          o.what, colony.name, extent );
      break;
    }
  }
  return res;
}

ColonyEvolution RealColonyEvolver::evolve_one_turn(
    Colony& colony ) const {
  ColonyEvolution ev;
  ev.production = production_for_colony( ss_, colony );

  Player& player =
      player_for_nation_or_die( ss_.players, colony.nation );

  // This must be done after computing the production for the
  // colony since we want the production to use last turn's SoL %
  // and associated bonuses.
  evolve_sons_of_liberty( as_const( player ),
                          ev.production.bells, colony,
                          ev.notifications );

  apply_production_to_colony( colony, ev.production,
                              ev.notifications );
  apply_bells_for_founding_fathers( player,
                                    ev.production.bells );

  check_ran_out_of_raw_materials( ev );

  check_construction( ss_, ts_, as_const( player ), colony, ev );

  // This determines which (and how much) of each commodity
  // should be sold this turn by the custom house. It will then
  // subtract that quantity from the colony and record the re-
  // ceipts, but will not actually process the transaction (so
  // the player's money will not change and the market won't ac-
  // tually receive the goods); that needs to be done at the end
  // of colony processing for all colonies together in such a way
  // as to not cause the prices of any goods to move more than
  // one unit per turn.
  //
  // Note: the custom house should be processed at a point in
  //       the process relative to the other steps in the
  //       following way (FIXME: encode the below in the form of
  //       unit tests):
  //
  //   * After production is computed and applied to the colony.
  //   * After construction is checked so that any tools needed
  //     for a construction project this turn will not be sold.
  //     This allowed rushing the completion of a project without
  //     worrying that the custom house will sell the tools
  //     (though this would only be a concern if tools are being
  //     sold by the custom house, which is not likely).
  //   * Before new colonists are created from food; this allows
  //     the custom house to always prevent the creation of a new
  //     colonist buy enabling the selling of food. That would
  //     only be an issue though if a ship or wagon train dumped
  //     a large amount of food into the colony, since if the
  //     food was just accumulating normally the custom house
  //     would have an opportunity to sell it off before it got
  //     to 200 (which is needed for a new colonist).
  //   * Before spoilage is assessed, so that when a custom house
  //     is selling something in a colony, no amount will ever
  //     cause spoilage, which is how the OG works.
  //
  process_custom_house( ss_, player, colony, ev );

  // Needs to be done after food deltas have been applied.
  check_create_or_starve_colonist(
      ss_, ts_, as_const( player ), colony, ev.production,
      ev.colony_disappeared, ev.notifications );
  if( ev.colony_disappeared )
    // If the colony is to disappear then there isn't much point
    // in doing anything further.
    return ev;

  // Note: This should be done late enough so that anything above
  // that could potentially consume commodities (including the
  // custom house) can do so before they spoil.
  maybe<ColonyNotification::spoilage> spoilage_notification =
      check_spoilage( colony );
  if( spoilage_notification.has_value() )
    ev.notifications.emplace_back(
        std::move( *spoilage_notification ) );

  // Since these can change the type of units, do this as late as
  // possible so that production has already been computed and
  // any other changes to colonists (such as starvation) have al-
  // ready been done.
  check_colonist_on_the_job_training(
      ss_, ts_, colony, ev.production, ev.notifications );
  check_colonists_teaching( ss_, ts_, colony, ev.notifications );

  give_stockade_if_needed( player, colony );

  check_prime_resource_depletion( ss_, ts_, colony,
                                  ev.notifications );

  return ev;
}

} // namespace

/****************************************************************
** IColonyEvolver
*****************************************************************/
unique_ptr<IColonyEvolver> IColonyEvolver::create( SS& ss,
                                                   TS& ts ) {
  return make_unique<RealColonyEvolver>( ss, ts );
}

} // namespace rn
