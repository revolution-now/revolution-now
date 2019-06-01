/****************************************************************
**midiseq.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-04-20.
*
* Description: Realtime MIDI Sequencer.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "adt.hpp"
#include "enum.hpp"

// C++ standard library.
#include <string>

// The possible commands that can be sent to the midi thread.
// Generally, after receiving one of these commands the midi
// thread will update its state.
//
//   play:   If the player was paused in the middle of a tune
//           then this will cause the tune to resume.  Otherwise
//           it will start playing the next track in the
//           playlist.
//
//   next:   If the player is playing then this will skip to the
//           next track and play it.  If the player is paused
//           then this will skip to the next track *and play it*.
//
//   pause:  Pause the player wherever it is, whether in the
//           middle of a tune or in the middle of tracks.
//
//   off:    Tell the player to turn off.
//
//   volume: Set the master volume from 0.0 to 1.0.  The value
//           1.0 corresponds to the value in the MIDI file, so
//           therefore this can really only be used to lower
//           the volume.  Gain beyond the standard volume would
//           need to be done with the synthesizer.
//
ADT( /*namespace */ rn::midiseq, //
     command,                    //
     ( play ),                   //
     ( next ),                   //
     ( pause ),                  //
     ( off ),                    //
     ( volume,                   //
       ( double, value ) )       //
);

namespace rn::midiseq {

// State held by (and updated by) the midi thread.
enum class e_( midiseq_state,
               playing, //
               paused,  //
               failed,  //
               off      //
);

bool midiseq_enabled();
// We can get state, but not set it. To change the state of the
// midi player you must send it commands.
e_midiseq_state midiseq_state();

bool is_processing_commands();

void send_command( command_t cmd );

// Testing.
void test_midi();

} // namespace rn::midiseq
