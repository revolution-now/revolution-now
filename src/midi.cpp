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
#include "aliases.hpp"
#include "errors.hpp"
#include "fmt-helper.hpp"
#include "init.hpp"
#include "logging.hpp"

// midifile (FIXME)
#include "../extern/midifile/include/MidiFile.h"

// rtmidi
#include "RtMidi.h"

// Abseil
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"

// C++ standard library
#include <vector>

using namespace std;

#define RTMIDI_FATAL( exception_object ) \
  FATAL( "rtmidi fatal error: {}",       \
         exception_object.getMessage() )

namespace rn {

namespace {

/****************************************************************
** Midi Output
*****************************************************************/
RtMidiOut* g_midi_out = nullptr;
Opt<int>   g_midi_output_port;

RtMidiOut& midi_out() {
  if( !g_midi_out ) {
    try {
      g_midi_out = new RtMidiOut();
    } catch( RtMidiError const& error ) {
      RTMIDI_FATAL( error );
    }
  }
  return *g_midi_out;
}

void free_midi_out() { delete g_midi_out; }

vector<pair<int, string>> scan_midi_output_ports() {
  vector<pair<int, string>> res;
  RtMidiOut&                midiout = midi_out();
  try {
    auto num_ports = int( midiout.getPortCount() );
    for( auto i = 0; i < num_ports; i++ )
      res.push_back( {i, midiout.getPortName( i )} );
  } catch( RtMidiError& error ) { RTMIDI_FATAL( error ); }
  return res;
}

bool is_valid_output_port_name( string_view port_name ) {
  return absl::StrContains( absl::AsciiStrToLower( port_name ),
                            "fluid" );
}

Opt<int> find_midi_output_port() {
  auto ports = scan_midi_output_ports();
  logger->info( "found {} midi output ports.", ports.size() );
  Opt<int> res;
  for( auto const& [port, name] : ports ) {
    logger->info( "  MIDI output port #{}: {}", port, name );
    if( !res && is_valid_output_port_name( name ) ) res = port;
  }
  if( !res ) {
    logger->warn(
        "failed to find recognizable midi output port." );
    // Just use first port if we have one.
    if( ports.size() > 0 ) res = 0;
  }
  if( !res ) logger->warn( "no midi output ports available." );
  return res;
}

void init_midi_io() {
  (void)midi_out(); // first time called will allocate.
  g_midi_output_port = find_midi_output_port();
  if( g_midi_output_port )
    logger->info( "using MIDI output port #{}",
                  *g_midi_output_port );
}

void cleanup_midi_io() { free_midi_out(); }

/****************************************************************
** Midi File Reading
*****************************************************************/
void scan_midi_file( string const& file ) {
  smf::MidiFile midifile;
  fmt::print( "File: {}", file );
  CHECK( midifile.read( file ) );
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

} // namespace

REGISTER_INIT_ROUTINE( midi_io, init_midi_io, cleanup_midi_io );

void test_midi() {}

} // namespace rn
