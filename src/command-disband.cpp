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
#include "ts.hpp"
#include "unit-mgr.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/units.hpp"

// Rds
#include "ui-enums.rds.hpp"

using namespace std;

namespace rn {

namespace {

struct DisbandHandler : public CommandHandler {
  DisbandHandler( SS& ss, TS& ts, UnitId unit_id )
    : ss_( ss ), ts_( ts ), unit_id_( unit_id ) {}

  wait<bool> confirm() override {
    auto q = fmt::format(
        "Really disband {}?",
        ss_.units.unit_for( unit_id_ ).desc().name );

    maybe<ui::e_confirm> const answer =
        co_await ts_.gui.optional_yes_no(
            { .msg = q, .yes_label = "Yes", .no_label = "No" } );
    co_return answer == ui::e_confirm::yes;
  }

  wait<> perform() override {
    destroy_unit( ss_, unit_id_ );
    co_return;
  }

  SS&    ss_;
  TS&    ts_;
  UnitId unit_id_;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<CommandHandler> handle_command(
    SS& ss, TS& ts, Player&, UnitId id,
    command::disband const& ) {
  return make_unique<DisbandHandler>( ss, ts, id );
}

} // namespace rn
