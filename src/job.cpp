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
#include "ustate.hpp"
#include "window.hpp"

// base-util
#include "base-util/variant.hpp"

using namespace std;

using util::holds;

namespace rn {

namespace {} // namespace

bool JobAnalysis::allowed_() const {
  return holds<e_unit_job_good>( desc ) != nullptr;
}

sync_future<bool> JobAnalysis::confirm_explain_() const {
  return matcher_( desc, ->, sync_future<bool> ) {
    case_( e_unit_job_good ) {
      switch( val ) {
        case e_unit_job_good::disband: {
          auto q = fmt::format( "Really disband {}?",
                                unit_from_id( id ).desc().name );
          return ui::yes_no( q ).then(
              L( _ == ui::e_confirm::yes ) );
        }
        case e_unit_job_good::fortify:
        case e_unit_job_good::sentry:
          return make_sync_future<bool>( true );
      }
    }
    case_( e_unit_job_error ) {
      auto return_false = []( auto ) { return false; };
      switch( val ) {
        case e_unit_job_error::ship_cannot_fortify:
          return ui::message_box( "Ships cannot be fortified." )
              .then( return_false );
        case e_unit_job_error::cannot_fortify_on_ship:
          return ui::message_box(
                     "Cannot fortify while on a ship." )
              .then( return_false );
      }
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
  }

  return res;
}

} // namespace rn
