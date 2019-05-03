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
#include "time.hpp"

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
  if( g_midi_output_port ) {
    logger->info( "using MIDI output port #{}",
                  *g_midi_output_port );
    // TODO: error checking
    g_midi_out->openPort( *g_midi_output_port );
    auto callback = []( RtMidiError::Type,
                        const std::string& error_text, void* ) {
      logger->warn( "RtMidi: {}", error_text );
    };
    void* userdata = nullptr;
    g_midi_out->setErrorCallback( callback, userdata );
  }
}

void cleanup_midi_io() {
  g_midi_out->closePort();
  free_midi_out();
}

void send_midi_message( smf::MidiEvent const& event ) {
  // TODO get rid of heap allocation
  vector<unsigned char> bytes;
  bytes.resize( event.size() );
  for( size_t i = 0; i < event.size(); ++i ) bytes[i] = event[i];
  try {
    g_midi_out->sendMessage( &bytes );
  } catch( RtMidiError const& error ) { RTMIDI_FATAL( error ); }
}

// Assumes that port is already open.
void play_midi_file( string const& file ) {
  CHECK( g_midi_out );

  smf::MidiFile midifile;
  fmt::print( "File: {}\n", file );
  CHECK( midifile.read( file ) );
  midifile.doTimeAnalysis();
  midifile.linkNotePairs();

  int tracks = midifile.getTrackCount();
  fmt::print( "Tracks: {}\n", tracks );

  int const track = 0;

  midifile.joinTracks();
  fmt::print( "Tracks after join: {}\n",
              midifile.getTrackCount() );

  auto tpq = midifile.getTPQ();
  fmt::print( "Ticks Per Quarter Note: {}\n", tpq );
  auto duration_ticks = midifile.getFileDurationInTicks();
  auto duration_secs  = midifile.getFileDurationInSeconds();
  auto millisecs_per_tick =
      1000.0 * duration_secs / duration_ticks;
  fmt::print( "milliseconds per tick: {}\n",
              millisecs_per_tick );

  fmt::print( "\nTrack {}\n", track );
  fmt::print( "-----------------------------------\n" );
  fmt::print( "{:<8}{:<9}{:<10}{}\n", "Tick", "Seconds", "Dur",
              "Message" );
  fmt::print( "-----------------------------------\n" );
  int tick = 0;
  for( int event = 0; event < midifile[track].size(); event++ ) {
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
    auto const& e     = midifile[track][event];
    int         delta = e.tick - tick;
    using namespace std::chrono;
    auto duration = int( delta * millisecs_per_tick );
    sleep( milliseconds( duration ) );
    // Must send midi message AFTER sleeping.
    send_midi_message( e );
    tick = e.tick;
  }
}

} // namespace

REGISTER_INIT_ROUTINE( midi_io, init_midi_io, cleanup_midi_io );

void test_midi() { play_midi_file( "test.mid" ); }

} // namespace rn
