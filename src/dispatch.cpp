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
  CHECK( !unit.moved_this_turn() );
  CHECK( unit.orders_mean_input_required() );

  for( auto f : dispatches )
    if( maybe_res = f( id, orders ); maybe_res.has_value() )
      return maybe_res;

  return maybe_res;
}

bool confirm_explain( PlayerIntent const& analysis ) {
  return util::visit( analysis, L( _.confirm_explain() ) );
}

void affect_orders( PlayerIntent const& analysis ) {
  util::visit( analysis, L( _.affect_orders() ) );
}

vector<UnitId> units_to_prioritize(
    PlayerIntent const& analysis ) {
  return util::visit( analysis, L( _.units_to_prioritize ) );
}

} // namespace rn
