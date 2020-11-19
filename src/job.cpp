/****************************************************************
**job.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-27.
*
* Description: Handles job orders on units.
*
*****************************************************************/
#include "job.hpp"

// Revolution Now
#include "colony-mgr.hpp"
#include "cstate.hpp"
#include "macros.hpp"
#include "ustate.hpp"
#include "variant.hpp"
#include "window.hpp"

// base-util
#include "base-util/variant.hpp"

using namespace std;

using util::holds;

namespace rn {

namespace {

expect<> is_valid_colony_name_input(
    Opt<string> const& proposed ) {
  if( !proposed.has_value() ) return xp_success_t{};
  if( colony_from_name( *proposed ).has_value() )
    return UNEXPECTED(
        "There is already a colony with that name!" );
  if( proposed->size() <= 1 )
    return UNEXPECTED(
        "Name must be longer than one character!" );
  return xp_success_t{};
}

sync_future<Opt<string>> ask_colony_name() {
  return ui::str_input_box(
      "Question",
      "What shall this colony be named, your majesty?" );
}

// Returns future of colony name, unless player clicks Cancel.
sync_future<Opt<string>> build_colony_ui_routine() {
  auto msg = ui::yes_no( "Build colony here?" );
  return msg >> []( ui::e_confirm answer ) {
    if( answer == ui::e_confirm::no )
      return make_sync_future<Opt<string>>();
    return ui::repeat_until<Opt<string>>(
        /*to_repeat=*/ask_colony_name,
        /*get_error=*/is_valid_colony_name_input );
  };
}

} // namespace

bool JobAnalysis::allowed_() const {
  return holds<e_unit_job_good>( desc ) != nullptr;
}

sync_future<bool> JobAnalysis::confirm_explain_() {
  return overload_visit(
      desc,
      [&]( e_unit_job_good val ) {
        switch( val ) {
          case e_unit_job_good::disband: {
            auto q =
                fmt::format( "Really disband {}?",
                             unit_from_id( id ).desc().name );
            return ui::yes_no( q ).fmap(
                L( _ == ui::e_confirm::yes ) );
          }
          case e_unit_job_good::fortify:
          case e_unit_job_good::sentry:
            return make_sync_future<bool>( true );
          case e_unit_job_good::build:
            return build_colony_ui_routine().fmap(
                [this]( Opt<string> const& maybe_name ) {
                  if( maybe_name.has_value() )
                    colony_name = *maybe_name;
                  return maybe_name.has_value();
                } );
        }
        UNREACHABLE_LOCATION;
      },
      []( e_unit_job_error val ) {
        auto return_false = []( auto ) { return false; };
        switch( val ) {
          case e_unit_job_error::ship_cannot_fortify:
            return ui::message_box(
                       "Ships cannot be fortified." )
                .fmap( return_false );
          case e_unit_job_error::cannot_fortify_on_ship:
            return ui::message_box(
                       "Cannot fortify while on a ship." )
                .fmap( return_false );
          case e_unit_job_error::colony_exists_here:
            return ui::message_box(
                       "There is already a colony on this "
                       "square." )
                .fmap( return_false );
          case e_unit_job_error::no_water_colony:
            return ui::message_box(
                       "Cannot found a colony on water." )
                .fmap( return_false );
          case e_unit_job_error::ship_cannot_found_colony:
            // No message box here since this should be obvious
            // to the player.
            return make_sync_future<bool>( false );
        }
        UNREACHABLE_LOCATION;
      } );
}

void JobAnalysis::affect_orders_() const {
  auto& unit = unit_from_id( id );
  CHECK( !unit.mv_pts_exhausted() );
  CHECK( unit.orders() == e_unit_orders::none );
  CHECK( holds<e_unit_job_good>( desc ) );
  switch( get<e_unit_job_good>( desc ) ) {
    case e_unit_job_good::sentry: unit.sentry(); return;
    case e_unit_job_good::fortify:
      unit.forfeight_mv_points();
      unit.fortify();
      return;
    case e_unit_job_good::disband:
      destroy_unit( unit.id() );
      return;
    case e_unit_job_good::build:
      CHECK_XP( found_colony( id, colony_name ) );
      return;
  }
}

Opt<JobAnalysis> JobAnalysis::analyze_( UnitId   id,
                                        orders_t orders ) {
  Opt<JobAnalysis> res{};

  auto& unit = unit_from_id( id );

  if( holds<orders::fortify>( orders ) ) {
    res = JobAnalysis( id, orders );
    if( unit.desc().ship )
      res->desc = e_unit_job_error::ship_cannot_fortify;
    else if( is_unit_onboard( id ) )
      res->desc = e_unit_job_error::cannot_fortify_on_ship;
    else
      res->desc = e_unit_job_good::fortify;
  } else if( holds<orders::sentry>( orders ) ) {
    res       = JobAnalysis( id, orders );
    res->desc = e_unit_job_good::sentry;
  } else if( holds<orders::disband>( orders ) ) {
    res       = JobAnalysis( id, orders );
    res->desc = e_unit_job_good::disband;
  } else if( holds<orders::build>( orders ) ) {
    res = JobAnalysis( id, orders );
    switch( can_found_colony( id ) ) {
      case e_found_colony_result::good:
        res->desc = e_unit_job_good::build;
        break;
      case e_found_colony_result::colony_exists_here:
        res->desc = e_unit_job_error::colony_exists_here;
        break;
      case e_found_colony_result::no_water_colony:
        res->desc = e_unit_job_error::no_water_colony;
        break;
      case e_found_colony_result::ship_cannot_found_colony:
        res->desc = e_unit_job_error::ship_cannot_found_colony;
        break;
      case e_found_colony_result::colonist_not_on_map:
        SHOULD_NOT_BE_HERE;
    }
  }

  return res;
}

} // namespace rn
