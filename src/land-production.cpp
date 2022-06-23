/****************************************************************
**land-production.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-14.
*
* Description: Computes what is produced on a land square.
*
*****************************************************************/
#include "land-production.hpp"

// Revolution Now
#include "map-square.hpp"
#include "utype.hpp"

// game-state
#include "gs/terrain.hpp"

// config
#include "config/production.rds.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Public API
*****************************************************************/
maybe<e_outdoor_job> outdoor_job_for_expertise(
    e_unit_activity activity ) {
  switch( activity ) {
    case e_unit_activity::farming: return e_outdoor_job::food;
    case e_unit_activity::fishing: return e_outdoor_job::fish;
    case e_unit_activity::sugar_planting:
      return e_outdoor_job::sugar;
    case e_unit_activity::tobacco_planting:
      return e_outdoor_job::tobacco;
    case e_unit_activity::cotton_planting:
      return e_outdoor_job::cotton;
    case e_unit_activity::fur_trapping:
      return e_outdoor_job::fur;
    case e_unit_activity::lumberjacking:
      return e_outdoor_job::lumber;
    case e_unit_activity::ore_mining: return e_outdoor_job::ore;
    case e_unit_activity::silver_mining:
      return e_outdoor_job::silver;
    case e_unit_activity::carpentry:
    case e_unit_activity::rum_distilling:
    case e_unit_activity::tobacconistry:
    case e_unit_activity::weaving:
    case e_unit_activity::fur_trading:
    case e_unit_activity::blacksmithing:
    case e_unit_activity::gunsmithing:
    case e_unit_activity::fighting:
    case e_unit_activity::pioneering:
    case e_unit_activity::scouting:
    case e_unit_activity::missioning:
    case e_unit_activity::bell_ringing:
    case e_unit_activity::preaching: return nothing;
    case e_unit_activity::teaching: return nothing;
  }
}

e_unit_activity activity_for_outdoor_job( e_outdoor_job job ) {
  switch( job ) {
    case e_outdoor_job::food: return e_unit_activity::farming;
    case e_outdoor_job::fish: return e_unit_activity::fishing;
    case e_outdoor_job::sugar:
      return e_unit_activity::sugar_planting;
    case e_outdoor_job::tobacco:
      return e_unit_activity::tobacco_planting;
    case e_outdoor_job::cotton:
      return e_unit_activity::cotton_planting;
    case e_outdoor_job::fur:
      return e_unit_activity::fur_trapping;
    case e_outdoor_job::lumber:
      return e_unit_activity::lumberjacking;
    case e_outdoor_job::ore: return e_unit_activity::ore_mining;
    case e_outdoor_job::silver:
      return e_unit_activity::silver_mining;
  }
}

[[nodiscard]] int apply_outdoor_bonus(
    int const in, bool const is_expert,
    OutdoorJobBonus_t const& bonus ) {
  switch( bonus.to_enum() ) {
    case OutdoorJobBonus::e::none: return in;
    case OutdoorJobBonus::e::add: {
      auto& o = bonus.get<OutdoorJobBonus::add>();
      return is_expert ? ( in + o.expert )
                       : ( in + o.non_expert );
    }
    case OutdoorJobBonus::e::mul: {
      auto& o = bonus.get<OutdoorJobBonus::mul>();
      return in * o.by;
    }
  }
}

int bordering_land_tiles( TerrainState const& terrain_state,
                          Coord               where ) {
  DCHECK( terrain_state.square_at( where ).surface ==
          e_surface::water );
  int n = 0;
  for( e_direction d : refl::enum_values<e_direction> )
    if( terrain_state.total_square_at( where.moved( d ) )
            .surface == e_surface::land )
      ++n;
  return n;
}

bool has_required_resources(
    MapSquare const& square,
    unordered_set<e_natural_resource> const&
        required_resources ) {
  if( square.ground_resource.has_value() &&
      required_resources.contains( *square.ground_resource ) )
    return true;
  if( square.forest_resource.has_value() &&
      required_resources.contains( *square.forest_resource ) )
    return true;
  return false;
}

int production_on_square( e_outdoor_job       job,
                          TerrainState const& terrain_state,
                          e_unit_type type, Coord where ) {
  auto const& conf =
      config_production.outdoor_production.jobs[job];

  maybe<e_unit_activity> const& activity =
      unit_attr( type ).expertise;
  bool const is_expert =
      activity.has_value()
          ? ( outdoor_job_for_expertise( *activity ) == job )
          : false;

  MapSquare const& square  = terrain_state.square_at( where );
  e_terrain const  terrain = effective_terrain( square );

  // If the base production is zero then that is taken to mean
  // that the square should never produce any of that commodity
  // regardless of the unit or terrain bonuses, and so we short
  // circuit here. Otherwise e.g. a mountain tile with a road
  // would produce 2 lumber via the road bonus. This also pre-
  // vents e.g. getting tobacco on a prairie square by putting a
  // prime tobacco resource there, which the original game did
  // not allow, but we technically allow it (because of our more
  // flexible map representation) and so it is nice that this
  // also shields against that.
  if( conf.base_productions[terrain] == 0 ) return 0;

  // If this field is present on a square that has no resources
  // then it overrides everything else. This is used to reproduce
  // some non-standard behavior with regard to silver production
  // that seems to have been inserted into some versions of the
  // game to nerf silver production.
  if( conf.non_resource_override.has_value() &&
      !has_required_resources(
          square,
          conf.non_resource_override->required_resources ) )
    return is_expert ? conf.non_resource_override->expert
                     : conf.non_resource_override->non_expert;

  // In general the order in which these are applied matters be-
  // cause some of them are additive and some multiplicative.
  // When they are all additive, as in e.g. the case of farming
  // in the original game, the order does not matter. In the case
  // of e.g. cotton on prairie tiles, the original game appears
  // to compute in this order, where each is present:
  //
  //   1. base:                   3
  //   2. resource:              x2
  //   3. plow/road/coast:       +1 each
  //   4. major/minor river      +1 (+2 for major)
  //   5. expert:                x2
  //
  // So let's say that we have an expert cotton planter working
  // on a square with prime cotton, a minor river, and plowed,
  // we'd have:
  //
  //   production = ((3*2)+1+1)*2 = 16
  //
  // Note that in this version of the game none of these are mu-
  // tually exclusive; i.e., when a tile is eligible for both
  // plow and river bonuses, they can both be present and will be
  // cumulative. At least some versions of the original game, it
  // appears that when a major river is present then the effects
  // of a road/plow are ignored, and only the major river bonus
  // is given. However that may not be the case in all version,
  // and doesn't appear to be documented. Because of that, and
  // because it introduces arguably a bit too much complexity, we
  // don't do that here.

  // 1. Base.
  int res = conf.base_productions[terrain];

  // 2. Resource Bonus.
  maybe<e_natural_resource> const& resource =
      has_forest( square ) ? square.forest_resource
                           : square.ground_resource;
  if( resource.has_value() )
    res = apply_outdoor_bonus( res, is_expert,
                               conf.resource_bonus[*resource] );

  // 3. Plow/River/Road/Coast Bonus.
  if( square.river.has_value() ) {
    // Note this applies to water squares as well, for when a
    // river empties into the ocean, the ocean tile will be given
    // the river setting as well (though technically this is up
    // to the map generator; the standard generator will do
    // this), and this gives a fishing bonus.
    switch( *square.river ) {
      case e_river::minor:
        res = apply_outdoor_bonus( res, is_expert,
                                   conf.minor_river_bonus );
        break;
      case e_river::major:
        res = apply_outdoor_bonus( res, is_expert,
                                   conf.major_river_bonus );
        break;
    }
  }

  if( square.road )
    res = apply_outdoor_bonus( res, is_expert, conf.road_bonus );

  if( square.irrigation && /*defense*/ !square.overlay )
    res = apply_outdoor_bonus( res, is_expert, conf.plow_bonus );

  if( square.surface == e_surface::water &&
      bordering_land_tiles( terrain_state, where ) >=
          config_production.outdoor_production
              .num_land_tiles_for_coast )
    res =
        apply_outdoor_bonus( res, is_expert, conf.coast_bonus );

  // 4. Expert Bonus.
  if( is_expert )
    res =
        apply_outdoor_bonus( res, is_expert, conf.expert_bonus );

  return res;
}

} // namespace rn
