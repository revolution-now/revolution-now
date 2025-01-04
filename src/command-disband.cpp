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
  DisbandHandler( SS& ss, TS& ts, Player const& player,
                  UnitId const unit_id )
    : ss_( ss ),
      ts_( ts ),
      player_( player ),
      unit_id_( unit_id ) {
    viz_ = create_visibility_for(
        ss_, player_for_role( ss_, e_player_role::viewer ) );
  }

  wait<bool> confirm() override {
    DisbandingPermissions const perms{
      .disbandable = { .units = { unit_id_ } } };
    entities_ = co_await disband_tile_ui_interaction(
        ss_.as_const, ts_, player_, *viz_, perms );
    co_return !entities_.units.empty();
  }

  wait<> perform() override {
    point const tile =
        coord_for_unit_indirect_or_die( ss_.units, unit_id_ );
    co_await execute_disband( ss_, ts_, *viz_, tile, entities_ );
    co_return;
  }

  SS& ss_;
  TS& ts_;
  Player const& player_;
  UnitId const unit_id_;
  EntitiesOnTile entities_;
  unique_ptr<IVisibility const> viz_;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<CommandHandler> handle_command(
    SS& ss, TS& ts, Player& player, UnitId const unit_id,
    command::disband const& ) {
  return make_unique<DisbandHandler>( ss, ts, player, unit_id );
}

} // namespace rn
