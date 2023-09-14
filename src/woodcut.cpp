/****************************************************************
**woodcut.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-01-21.
*
* Description: Displays woodcuts (cut scene images from the OG).
*
*****************************************************************/
#include "woodcut.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "ieuro-mind.hpp"
#include "igui.hpp"

// ss
#include "ss/player.rds.hpp"

using namespace std;

namespace rn {

namespace {

// TODO: Move this to the config files.
string_view msg_for_woodcut( e_woodcut cut ) {
  switch( cut ) {
    case e_woodcut::discovered_new_world:
      return "Discovery of the New World!";
    case e_woodcut::building_first_colony:
      return "Construction of First Colony";
    case e_woodcut::meeting_the_natives:
      return "Meeting the Natives";
    case e_woodcut::meeting_the_aztec_empire:
      return "The Aztec Empire";
    case e_woodcut::meeting_the_inca_nation:
      return "The Inca Nation";
    case e_woodcut::discovered_pacific_ocean:
      return "Discovery of the Pacific Ocean";
    case e_woodcut::entering_native_village:
      return "Entering Native Village";
    case e_woodcut::discovered_fountain_of_youth:
      return "Discovery of the Fountain of Youth!";
    case e_woodcut::cargo_from_the_new_world:
      return "Cargo from the New World";
    case e_woodcut::meeting_fellow_europeans:
      return "Meeting Fellow Europeans";
    case e_woodcut::colony_burning:   //
      return "Colony Burning!";
    case e_woodcut::colony_destroyed: //
      return "Colony Destroyed";
    case e_woodcut::indian_raid:      //
      return "Indian Raid!";
  }
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
wait<> show_woodcut_if_needed( Player& player, IEuroMind& mind,
                               e_woodcut cut ) {
  if( player.woodcuts[cut] ) co_return;
  player.woodcuts[cut] = true;
  co_await mind.show_woodcut( cut );
}

namespace internal {
wait<> show_woodcut( IGui& gui, e_woodcut cut ) {
  // TODO: temporary implementation.
  co_await gui.message_box( "(woodcut): {}",
                            msg_for_woodcut( cut ) );
}
}

} // namespace rn
