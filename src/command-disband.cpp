/****************************************************************
**command-disband.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-16.
*
* Description: Carries out commands to disband a unit.
*
*****************************************************************/
#include "command-disband.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "disband.hpp"

// ss
#include "ss/ref.hpp"

using namespace std;

namespace rn {

namespace {

/****************************************************************
** DisbandHandler
*****************************************************************/
struct DisbandHandler : public CommandHandler {
  DisbandHandler( SS& ss, TS& ts, UnitId const unit_id )
    : ss_( ss ), ts_( ts ), unit_id_( unit_id ) {}

  wait<bool> confirm() override {
    DisbandingPermissions const perms{
      .disbandable = { .units = { unit_id_ } } };
    entities_ = co_await disband_tile_ui_interaction(
        ss_.as_const, ts_, perms );
    co_return !entities_.units.empty();
  }

  wait<> perform() override {
    execute_disband( ss_, ts_, entities_ );
    co_return;
  }

  SS&            ss_;
  TS&            ts_;
  UnitId const   unit_id_;
  EntitiesOnTile entities_;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<CommandHandler> handle_command(
    SS& ss, TS& ts, Player&, UnitId const unit_id,
    command::disband const& ) {
  return make_unique<DisbandHandler>( ss, ts, unit_id );
}

} // namespace rn
