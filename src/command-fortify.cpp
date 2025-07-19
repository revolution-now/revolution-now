/****************************************************************
**command-fortify.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-16.
*
* Description: Carries out command to fortify or sentry a unit.
*
*****************************************************************/
#include "command-fortify.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "iagent.hpp"
#include "unit-mgr.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {

struct FortifyHandler : public CommandHandler {
  FortifyHandler( SS& ss, IAgent& agent, UnitId unit_id )
    : ss_( ss ), agent_( agent ), unit_id_( unit_id ) {}

  wait<bool> confirm() override {
    if( is_unit_onboard( ss_.units, unit_id_ ) ) {
      co_await agent_.message_box(
          "Cannot fortify as cargo of another unit." );
      co_return false;
    }
    co_return true;
  }

  wait<> perform() override {
    Unit& unit = ss_.units.unit_for( unit_id_ );
    // Note that this will forfeight movement points, since the
    // original game appears to end a unit's turn when the F
    // order is given (as well as the following turn when it is
    // transitioned to "fortified").
    unit.start_fortify();
    co_return;
  }

  SS& ss_;
  IAgent& agent_;
  UnitId unit_id_ = {};
};

struct SentryHandler : public CommandHandler {
  SentryHandler( SS& ss, UnitId unit_id )
    : ss_( ss ), unit_id_( unit_id ) {}

  wait<bool> confirm() override { co_return true; }

  wait<> perform() override {
    ss_.units.unit_for( unit_id_ ).sentry();
    co_return;
  }

  SS& ss_;
  UnitId unit_id_ = {};
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<CommandHandler> handle_command(
    IEngine&, SS& ss, TS&, IAgent& agent, Player&, UnitId id,
    command::fortify const& ) {
  return make_unique<FortifyHandler>( ss, agent, id );
}

unique_ptr<CommandHandler> handle_command(
    IEngine&, SS& ss, TS&, IAgent&, Player&, UnitId id,
    command::sentry const& ) {
  return make_unique<SentryHandler>( ss, id );
}

} // namespace rn
