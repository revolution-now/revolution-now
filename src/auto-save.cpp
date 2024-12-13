/****************************************************************
**auto-save.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-05.
*
* Description: Handles logic related to autosaving.
*
*****************************************************************/
#include "auto-save.hpp"

// Revolution Now
#include "isave-game.rds.hpp"

// config
#include "config/savegame.rds.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/turn.rds.hpp"

// base
#include "base/to-str-ext-std.hpp"

// C++ standard library
#include <ranges>

using namespace std;

namespace rn {

namespace {

using ::std::views::iota;

// Autosave slot indices by convention start after the normal
// slots end. I.e autosave slot 0 is the first autosave slot,
// which, in the OG, would be COLONY08.SAV.
int autosave_to_absolute( int autosave_slot ) {
  int const num_normal_save_slots =
      config_savegame.num_normal_save_slots;
  return num_normal_save_slots + autosave_slot;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
set<int> should_autosave( SSConst const& ss ) {
  set<int> res;
  if( !config_savegame.autosave.enabled ) return res;
  if( !ss.settings.game_options
           .flags[e_game_flag_option::autosave] )
    return res;
  int const curr_turn   = ss.turn.time_point.turns;
  auto const& last_save = ss.turn.autosave.last_save;
  if( last_save.has_value() && *last_save >= curr_turn )
    return res;
  int const num_autosave_slots =
      config_savegame.autosave.slots.size();
  for( int const i : iota( 0, num_autosave_slots ) ) {
    auto const& slot = config_savegame.autosave.slots[i];
    if( !slot.enabled ) continue;
    CHECK_NEQ( slot.frequency, 0 ); // validated by config.
    if( curr_turn % slot.frequency != 0 ) continue;
    if( curr_turn == 0 && !slot.save_on_turn_zero ) continue;
    res.insert( i );
  }
  return res;
}

// This implementation is slightly complicated because we want to
// make sure to avoid serializing the game more than once which
// would expensive and redundant. We want to serialize it once,
// then if there are multiple slots that need to get saved then
// we'll just copy the first file onto the remaining files.
expect<std::vector<fs::path>> autosave(
    SSConst const& ss, IGameSaver const& game_saver,
    Autosave& autosave, set<int> autosave_slots ) {
  auto const initial_slots_size = autosave_slots.size();
  vector<fs::path> res;
  if( autosave_slots.empty() ) return res;

  // Important: need to do this before saving, otherwise when the
  // game gets reloaded it will immediately save again, since
  // after reload the code will retrace over the caller of this
  // method as it did when the save was made.
  autosave.last_save = ss.turn.time_point.turns;

  CHECK_GT( autosave_slots.size(), 0u );
  int const first_absolute_slot =
      autosave_to_absolute( *autosave_slots.begin() );
  autosave_slots.erase( *autosave_slots.begin() );
  // Note that, unlike player-initiated saving, we don't capture
  // a checkpoint of the save state here (which is held in ts.s-
  // tate) because we don't want auto-saves to count as saves in
  // that regard, since otherwise the player would not be
  // prompted to save the game on exit if it had just been
  // auto-saved.
  UNWRAP_RETURN( first_path,
                 game_saver.save_to_slot_no_checkpoint(
                     first_absolute_slot ) );
  res.push_back( first_path );

  while( !autosave_slots.empty() ) {
    int const dst_autosave_slot = *autosave_slots.begin();
    autosave_slots.erase( *autosave_slots.begin() );
    int const dst_absolute_slot =
        autosave_to_absolute( dst_autosave_slot );
    CHECK_NEQ( first_absolute_slot, dst_absolute_slot );
    UNWRAP_RETURN(
        paths, game_saver.copy_slot_to_slot(
                   first_absolute_slot, dst_absolute_slot ) );
    auto const& [src_path, dst_path] = paths;
    CHECK_EQ( src_path, first_path );
    res.push_back( dst_path );
  }

  CHECK_EQ( res.size(), initial_slots_size );
  return res;
}

} // namespace rn
