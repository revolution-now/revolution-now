/****************************************************************
**dispatch.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-08.
*
* Description: Dispatches player orders to the right handler.
*
*****************************************************************/
#include "dispatch.hpp"

// Revolution Now
#include "ustate.hpp"

// base-util
#include "base-util/misc.hpp"
#include "base-util/variant.hpp"

// C++ standard library
#include <functional>
#include <vector>

using namespace std;

namespace rn {

namespace {

template<typename T>
Opt<PlayerIntent> try_dispatch( UnitId          id,
                                orders_t const& orders ) {
  return T::analyze( id, orders );
}

vector<function<Opt<PlayerIntent>( UnitId, orders_t const& )>>
    dispatches{
        // Will be tried in this order.
        try_dispatch<MetaAnalysis>,   //
        try_dispatch<JobAnalysis>,    //
        try_dispatch<TravelAnalysis>, //
        try_dispatch<CombatAnalysis>  //
    };

} // namespace

Opt<PlayerIntent> player_intent( UnitId          id,
                                 orders_t const& orders ) {
  Opt<PlayerIntent> maybe_res;

  auto const& unit = unit_from_id( id );
  // TODO: consolidate these checks
  CHECK( unit.movement_points() > 0 );
  CHECK( !unit.mv_pts_exhausted() );
  CHECK( unit.orders_mean_input_required() );

  for( auto f : dispatches )
    if( maybe_res = f( id, orders ); maybe_res.has_value() )
      return maybe_res;

  return maybe_res;
}

sync_future<bool> confirm_explain( PlayerIntent* analysis ) {
  return std::visit(
      []( auto& _ ) { return _.confirm_explain(); }, *analysis );
}

void affect_orders( PlayerIntent const& analysis ) {
  std::visit( L( _.affect_orders() ), analysis );
}

vector<UnitId> units_to_prioritize(
    PlayerIntent const& analysis ) {
  return std::visit( L( _.units_to_prioritize ), analysis );
}

} // namespace rn
