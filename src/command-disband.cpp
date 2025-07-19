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
#include "iagent.hpp"
#include "iengine.hpp"
#include "roles.hpp"
#include "unit-mgr.hpp"
#include "visibility.hpp"

// ss
#include "ss/ref.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::point;

/****************************************************************
** DisbandHandler
*****************************************************************/
struct DisbandHandler : public CommandHandler {
  DisbandHandler( IEngine& engine, SS& ss, TS& ts, IAgent& agent,
                  Player const& player, UnitId const unit_id )
    : engine_( engine ),
      ss_( ss ),
      ts_( ts ),
      player_( player ),
      agent_( agent ),
      unit_id_( unit_id ) {}

  wait<bool> confirm() override {
    ui::e_confirm const res =
        co_await agent_.confirm_disband_unit( unit_id_ );
    co_return res == ui::e_confirm::yes;
  }

  wait<> perform() override {
    point const tile =
        coord_for_unit_indirect_or_die( ss_.units, unit_id_ );
    auto const viz = create_visibility_for(
        ss_, player_for_role( ss_, e_player_role::viewer ) );
    EntitiesOnTile const entities{ .units = { unit_id_ } };
    co_await execute_disband( ss_, ts_, player_, *viz, tile,
                              entities );
  }

  IEngine& engine_;
  SS& ss_;
  TS& ts_;
  Player const& player_;
  IAgent& agent_;
  UnitId const unit_id_;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<CommandHandler> handle_command(
    IEngine& engine, SS& ss, TS& ts, IAgent& agent,
    Player& player, UnitId const unit_id,
    command::disband const& ) {
  return make_unique<DisbandHandler>( engine, ss, ts, agent,
                                      player, unit_id );
}

} // namespace rn
