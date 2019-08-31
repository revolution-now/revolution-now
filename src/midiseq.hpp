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
//   play:   Start playing the given tune from the beginning,
//           even if the player was paused.
//
//   stop:   Stop playing (if playing).  If playing, position in
//           current tune will be lost.  Playing must be resumed
//           with a `play` command.
//
//   pause:  Pause the player wherever it is, whether in the
//           middle of a tune or in the middle of tracks.
//
//   pause:  Resume the player if it was paused.
//
//   off:    Tell the player to turn off.
//
//   volume: Set the master volume from 0.0 to 1.0.  The value
//           1.0 corresponds to the value in the MIDI file, so
//           therefore this can really only be used to lower
//           the volume.  Gain beyond the standard volume would
//           need to be done with the synthesizer.
//
adt_( /*namespace */ rn::midiseq, //
      command,                    //
      ( play,                     //
        ( fs::path, file ) ),     //
      ( stop ),                   //
      ( pause ),                  //
      ( resume ),                 //
      ( off ),                    //
      ( volume,                   //
        ( double, value ) )       //
);

namespace rn::midiseq {

// State held by (and updated by) the midi thread.
enum class e_( midiseq_state,
               playing, //
               paused,  //
               stopped, //
               failed,  //
               off      //
);

bool midiseq_enabled();
// We can get state, but not set it. To change the state of the
// midi player you must send it commands.
e_midiseq_state state();

// If playing a tune, returns [0,1.0] progress.
Opt<double> progress();

bool is_processing_commands();

Opt<Duration_t> can_play_tune( fs::path const& path );

void send_command( command_t cmd );

// Testing.
void test();

} // namespace rn::midiseq
