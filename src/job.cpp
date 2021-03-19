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
#include "waitable-coro.hpp"
#include "window.hpp"

// base
#include "base/lambda.hpp"

// base-util
#include "base-util/string.hpp"
using namespace std;

namespace rn {

namespace {

valid_or<string> is_valid_colony_name_msg(
    std::string_view name ) {
  if( util::strip( name ) != name )
    return invalid<string>(
        "Colony name must not start or end with spaces." );
  auto res = is_valid_new_colony_name( name );
  if( res ) return valid;
  switch( res.error() ) {
    case e_new_colony_name_err::already_exists:
      return invalid<string>(
          "There is already a colony with that name!" );
    case e_new_colony_name_err::name_too_short:
      return invalid<string>(
          "Name must be longer than one character!" );
  }
}

// Returns future of colony name, unless player clicks Cancel.
waitable<maybe<string>> build_colony_ui_routine() {
  ui::e_confirm proceed =
      co_await ui::yes_no( "Build colony here?" );
  if( proceed == ui::e_confirm::no ) co_return nothing;
  maybe<string> colony_name;
  while( true ) {
    colony_name = co_await ui::str_input_box(
        "Question",
        "What shall this colony be named, your majesty?",
        /*initial_text=*/colony_name.value_or( "" ) );
    if( !colony_name.has_value() ) co_return nothing; // cancel
    auto is_valid = is_valid_colony_name_msg( *colony_name );
    if( is_valid ) co_return( *colony_name );
    co_await ui::message_box( is_valid.error() );
  }
}

} // namespace

bool JobAnalysis::allowed_() const {
  return holds<e_unit_job_good>( desc ).has_value();
}

waitable<bool> JobAnalysis::confirm_explain_() {
  return overload_visit(
      desc,
      [&]( e_unit_job_good val ) -> waitable<bool> {
        switch( val ) {
          case e_unit_job_good::disband: {
            auto q =
                fmt::format( "Really disband {}?",
                             unit_from_id( id ).desc().name );
            ui::e_confirm answer = co_await ui::yes_no( q );
            co_return answer == ui::e_confirm::yes;
          }
          case e_unit_job_good::fortify:
          case e_unit_job_good::sentry: co_return true;
          case e_unit_job_good::build:
            colony_name = ( co_await build_colony_ui_routine() )
                              .value_or( string{} );
            co_return !colony_name.empty();
        }
      },
      []( e_unit_job_error val ) -> waitable<bool> {
        switch( val ) {
          case e_unit_job_error::ship_cannot_fortify:
            co_await ui::message_box(
                "Ships cannot be fortified." );
            co_return false;
          case e_unit_job_error::cannot_fortify_on_ship:
            co_await ui::message_box(
                "Cannot fortify while on a ship." );
            co_return false;
          case e_unit_job_error::colony_exists_here:
            co_await ui::message_box(
                "There is already a colony on this "
                "square." );
            co_return false;
          case e_unit_job_error::no_water_colony:
            co_await ui::message_box(
                "Cannot found a colony on water." );
            co_return false;
          case e_unit_job_error::ship_cannot_found_colony:
            // No message box here since this should be obvious
            // to the player.
            co_return false;
        }
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
      found_colony_unsafe( id, colony_name );
      return;
  }
}

maybe<JobAnalysis> JobAnalysis::analyze_( UnitId   id,
                                          orders_t orders ) {
  maybe<JobAnalysis> res{};

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
    if( auto valid = unit_can_found_colony( id ); valid )
      res->desc = e_unit_job_good::build;
    else {
      switch( valid.error() ) {
        case e_found_colony_err::colony_exists_here:
          res->desc = e_unit_job_error::colony_exists_here;
          break;
        case e_found_colony_err::no_water_colony:
          res->desc = e_unit_job_error::no_water_colony;
          break;
        case e_found_colony_err::ship_cannot_found_colony:
          res->desc = e_unit_job_error::ship_cannot_found_colony;
          break;
        case e_found_colony_err::colonist_not_on_map:
          SHOULD_NOT_BE_HERE;
      }
    }
  }

  return res;
}

} // namespace rn
