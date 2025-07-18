/****************************************************************
**command-dump.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-09.
*
* Description: Carries out commands to dump cargo overboard from
*              a ship or wagon train.
*
*****************************************************************/
#include "command-dump.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "commodity.hpp"
#include "ieuro-agent.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/logger.hpp"

// C++ standard library
#include <map>

using namespace std;

namespace rn {

namespace {

struct DumpHandler : public CommandHandler {
  DumpHandler( SS& ss, IEuroAgent& agent, UnitId unit_id )
    : ss_( ss ), agent_( agent ), unit_id_( unit_id ) {}

  wait<bool> confirm() override {
    Unit const& unit    = ss_.units.unit_for( unit_id_ );
    int const num_slots = unit.desc().cargo_slots;
    if( num_slots == 0 ) {
      // The UI shouldn't really allow this to happen for
      // non-cargo units, but let's just be defensive.
      co_await agent_.message_box(
          "Only units with cargo holds can dump cargo "
          "overboard." );
      co_return false;
    }

    // We use a std::map because we want to be able to iterate in
    // increasing slot order.
    map<int, Commodity> commodities;

    for( int i = 0; i < num_slots; ++i ) {
      // If there is a cargo item whose first (and possibly only)
      // slot is `idx`, it will be returned.
      auto const comm = unit.cargo()
                            .cargo_starting_at_slot( i )
                            .inner_if<Cargo::commodity>();
      if( !comm.has_value() ) continue;
      commodities[i] = *comm;
    }

    if( commodities.empty() ) {
      co_await agent_.message_box(
          "This unit is not carrying any cargo that can be "
          "dumped overboard." );
      co_return false;
    }

    auto const selection =
        co_await agent_.pick_dump_cargo( commodities );
    if( !selection.has_value() ) co_return false; // cancelled.
    slot_      = *selection;
    to_remove_ = commodities[slot_];
    co_return true;
  }

  wait<> perform() override {
    lg.info( "dumping {} overboard.", to_remove_ );
    Commodity const removed = rm_commodity_from_cargo(
        ss_.units, ss_.units.unit_for( unit_id_ ).cargo(),
        slot_ );
    CHECK( removed == to_remove_ );
    co_return;
  }

  SS& ss_;
  IEuroAgent& agent_;
  UnitId unit_id_ = {};
  // These are only relevant if the action is confirmed.
  int slot_            = 0;
  Commodity to_remove_ = {};
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<CommandHandler> handle_command(
    IEngine&, SS& ss, TS&, IEuroAgent& agent, Player&, UnitId id,
    command::dump const& ) {
  return make_unique<DumpHandler>( ss, agent, id );
}

} // namespace rn
