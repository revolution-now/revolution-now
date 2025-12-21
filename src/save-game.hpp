/****************************************************************
**save-game.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-18.
*
* Description: Interface for saving and loading a game.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "error.hpp"
#include "expect.hpp"
#include "wait.hpp"

// base
#include "base/fs.hpp"
#include "base/no-discard.hpp"

namespace rn {

struct IGameStorageLoad;
struct IGameStorageQuery;
struct IGameStorageSave;
struct IEngine;
struct IGui;
struct RootState;
struct SS;
struct SSConst;

/****************************************************************
** Slots.
*****************************************************************/
// This will open the save-game dialog box and will allow the
// player to choose a slot, but will not actually do the saving.
wait<maybe<int>> select_save_slot(
    IEngine& engine, IGui& gui, IGameStorageQuery const& query );

// This will open the load-game dialog box and will allow the
// player to choose a slot, but will not actually do the loading.
// This is useful in cases where we want to immediately display
// the box to the user, but we want to do the loading elsewhere.
wait<maybe<int>> select_load_slot(
    IGui& gui, IGameStorageQuery const& query );

/****************************************************************
** Saving.
*****************************************************************/
wait_bool save_to_slot_interactive(
    SSConst const& ss, IGui& gui, IGameStorageSave const& saver,
    RootState& saved, int slot );

expect<fs::path> save_to_slot( SSConst const& ss, IGui& gui,
                               IGameStorageSave const& saver,
                               RootState& saved, int slot );

expect<fs::path> save_to_slot_no_checkpoint(
    IGameStorageSave const& saver, int slot );

struct SlotCopiedPaths {
  fs::path src;
  fs::path dst;
};

// When there is a save file in one slot that you want to save
// (copy) to another, overwriting the destination save file if
// present.
expect<SlotCopiedPaths> copy_slot_to_slot(
    IGameStorageSave const& saver, int src_slot, int dst_slot );

/****************************************************************
** Loading.
*****************************************************************/
// Given a slot (which must contain a save file) load it. Will
// return if the load actually succeeded.
wait_bool load_from_slot_interactive(
    SS& ss, IGui& gui, IGameStorageLoad const& loader,
    RootState& saved, int slot );

expect<fs::path> load_from_slot( SS& ss,
                                 IGameStorageLoad const& loader,
                                 RootState& saved, int slot );

} // namespace rn
