/****************************************************************
**teaching.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-19.
*
* Description: All things related to schools/colleges/university.
*
*****************************************************************/
#include "teaching.hpp"

// Revolution Now
#include "colony-buildings.hpp"
#include "irand.hpp"
#include "promotion.hpp"
#include "ts.hpp"

// config
#include "config/colony.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colony-enums.rds.hpp"
#include "ss/colony.rds.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {

vector<UnitId> teachable_colonists( SSConst const& ss,
                                    Colony const&  colony ) {
  vector<UnitId> units = colony_units_all( colony );
  // Erase the units that cannot be taught.
  erase_if( units, [&]( UnitId unit_id ) {
    e_unit_type const unit_type =
        ss.units.unit_for( unit_id ).type();
    switch( unit_type ) {
      case e_unit_type::petty_criminal:
        return false;
      case e_unit_type::indentured_servant:
        return false;
      case e_unit_type::free_colonist:
        return false;
      default:
        return true;
    }
  } );
  return units;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void sync_colony_teachers( Colony& colony ) {
  unordered_map<UnitId, int> current_teachers = colony.teachers;
  colony.teachers.clear();
  colony.teachers.reserve(
      colony.indoor_jobs[e_indoor_job::teacher].size() );
  for( UnitId unit_id :
       colony.indoor_jobs[e_indoor_job::teacher] )
    // Will initialize to zero if it is not in the list of cur-
    // rent teachers, which is what we want.
    colony.teachers[unit_id] = current_teachers[unit_id];
}

int max_teachers_allowed( Colony const& colony ) {
  maybe<e_colony_building> const building = building_for_slot(
      colony, e_colony_building_slot::schools );
  if( !building.has_value() ) return 0;
  UNWRAP_CHECK( school_type,
                school_type_from_building( *building ) );
  return config_colony.teaching.max_teachers[school_type];
}

// This will actuall evolve the teachers and promote any units.
ColonyTeachingEvolution evolve_teachers( SS& ss, TS& ts,
                                         Player const& player,
                                         Colony&       colony ) {
  ColonyTeachingEvolution ev;
  if( colony.teachers.empty() ) return ev;

  // This computes the list of colonists that are available to
  // teach, but randomly shuffled; but this does it lazily since
  // it would be a waste on most turns to do this.
  auto shuffled_teachable =
      [&, stored = maybe<vector<UnitId>>{}]() mutable
      -> vector<UnitId>& {
    if( !stored.has_value() ) {
      // First get the colonists available to teach.
      stored = teachable_colonists( ss, colony );
      // For ease of testing/predictability.
      sort( stored->begin(), stored->end() );
      ts.rand.shuffle( *stored );
    }
    DCHECK( stored.has_value() );
    return *stored;
  };

  CHECK_EQ( colony.teachers.size(),
            colony.indoor_jobs[e_indoor_job::teacher].size() );
  // Iterate over indoor_jobs since the order is deterministic.
  for( UnitId teacher_id :
       colony.indoor_jobs[e_indoor_job::teacher] ) {
    auto&            turns = colony.teachers[teacher_id];
    TeacherEvolution tev;
    tev.teacher_unit_id = teacher_id;
    Unit const& teacher = ss.units.unit_for( teacher_id );
    UNWRAP_CHECK( expert_activity,
                  unit_attr( teacher.type() ).expertise );
    e_school_type const school_type_for_expertise =
        config_colony.teaching
            .school_type_for_activity[expert_activity];
    int const turns_needed =
        config_colony.teaching
            .turns_needed[school_type_for_expertise];
    DCHECK( turns < turns_needed );
    ++turns;
    if( turns == turns_needed ) {
      if( shuffled_teachable().empty() )
        tev.action = TeacherAction::taught_no_one{};
      else {
        // We have a unit to teach, so promote that unit.
        UnitId const to_promote_id = shuffled_teachable().back();
        Unit& to_promote = ss.units.unit_for( to_promote_id );
        UNWRAP_CHECK( new_comp, promoted_from_activity(
                                    to_promote.composition(),
                                    expert_activity ) );
        tev.action = TeacherAction::taught_unit{
            .taught_id = to_promote_id,
            .from_type = to_promote.type(),
            .to_type   = new_comp.type() };
        shuffled_teachable().pop_back();
        to_promote.change_type( player, new_comp );
      }
      turns = 0;
    } else {
      tev.action = TeacherAction::in_progress{};
    }
    ev.teachers.push_back( std::move( tev ) );
  }

  return ev;
}

base::valid_or<std::string> can_unit_teach_in_building(
    e_unit_type type, e_school_type school_type ) {
  UnitTypeAttributes const attr =
      unit_attr( UnitType( type ).base_type() );

  // We need to get the base type so that e.g. if `type` is a
  // veteran soldier then we will evaluate the veteran_colonist
  // type which is the one that actually has the expertise.
  bool const is_expert = attr.expertise.has_value();
  if( !is_expert )
    return "Only expert colonists with a specialy profession "
           "may teach in schools, colleges, and universities.";

  e_unit_activity const activity = *attr.expertise;

  e_school_type const required_building =
      config_colony.teaching.school_type_for_activity[activity];

  if( school_type < required_building )
    return fmt::format(
        "This [{}] requires at least a [{}] in order "
        "to teach.",
        attr.name,
        config_colony
            .building_display_names[building_for_school_type(
                required_building )] );

  return base::valid;
}

} // namespace rn
