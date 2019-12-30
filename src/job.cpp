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
#include "cstate.hpp"
#include "macros.hpp"
#include "ustate.hpp"
#include "window.hpp"

// base-util
#include "base-util/variant.hpp"

using namespace std;

using util::holds;

namespace rn {

namespace {

sync_future<Opt<string>> ask_colony_name() {
  auto q = ui::str_input_box(
      "Question",
      "What shall this colony be named, your majesty?" );
  return q >> []( Opt<string> const& maybe_name ) {
    if( !maybe_name.has_value() )
      return make_sync_future<Opt<string>>();

    auto err_retry = [&]( string_view msg ) {
      // Recursion by calling `ask_colony_name` again.
      return ui::message_box( msg ).next( ask_colony_name );
    };
    if( colony_from_name( *maybe_name ).has_value() )
      return err_retry(
          "There is already a colony with that name!" );
    if( maybe_name->size() <= 1 )
      return err_retry(
          "Name must be longer than one character!" );

    return make_sync_future<Opt<string>>( maybe_name );
  };
}

// Returns future of colony name, if player wants to proceed.
sync_future<Opt<string>> build_colony_ui_routine() {
  auto msg = ui::yes_no( "Build colony here?" );
  return msg >> []( ui::e_confirm answer ) {
    if( answer == ui::e_confirm::no )
      return make_sync_future<Opt<string>>();
    return ask_colony_name();
  };
}

} // namespace

bool JobAnalysis::allowed_() const {
  return holds<e_unit_job_good>( desc ) != nullptr;
}

sync_future<bool> JobAnalysis::confirm_explain_() {
  return matcher_( desc, ->, sync_future<bool> ) {
    case_( e_unit_job_good ) {
      switch( val ) {
        case e_unit_job_good::disband: {
          auto q = fmt::format( "Really disband {}?",
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
    }
    case_( e_unit_job_error ) {
      auto return_false = []( auto ) { return false; };
      switch( val ) {
        case e_unit_job_error::ship_cannot_fortify:
          return ui::message_box( "Ships cannot be fortified." )
              .fmap( return_false );
        case e_unit_job_error::cannot_fortify_on_ship:
          return ui::message_box(
                     "Cannot fortify while on a ship." )
              .fmap( return_false );
        case e_unit_job_error::colony_already_here:
          return ui::message_box(
                     "There is already a colony on this "
                     "square." )
              .fmap( return_false );
      }
      UNREACHABLE_LOCATION;
    }
    matcher_exhaustive;
  }
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
      // FIXME: temporary
      CHECK_XP( create_colony( unit.nation(),
                               coord_for_unit( id ).value(),
                               colony_name ) );
      return;
  }
}

Opt<JobAnalysis> JobAnalysis::analyze_( UnitId   id,
                                        orders_t orders ) {
  Opt<JobAnalysis> res{};

  auto& unit = unit_from_id( id );

  if( holds<orders::fortify>( orders ) ) {
    res = JobAnalysis( id, orders );
    if( unit.desc().boat )
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
    res        = JobAnalysis( id, orders );
    auto coord = coord_for_unit( id );
    // FIXME
    CHECK( coord.has_value() );
    if( colony_from_coord( *coord ).has_value() )
      res->desc = e_unit_job_error::colony_already_here;
    else
      res->desc = e_unit_job_good::build;
  }

  return res;
}

} // namespace rn
