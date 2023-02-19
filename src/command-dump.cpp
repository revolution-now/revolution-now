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
#include "logger.hpp"
#include "ts.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// C++ standard library
#include <map>

using namespace std;

namespace rn {

namespace {

struct DumpHandler : public CommandHandler {
  DumpHandler( SS& ss, TS& ts, UnitId unit_id )
    : ss_( ss ), ts_( ts ), unit_id_( unit_id ) {}

  wait<bool> confirm() override {
    Unit const& unit      = ss_.units.unit_for( unit_id_ );
    int const   num_slots = unit.desc().cargo_slots;
    if( num_slots == 0 ) {
      // The UI shouldn't really allow this to happen for
      // non-cargo units, but let's just be defensive.
      co_await ts_.gui.message_box(
          "Only units with cargo holds can dump cargo "
          "overboard." );
      co_return false;
    }

    // We use a std::map because we want to be able to iterate in
    // increasing slot order.
    map<int, Commodity>        commodities;
    unordered_map<string, int> keys;

    for( int i = 0; i < num_slots; ++i ) {
      // If there is a cargo item whose first (and possibly only)
      // slot is `idx`, it will be returned.
      maybe<Commodity const&> comm =
          unit.cargo()
              .cargo_starting_at_slot( i )
              .get_if<Cargo::commodity>()
              .member( &Cargo::commodity::obj );
      if( !comm.has_value() ) continue;
      commodities[i]            = *comm;
      keys[fmt::to_string( i )] = i;
    }

    if( commodities.empty() ) {
      co_await ts_.gui.message_box(
          "This unit is not carrying any cargo that can be "
          "dumped overboard." );
      co_return false;
    }

    ChoiceConfig config{
        .msg = "What cargo would you like to dump overboard?",
        .options = {},
    };

    for( auto const& [slot, comm] : commodities ) {
      // FIXME: need to put these names into a config file with
      // both singular and plural versions.
      string const text = fmt::format(
          "{} {}", comm.quantity,
          lowercase_commodity_display_name( comm.type ) );
      ChoiceConfigOption option{
          .key          = fmt::to_string( slot ),
          .display_name = text,
      };
      config.options.push_back( option );
    }

    maybe<string> const selection =
        co_await ts_.gui.optional_choice( config );
    if( !selection.has_value() ) co_return false; // cancelled.

    CHECK( keys.contains( *selection ) );
    slot_      = keys[*selection];
    to_remove_ = commodities[slot_];
    co_return true;
  }

  wait<> perform() override {
    lg.info( "dumping {} overboard.", to_remove_ );
    Commodity const removed =
        rm_commodity_from_cargo( ss_.units, unit_id_, slot_ );
    CHECK( removed == to_remove_ );
    co_return;
  }

  SS&    ss_;
  TS&    ts_;
  UnitId unit_id_;
  // These are only relevant if the action is confirmed.
  int       slot_      = 0;
  Commodity to_remove_ = {};
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<CommandHandler> handle_command(
    SS& ss, TS& ts, Player&, UnitId id, command::dump const& ) {
  return make_unique<DumpHandler>( ss, ts, id );
}

} // namespace rn
