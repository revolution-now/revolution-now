/****************************************************************
**midi.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-04-20.
*
* Description: Interface for playing midi files.
*
*****************************************************************/
#include "midi.hpp"

// Revolution Now
#include "errors.hpp"
#include "fmt-helper.hpp"

// midifile (FIXME)
#include "../extern/midifile/include/MidiFile.h"

// rtmidi
#include "RtMidi.h"

using namespace std;

namespace rn {

namespace {} // namespace

void test_midi() {
  try {
    RtMidiOut midi_out;
  } catch( RtMidiError const& error ) {
    // Handle the exception here
    error.printMessage();
  }

  //////////////////////////////////////////////////////////

  smf::MidiFile midifile;
  CHECK( midifile.read( "test.mid" ) );
  midifile.doTimeAnalysis();
  midifile.linkNotePairs();

  int tracks = midifile.getTrackCount();
  fmt::print( "TPQ: {}\n", midifile.getTicksPerQuarterNote() );
  fmt::print( "TRACKS: {}\n", tracks );
  for( int track = 0; track < tracks; track++ ) {
    fmt::print( "\nTrack {}\n", track );
    fmt::print( "-----------------------------------\n" );
    fmt::print( "{:<8}{:<9}{:<10}{}\n", "Tick", "Seconds", "Dur",
                "Message" );
    fmt::print( "-----------------------------------\n" );
    for( int event = 0; event < midifile[track].size();
         event++ ) {
      fmt::print( "{:<8}", midifile[track][event].tick );
      fmt::print( "{:<9}", midifile[track][event].seconds );
      if( midifile[track][event].isNoteOn() )
        fmt::print(
            "{:<10}",
            midifile[track][event].getDurationInSeconds() );
      else
        fmt::print( "{:<10}", "" );
      for( size_t i = 0; i < midifile[track][event].size(); i++ )
        fmt::print( "{:<3x}", (int)midifile[track][event][i] );
      fmt::print( "\n" );
    }
  }
}

} // namespace rn
