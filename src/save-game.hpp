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
#include "base/vocab.hpp"

namespace rn {

struct IGameStorageLoad;
struct IGameStorageQuery;
struct IGameStorageSave;
struct SS;
struct SSConst;
struct TS;

/****************************************************************
** Slots.
*****************************************************************/
// This will open the save-game dialog box and will allow the
// player to choose a slot, but will not actually do the saving.
wait<maybe<int>> select_save_slot(
    TS& ts, IGameStorageQuery const& query );

// This will open the load-game dialog box and will allow the
// player to choose a slot, but will not actually do the loading.
// This is useful in cases where we want to immediately display
// the box to the user, but we want to do the loading elsewhere.
wait<maybe<int>> select_load_slot(
    TS& ts, IGameStorageQuery const& query );

/****************************************************************
** Saving.
*****************************************************************/
wait<base::NoDiscard<bool>> save_to_slot_interactive(
    SSConst const& ss, TS& ts, IGameStorageSave const& saver,
    int slot );

expect<fs::path> save_to_slot( SSConst const& ss, TS& ts,
                               IGameStorageSave const& saver,
                               int                     slot );

expect<fs::path> save_to_slot_no_checkpoint(
    SSConst const& ss, TS& ts, IGameStorageSave const& saver,
    int slot );

struct SlotCopiedPaths {
  fs::path src;
  fs::path dst;
};

// When there is a save file in one slot that you want to save
// (copy) to another, overwriting the destination save file if
// present.
expect<SlotCopiedPaths> copy_slot_to_slot(
    SSConst const& ss, TS& ts, IGameStorageSave const& saver,
    int src_slot, int dst_slot );

/****************************************************************
** Loading.
*****************************************************************/
// Given a slot (which must contain a save file) load it. Will
// return if the load actually succeeded.
wait<base::NoDiscard<bool>> load_from_slot_interactive(
    SS& ss, TS& ts, IGameStorageLoad const& loader, int slot );

expect<fs::path> load_from_slot( SS& ss, TS& ts,
                                 IGameStorageLoad const& loader,
                                 int                     slot );

} // namespace rn
