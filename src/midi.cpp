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
#include "enum.hpp"
#include "errors.hpp"
#include "fmt-helper.hpp"
#include "init.hpp"
#include "logging.hpp"
#include "rand.hpp"
#include "time.hpp"

// midifile (FIXME)
#include "../extern/midifile/include/MidiFile.h"

// rtmidi
#include "RtMidi.h"

// Abseil
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"

// C++ standard library
#include <set>
#include <thread>
#include <vector>

using namespace std;

#define RTMIDI_FATAL( exception_object ) \
  FATAL( "rtmidi fatal error: {}",       \
         exception_object.getMessage() )

#define RTMIDI_WARN( exception_object ) \
  logger->warn( "rtmidi warning: {}",   \
                exception_object.getMessage() )

namespace rn {

namespace {

/****************************************************************
** Midi I/O
*****************************************************************/
// Apart from initialization and cleanup (by the main thread)
// this g_midi_out should only be used by the midi thread since
// it is unknown whether it is thread safe.
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

// Apparently there is a midi message called "all notes off", but
// it is not clear that it works in the same way on all devices.
// So just to be sure, we implement this by sending a separate
// "Note Off" message for every note on every channel.
void all_notes_off() {
  vector<uint8_t> message;
  message.resize( 3 );

  for( uint8_t channel = 0; channel < 16; ++channel ) {
    for( uint8_t note = 0; note < 128; ++note ) {
      // Note Off:
      //   1000nnnn : nnnn    = channel
      //   0kkkkkkk : kkkkkkk = note
      //   0vvvvvvv : vvvvvvv = velocity
      message[0] = 128 + channel;
      message[1] = 0 + note;
      message[2] = 0 + 127;
      try {
        g_midi_out->sendMessage( &message );
      } catch( RtMidiError const& error ) {
        RTMIDI_WARN( error );
      }
    }
  }
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

/****************************************************************
** Midi Thread
*****************************************************************/
// Will be left as nullopt if the midi subsystem fails to ini-
// tialize. Music errors are generally not fatal for the game.
Opt<thread> g_midi_thread;

// TODO: separate this into state and commands.
enum class e_( midi_cmd,
               play,   //
               pause,  //
               failed, //
               exit    //
);

// Class used for communicating with the midi thread in a thread
// safe way.
class MidiCommunication {
public:
  MidiCommunication() = default;

  e_midi_cmd state() {
    lock_guard<mutex> lock( mutex_ );
    return state_;
  }

  void set_state( e_midi_cmd new_state ) {
    lock_guard<mutex> lock( mutex_ );
    state_ = new_state;
  }

  void set_playlist( set<string> const& files ) {
    lock_guard<mutex> lock( mutex_ );
    playlist_.clear();
    for( auto const& f : files ) playlist_.push_back( f );
  }

  vector<string> playlist() {
    lock_guard<mutex> lock( mutex_ );
    return playlist_; // copy
  }

  // Return copy for thread safety (we don't want the caller to
  // have a reference to any data inside this class).
  string last_error() {
    lock_guard<mutex> lock( mutex_ );
    return last_error_;
  }

  void set_last_error( string error ) {
    lock_guard<mutex> lock( mutex_ );
    last_error_ = std::move( error );
  }

private:
  mutex          mutex_;
  e_midi_cmd     state_{e_midi_cmd::pause};
  vector<string> playlist_{};
  string         last_error_{};
};

// Shared state between main thread and midi thread.
MidiCommunication g_midi_comm;

/****************************************************************
** Init/Cleanup
*****************************************************************/
struct MidiPlayInfo {
  smf::MidiFile midifile;
  int           current_event;
  double        millisecs_per_tick;
  int           tick;
};

// May fail to load the file.
Opt<MidiPlayInfo> load_midi_file( string const& file ) {
  if( !g_midi_out ) {
    logger->warn(
        "attempting to load midi file when "
        "g_midi_out not initialized!" );
    return nullopt;
  }
  MidiPlayInfo info;

  info.midifile.clear();
  logger->info( "loading midi file: {}\n", file );
  if( !info.midifile.read( file ) ) return nullopt;
  info.midifile.doTimeAnalysis(); // why do we need this?
  info.midifile.linkNotePairs();  // why do we need this?

  info.midifile.joinTracks();
  // Should now have precisely one track.

  auto duration_ticks = info.midifile.getFileDurationInTicks();
  auto duration_secs  = info.midifile.getFileDurationInSeconds();
  info.millisecs_per_tick =
      1000.0 * duration_secs / duration_ticks;
  info.current_event = 0;
  info.tick          = 0;
  return info;
}

// Plays a single midi event and waits first if necessary.
void midi_play_event( MidiPlayInfo& info ) {
  // Always use track 0 because on loading the midi files we join
  // all tracks into one.
  if( info.current_event >= info.midifile[0].size() ) return;
  auto const& e     = info.midifile[0][info.current_event];
  int         delta = e.tick - info.tick;
  using namespace std::chrono;
  auto duration = int( delta * info.millisecs_per_tick );
  sleep( milliseconds( duration ) );
  // Must send midi message AFTER sleeping.
  send_midi_message( e );
  info.tick = e.tick;
  info.current_event++;
}

template<typename... Args>
void fail_midi_thread( Args... args ) {
  g_midi_comm.set_state( e_midi_cmd::failed );
  g_midi_comm.set_last_error(
      fmt::format( std::forward<Args>( args )... ) );
}

void midi_thread() {
  Opt<MidiPlayInfo> maybe_info;
  int               track = -1;
  while( true ) {
    switch( g_midi_comm.state() ) {
      case +e_midi_cmd::failed: all_notes_off(); return;
      case +e_midi_cmd::exit: all_notes_off(); return;
      case +e_midi_cmd::pause:
        all_notes_off();
        sleep( chrono::milliseconds( 200 ) );
        continue;
      case +e_midi_cmd::play: {
        auto playlist = g_midi_comm.playlist();
        if( !maybe_info.has_value() ) {
          // Try loading the next track.
          if( playlist.size() == 0 ) {
            // There are no songs in the playlist, so just go
            // into the pause state.
            logger->warn(
                "no midi files in playlist; stopping player." );
            g_midi_comm.set_state( e_midi_cmd::pause );
            break;
          }
          // We have at least one track in the playlist.
          track++;
          track      = track % playlist.size();
          maybe_info = load_midi_file( playlist[track] );
          if( !maybe_info ) {
            fail_midi_thread( "failed to load midi file {}",
                              playlist[track] );
            break;
          }
          // We've loaded the midi file.
        }
        if( maybe_info.has_value() ) {
          auto& info = maybe_info.value();
          midi_play_event( info );
          // If this midi file has finished playing then signal
          // that we should load the next one.
          if( info.current_event >= info.midifile[0].size() ) {
            logger->debug( "midi file {} has finished.",
                           playlist[track] );
            // Just for good measure.
            all_notes_off();
            maybe_info = nullopt;
          }
        }
        break;
      }
    }
  }
}

void init_midi() {
  // Initilization of rt-midi i/o mechanism.
  (void)midi_out(); // first time called will allocate.
  g_midi_output_port = find_midi_output_port();
  if( g_midi_output_port ) {
    logger->info( "using MIDI output port #{}",
                  *g_midi_output_port );
    // TODO: error checking
    g_midi_out->openPort( *g_midi_output_port );
    // This may be called from the midi thread.
    auto callback = []( RtMidiError::Type,
                        const std::string& error_text, void* ) {
      logger->warn( "RtMidi: {}", error_text );
    };
    void* userdata = nullptr;
    g_midi_out->setErrorCallback( callback, userdata );

    logger->info( "creating MIDI thread." );
    // Initialization of midi thread. This is only done if we
    // found a midi port.
    g_midi_thread = thread( []() { midi_thread(); } );
  }
}

void cleanup_midi() {
  if( g_midi_output_port ) {
    // Cleanup midi thread.
    if( g_midi_thread.has_value() ) {
      logger->debug(
          "sending `exit` message to midi thread..." );
      g_midi_comm.set_state( e_midi_cmd::exit );
      logger->debug( "waiting for midi thread to join..." );
      g_midi_thread->join();
      logger->debug( "midi thread joined." );
      // This may have already been done by the midi thread, but
      // just in case...
      all_notes_off();
    }

    // Cleanup rt-midi i/o.
    g_midi_out->closePort();
    free_midi_out();
  }
}

} // namespace

REGISTER_INIT_ROUTINE( midi, init_midi, cleanup_midi );

// Just for testing. Assumes that port is already open.
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

void test_midi() {
  g_midi_comm.set_playlist(
      {"test.mid", "test2.mid", "test3.mid"} );
  while( true ) {
    fmt::print( "Enter command [play,pause,quit]: " );
    string in;
    cin >> in;
    if( in == "play" ) {
      g_midi_comm.set_state( e_midi_cmd::play );
      continue;
    }
    if( in == "pause" ) {
      g_midi_comm.set_state( e_midi_cmd::pause );
      continue;
    }
    if( in == "quit" ) break;
  }
}

} // namespace rn
