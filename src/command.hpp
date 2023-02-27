/****************************************************************
**command.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-27.
*
* Description: Handles the representation and application of
*              the commands that players can give to units.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "igui.hpp"
#include "unit-id.hpp"
#include "wait.hpp"

// Rds
#include "command.rds.hpp"

// C++ standard library
#include <memory>

namespace rn {

struct Player;
struct SS;
struct TS;

void push_unit_command( UnitId id, command const& command );
maybe<command> pop_unit_command( UnitId id );

struct CommandHandler {
  CommandHandler()          = default;
  virtual ~CommandHandler() = default;

  CommandHandler( CommandHandler const& )            = delete;
  CommandHandler& operator=( CommandHandler const& ) = delete;
  CommandHandler( CommandHandler&& )                 = delete;
  CommandHandler& operator=( CommandHandler&& )      = delete;

  // Run though the entire sequence of
  wait<CommandHandlerRunResult> run();

  // This will do a few things:
  //
  //   1. it will perform more thorough checks to see that this
  //      move can be carried out; if not, will return false and
  //      typically show a message to the user.
  //   2. it will ask the user for input and/or confirmation if
  //      necessary, sometimes allowing the user to cancel the
  //      move (in which case it returns false).
  //   3. if the move can proceed, it will return true.
  //
  virtual wait<bool> confirm() = 0;

  // This will be called when `confirm` has returned true to see
  // if the handler wants to delegate to another handler for the
  // remainder. If so, this function will return non-null, then
  // the process will start over again with the new handler.
  virtual std::unique_ptr<CommandHandler> switch_handler() {
    return nullptr;
  }

  // Animate the commands being carried out, if any. This should
  // be run before `perform`.
  virtual wait<> animate() const { return make_wait<>(); }

  // Perform the commands (i.e., make changes to game state).
  virtual wait<> perform() = 0;

 protected:
  virtual std::vector<UnitId> units_to_prioritize() const {
    return {};
  }
};

std::unique_ptr<CommandHandler> command_handler(
    SS& ss, TS& ts, Player& player, UnitId id,
    command const& command );

} // namespace rn
