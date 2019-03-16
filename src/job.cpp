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
#include "ownership.hpp"
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

bool JobAnalysis::confirm_explain_() const {
  if( !allowed() ) return false;
  if_v( desc, e_unit_job_good, val ) {
    if( *val == e_unit_job_good::disband ) {
      auto q = fmt::format( "Really disband {}?",
                            unit_from_id( id ).desc().name );
      return ui::yes_no( q ) == ui::e_confirm::yes;
    }
  }
  return true;
}

void JobAnalysis::affect_orders_() const {
  auto& unit = unit_from_id( id );
  CHECK( !unit.moved_this_turn() );
  CHECK( unit.orders() == Unit::e_orders::none );
  CHECK( holds<e_unit_job_good>( desc ) );
  switch( get<e_unit_job_good>( desc ) ) {
    case e_unit_job_good::sentry: unit.sentry(); return;
    case e_unit_job_good::fortify: unit.fortify(); return;
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
