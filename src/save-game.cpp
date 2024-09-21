/****************************************************************
**save-game.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-18.
*
* Description: Interface for saving and loading a game.
*
*****************************************************************/
#include "save-game.hpp"

// Revolution Now
#include "error.hpp"
#include "igame-storage.hpp"
#include "igui.hpp"
#include "logger.hpp"
#include "macros.hpp"
#include "ts.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/terrain.hpp" // FIXME
#include "ss/turn.hpp"

// config
#include "config/savegame.rds.hpp"

// base
#include "base/conv.hpp"
#include "base/fs.hpp"
#include "base/string.hpp"
#include "base/timer.hpp"
#include "base/to-str-ext-std.hpp"

// C++ standard library
#include <fstream>

using namespace std;

namespace rn {

namespace {

/****************************************************************
** Helpers.
*****************************************************************/
fs::path stem_for_slot( int slot ) {
  CHECK( slot >= 0 );
  return config_savegame.folder /
         fmt::format( "slot{:02}", slot );
}

fs::path query_file_for_slot( IGameStorageQuery const& query,
                              int                      slot ) {
  auto p = stem_for_slot( slot );
  p.replace_extension( query.extension() );
  return p;
}

bool query_slot_exists( IGameStorageQuery const& query,
                        int                      slot ) {
  return fs::exists( query_file_for_slot( query, slot ) );
}

int number_of_normal_slots() {
  return config_savegame.num_normal_save_slots;
}

int number_of_total_slots() {
  int const num_autosave_slots =
      config_savegame.autosave.slots.size();
  return number_of_normal_slots() + num_autosave_slots;
}

// We must record the serialized state of the game each time it
// is loaded or saved so that we can check when it is dirty.
void record_checkpoint( SSConst const& ss, TS& ts ) {
  // If we're trying to copy to ourselves then something is
  // wrong.
  CHECK( &ts.saved != &ss.root );
  base::timer( "copy of root baseline", [&] {
    assign_src_to_dst( as_const( ss.root ), ts.saved );
  } );
}

} // namespace

/****************************************************************
** Slot selection.
*****************************************************************/
static wait<maybe<int>> select_save_slot_impl(
    TS& ts, IGameStorageQuery const& query ) {
  unordered_map<int, string> slots;
  for( int i = 0; i < number_of_total_slots(); ++i )
    if( query_slot_exists( query, i ) )
      slots[i] =
          query.description( query_file_for_slot( query, i ) );
  ChoiceConfig config{
      .msg  = "Select a slot:",
      .sort = false,
  };
  int const    num_slots  = number_of_normal_slots();
  string const kEmptyName = "(none)";
  for( int i = 0; i < num_slots; ++i ) {
    string summary = kEmptyName;
    if( slots.contains( i ) ) {
      summary = slots[i];
      if( i >= number_of_normal_slots() )
        summary = "Autosave: " + summary;
    }
    config.options.push_back( { .key = fmt::to_string( i ),
                                .display_name = summary,
                                .disabled     = false } );
  }
  while( true ) {
    maybe<string> selection =
        co_await ts.gui.optional_choice( config );
    if( !selection.has_value() ) co_return nothing;
    UNWRAP_CHECK( slot, base::from_chars<int>( *selection ) );
    co_return slot;
  }
}

wait<maybe<int>> select_save_slot(
    TS& ts, IGameStorageQuery const& query ) {
  maybe<int> slot;
  while( true ) {
    slot = co_await select_save_slot_impl( ts, query );
    if( !slot.has_value() ) co_return nothing;
    if( !query_slot_exists( query, *slot ) ) break;
    YesNoConfig const config{ .msg =
                                  "A saved game already exists "
                                  "in this slot.  Overwite?",
                              .yes_label      = "Overwrite",
                              .no_label       = "Cancel",
                              .no_comes_first = true };

    maybe<ui::e_confirm> const answer =
        co_await ts.gui.optional_yes_no( config );
    if( answer == ui::e_confirm::yes ) break;
  }
  CHECK( slot.has_value() );
  co_return *slot;
}

wait<maybe<int>> select_load_slot(
    TS& ts, IGameStorageQuery const& query ) {
  unordered_map<int, string> slots;
  for( int i = 0; i < number_of_total_slots(); ++i )
    if( query_slot_exists( query, i ) )
      slots[i] =
          query.description( query_file_for_slot( query, i ) );
  if( slots.size() == 0 ) {
    co_await ts.gui.message_box(
        "There are no available games to load." );
    co_return nothing;
  }
  ChoiceConfig config{
      .msg  = "Select a slot:",
      .sort = false,
  };
  int const    num_slots  = number_of_total_slots();
  string const kEmptyName = "(none)";
  for( int i = 0; i < num_slots; ++i ) {
    string summary  = kEmptyName;
    bool   disabled = true;
    if( slots.contains( i ) ) {
      summary  = slots[i];
      disabled = false;
      if( i >= number_of_normal_slots() )
        summary = "Autosave: " + summary;
    }
    config.options.push_back( { .key = fmt::to_string( i ),
                                .display_name = summary,
                                .disabled     = disabled } );
  }
  while( true ) {
    maybe<string> selection =
        co_await ts.gui.optional_choice( config );
    if( !selection.has_value() ) co_return nothing;
    UNWRAP_CHECK( slot, base::from_chars<int>( *selection ) );
    // We're not allowing to select empty slots.
    if( slots.contains( slot ) ) co_return slot;
    co_await ts.gui.message_box(
        "There is no game saved in this slot." );
  }
}

expect<SlotCopiedPaths> copy_slot_to_slot(
    SSConst const&, TS&, IGameStorageSave const& saver,
    int src_slot, int dst_slot ) {
  fs::path const src_path =
      query_file_for_slot( saver, src_slot );
  fs::path const dst_path =
      query_file_for_slot( saver, dst_slot );

  // Technically this check is redundant because the copy func-
  // tion below will fail in this case, but here we can provide a
  // more meaningful error message.
  if( !fs::exists( src_path ) )
    return fmt::format(
        "failed to locate save file {} for copying to {}.",
        src_path, dst_path );

  HAS_VALUE_OR_RET( base::copy_file_overwriting_destination(
      src_path, dst_path ) );

  return SlotCopiedPaths{ .src = src_path, .dst = dst_path };
}

/****************************************************************
** Saving.
*****************************************************************/
wait<base::NoDiscard<bool>> save_to_slot_interactive(
    SSConst const& ss, TS& ts, IGameStorageSave const& saver,
    int slot ) {
  expect<fs::path> result = save_to_slot( ss, ts, saver, slot );
  if( !result.has_value() ) {
    co_await ts.gui.message_box( "Error: failed to save game." );
    lg.error( "failed to save game: {}.", result.error() );
    co_return false;
  }
  co_await ts.gui.message_box(
      fmt::format( "Successfully saved game to {}.",
                   stem_for_slot( slot ) ) );
  lg.info( "saved game to {}.", stem_for_slot( slot ) );
  co_return true;
}

expect<fs::path> save_to_slot( SSConst const& ss, TS& ts,
                               IGameStorageSave const& saver,
                               int                     slot ) {
  auto const p = query_file_for_slot( saver, slot );
  HAS_VALUE_OR_RET( saver.store( p ) );
  record_checkpoint( ss, ts );
  return p;
}

// The extra unused params are so that we can fit into the IGame-
// Saver module interface.
expect<fs::path> save_to_slot_no_checkpoint(
    SSConst const&, TS&, IGameStorageSave const& saver,
    int slot ) {
  auto const p = query_file_for_slot( saver, slot );
  HAS_VALUE_OR_RET( saver.store( p ) );
  return p;
}

/****************************************************************
** Loading.
*****************************************************************/
wait<base::NoDiscard<bool>> load_from_slot_interactive(
    SS& ss, TS& ts, IGameStorageLoad const& loader, int slot ) {
  expect<fs::path> const result =
      load_from_slot( ss, ts, loader, slot );
  if( !result.has_value() ) {
    co_await ts.gui.message_box( "Error: failed to load game." );
    lg.error( "failed to load game: {}", result.error() );
    co_return false;
  }
  co_await ts.gui.message_box(
      fmt::format( "Successfully loaded game from {}.",
                   stem_for_slot( slot ) ) );
  lg.info( "loaded game from {}.", stem_for_slot( slot ) );
  co_return true;
}

expect<fs::path> load_from_slot( SS& ss, TS& ts,
                                 IGameStorageLoad const& loader,
                                 int                     slot ) {
  fs::path const p = query_file_for_slot( loader, slot );
  if( !fs::exists( p ) )
    return fmt::format( "save file not found for slot {}.",
                        slot );
  HAS_VALUE_OR_RET( loader.load( p ) );
  record_checkpoint( ss, ts );
  return p;
}

} // namespace rn
