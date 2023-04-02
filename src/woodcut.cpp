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
#include "igui.hpp"

// ss
#include "ss/player.rds.hpp"

using namespace std;

namespace rn {

namespace {

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
namespace detail {

wait<> display_woodcut( IGui& gui, e_woodcut cut ) {
  co_await gui.message_box( "(woodcut): {}",
                            msg_for_woodcut( cut ) );
}

}

wait<> display_woodcut_if_needed( IGui& gui, Player& player,
                                  e_woodcut cut ) {
  if( player.woodcuts[cut] ) co_return;
  // Note that we don't directly call the display_woodcut method
  // above; we want to call it via the IGui interface so that it
  // can be mocked.
  co_await gui.display_woodcut( cut );
  player.woodcuts[cut] = true;
}

} // namespace rn
