/****************************************************************
**harbor-extra.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-02-16.
*
* Description: Helpers for the harbor view.
*
*****************************************************************/
#include "harbor-extra.hpp"

// Revolution Now
#include "market.hpp"
#include "player-mgr.hpp"

// ss
#include "ss/old-world-state.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Public API.
*****************************************************************/
HarborUnloadables find_unloadable_slots_in_harbor(
    SSConst const& ss, UnitId const ship_id ) {
  Unit const& unit = ss.units.unit_for( ship_id );
  UNWRAP_CHECK_T( Player const& player,
                  ss.players.players[unit.player_type()] );

  auto const& commodities =
      old_world_state( ss, player.type ).market.commodities;

  HarborUnloadables res;
  res.items.reserve( 6 );
  auto const comms_in_cargo = unit.cargo().commodities();
  for( auto const& [comm, slot] : comms_in_cargo ) {
    e_commodity const type = comm.type;
    res.items.push_back( HarborUnloadable{
      .slot    = slot,
      .comm    = comm,
      .boycott = commodities[type].boycott } );
  }

  auto const sale_value_no_tax = [&]( Commodity const& comm ) {
    return market_price( ss, player, comm.type ).bid *
           comm.quantity;
  };

  auto const comparator = [&]( HarborUnloadable const& l,
                               HarborUnloadable const& r ) {
    return sale_value_no_tax( l.comm ) <
           sale_value_no_tax( r.comm );
  };

  sort( res.items.begin(), res.items.end(), comparator );

  return res;
}

} // namespace rn
