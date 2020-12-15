/****************************************************************
**midiseq.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-04-20.
*
* Description: Realtime MIDI Sequencer.
*
*****************************************************************/
#include "midiseq.hpp"

// Revolution Now
#include "errors.hpp"
#include "fmt-helper.hpp"
#include "init.hpp"
#include "logging.hpp"
#include "rand.hpp"
#include "ranges-fwd.hpp"
#include "time.hpp"

// base-util
#include "base-util/io.hpp"

// midifile (FIXME)
#include "../extern/midifile/include/MidiFile.h"

// rtmidi
#include "RtMidi.h"

// Abseil
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"

// Range-v3
#include "range/v3/algorithm/any_of.hpp"

// C++ standard library
#include <algorithm>
#include <queue>
#include <set>
#include <thread>
#include <vector>

using namespace std;
using namespace std::chrono;
using namespace std::literals::chrono_literals;

#define RTMIDI_WARN( exception_object ) \
  lg.warn( "rtmidi warning: {}", exception_object.getMessage() )

namespace rn::midiseq {

namespace {

/****************************************************************
** Midi I/O
*****************************************************************/
void rtmidi_error_callback( RtMidiError::Type,
                            string const& error_text,
                            void* /*unused*/ );

// This class is a wrapper around the RtMidiOut object and han-
// dles creating and initializing it. Initialization consists of
// finding an appropriate midi output port and opening that port.
//
// This object can only be created with the static `create`
// method. If said method succeeds to return a MidiIO object then
// that means that MIDI music can and should be playable.
class MidiIO {
public:
  NON_COPYABLE( MidiIO );

  MidiIO( MidiIO&& rhs ) noexcept {
    out_ = std::exchange( rhs.out_, nullptr );
  }

  MidiIO& operator=( MidiIO&& rhs ) noexcept {
    rhs.swap( *this );
    return *this;
  }

  void swap( MidiIO& rhs ) noexcept { rhs.out_.swap( out_ ); }

  ~MidiIO() {
    if( out_ ) out_->closePort();
  }

  static maybe<MidiIO> create() {
    MidiIO res;
    try {
      res.out_.reset( new RtMidiOut() );
    } catch( RtMidiError const& error ) {
      RTMIDI_WARN( error );
      return nothing;
    }
    auto maybe_port = res.find_midi_output_port();
    if( !maybe_port ) return nothing;
    // This may be called from the midi thread.
    res.out_->setErrorCallback( rtmidi_error_callback,
                                /*userdata=*/nullptr );

    try {
      res.out_->openPort( *maybe_port );
    } catch( RtMidiError const& error ) {
      RTMIDI_WARN( error );
      lg.warn( "failed to open MIDI output port {}." );
      return nothing;
    }
    lg.info( "using MIDI output port #{}", *maybe_port );
    return res;
  }

  void send_midi_message( smf::MidiEvent const& event ) {
    size_t const max_event_size = 200;
    // Should be more than large enough for any event. Use a
    // static vector so that we don't have to do a heap alloca-
    // tion on each event.
    static vector<unsigned char> bytes = [&]() {
      vector<unsigned char> res;
      res.resize( max_event_size );
      return res;
    }();
    if( event.size() > max_event_size ) {
      // Should be extremely rare.
      lg.warn( "midi event size {} is larger than max (={})",
               event.size(), max_event_size );
      return;
    }
    for( size_t i = 0; i < event.size(); ++i )
      bytes[i] = event[i];
    send_midi_message( &bytes[0], event.size() );
  }

  // Apparently there is a midi message called "all notes off",
  // but it is not clear that it works in the same way on all
  // devices. So just to be sure, we implement this by sending a
  // separate "Note Off" message for every note on every channel.
  void all_notes_off() {
    uint8_t message[3];

    for( uint8_t channel = 0; channel < 16; ++channel ) {
      for( uint8_t note = 0; note < 128; ++note ) {
        // Note Off:
        //   1000nnnn : nnnn    = channel
        //   0kkkkkkk : kkkkkkk = note
        //   0vvvvvvv : vvvvvvv = velocity
        message[0] = 128 + channel;
        message[1] = 0 + note;
        message[2] = 0 + 127;
        send_midi_message( message, 3 );
      }
    }
  }

  // Called in response to sending a `set volume` command to the
  // midi thread.
  void set_master_volume( double vol ) {
    master_volume_ = std::clamp( vol, 0.0, 1.0 );
    update_volumes();
  }

  // Set all channel volumes to max, as if the MIDI file had re-
  // quested it. This is to accommodate MIDI files that don't set
  // their own channel volumes. But note that we do not change
  // the master volume in this function.
  void reset_channel_volumes() {
    for( uint8_t channel = 0; channel < 16; channel++ )
      midi_requested_volumes_[channel] = 127;
    update_volumes();
  }

private:
  MidiIO() = default;

  double             master_volume_{ 1.0 };
  array<uint8_t, 16> midi_requested_volumes_{}; // init to zeros

  vector<unsigned char> last_message_;

  void update_volumes() {
    for( uint8_t channel = 0; channel < 16; channel++ ) {
      uint8_t vol = std::clamp(
          std::lround( master_volume_ *
                       midi_requested_volumes_[channel] ),
          0l, 127l );
      // Should not call `send_midi_message` here otherwise we
      // could get an infinite loop. Must call only functions
      // that call `send_midi_message_impl`.
      set_volume_impl( channel, vol );
    }
  }

  // This one does some filtering of midi messages.
  void send_midi_message( unsigned char* bytes, size_t size ) {
    if( size == 0 ) return;

    if( bytes[0] == 0xff ) {
      // These messages that start with 0xff are "meta" MIDI mes-
      // sages that are intended not for the synthesizer but for
      // the MIDI sequencer itself. It doesn't appear that we
      // need to process them, so here we just ignore them (if we
      // were to send them to the synth we'd get an error). See
      // https://github.com/craigsapp/midifile/issues/67.
      return;
    }

    // Each time a "volume set" MIDI messages passes through we
    // intercept the message, record the volume that is requested
    // for that channel (16 channels in MIDI), and instead scale
    // it by the user-defined master volume. This way when the
    // user wishes to scale the master volume we can scale each
    // channel in proportion. Furthermore, if the MIDI track
    // changes the volume midway through it will respect the
    // master volume.
    if( size == 3 && ( bytes[0] & 0xf0 ) == 0xB0 &&
        bytes[1] == 0x07 ) {
      // We have a volume set message.
      auto channel                     = bytes[0] & 0x0f;
      auto vol                         = bytes[2];
      midi_requested_volumes_[channel] = vol;
      update_volumes(); // Send the messages
      return;
    }

    send_midi_message_impl( bytes, size );
  }

  void set_volume_impl( uint8_t channel, uint8_t vol ) {
    uint8_t message[3];
    message[0] = 0xB0 + channel;
    message[1] = 0x07;
    message[2] = vol;
    send_midi_message_impl( message, 3 );
  }

  void send_midi_message_impl( unsigned char* bytes,
                               size_t         size ) {
    // Save the message for debugging purposes.
    last_message_.resize( size );
    for( size_t i = 0; i < size; ++i )
      last_message_[i] = bytes[i];

    int num_retries = 3;
    for( int i = 0; i < num_retries; ++i ) {
      try {
        out_->sendMessage( bytes, size );
        return;
      } catch( RtMidiError const& error ) {
        if( i < num_retries - 1 ) {
          sleep( milliseconds( 2 ) );
          continue;
        }
        RTMIDI_WARN( error );
      }
    }
  }

  vector<pair<int, string>> scan_midi_output_ports() {
    vector<pair<int, string>> res;
    try {
      auto num_ports = int( out_->getPortCount() );
      for( auto i = 0; i < num_ports; i++ )
        res.push_back( { i, out_->getPortName( i ) } );
    } catch( RtMidiError& error ) { RTMIDI_WARN( error ); }
    return res;
  }

  bool is_valid_output_port_name( string_view port_name ) {
    auto valid = [&]( auto const& n ) {
      return absl::StrContains(
          absl::AsciiStrToLower( port_name ), n );
    };
    return rg::any_of(
        initializer_list<char const*>{ "fluid", "timidity" },
        valid );
  }

  maybe<int> find_midi_output_port() {
    auto ports = scan_midi_output_ports();
    lg.info( "found {} midi output ports.", ports.size() );
    maybe<int> res;
    for( auto const& [port, name] : ports ) {
      lg.info( "  MIDI output port #{}: {}", port, name );
      if( !res && is_valid_output_port_name( name ) ) res = port;
    }
    if( !res ) {
      if( ports.size() == 0 )
        lg.warn( "no midi output ports available." );
      else
        lg.warn(
            "failed to find recognizable midi output port." );
      return nothing;
    }
    return res;
  }

  friend void rtmidi_error_callback( RtMidiError::Type,
                                     string const& error_text,
                                     void* /*unused*/ );

  // Apart from initialization and cleanup (by the main thread)
  // this should only be used by the midi thread since it is un-
  // known whether it is thread safe.
  unique_ptr<RtMidiOut> out_{};
};
NOTHROW_MOVE( MidiIO );

// This will be nothing if midi music cannot be played for any
// reason.
maybe<MidiIO> g_midi;

void rtmidi_error_callback( RtMidiError::Type,
                            string const& error_text,
                            void* /*unused*/ ) {
  lg.warn( "rt-midi: {}", error_text );
  if( absl::StrContains( error_text, "event parsing error" ) ) {
    if( !g_midi.has_value() ) {
      lg.critical(
          "programmer error: should not be here ({}:{})",
          __FILE__, __LINE__ );
      return;
    }
    MidiIO& mio = g_midi.value();
    string  lms = "last message sent:";
    for( size_t i = 0; i < mio.last_message_.size(); i++ )
      lms += fmt::format( " {:<3x}", mio.last_message_[i] );
    lg.debug( "{}", lms );
  }
}

/****************************************************************
** Midi Thread
*****************************************************************/
// Will be left as nothing if the midi subsystem fails to ini-
// tialize. Music errors are generally not fatal for the game.
maybe<thread> g_midi_thread;

// Class used for communicating with the midi thread in a thread
// safe way. Note that the methods here return things by copy for
// thread safety (we don't want the caller to have a reference to
// any data inside this class).
class MidiCommunication {
public:
  MidiCommunication() = default;
  NO_COPY_NO_MOVE( MidiCommunication );

  // ************************************************************
  // Interface for Any Thread
  // ************************************************************
  e_midiseq_state state() {
    lock_guard<mutex> lock( mutex_ );
    return state_;
  }

  // ************************************************************
  // Interface for Main Thread
  // ************************************************************
  void send_cmd( command_t cmd ) {
    lock_guard<mutex> lock( mutex_ );
    commands_.push( cmd );
  }

  string last_error() const {
    lock_guard<mutex> lock( mutex_ );
    return last_error_;
  }

  // This returns false iff there are no commands queued and no
  // previous commands are still being processed.
  bool processing_commands() const {
    lock_guard<mutex> lock( mutex_ );
    return !commands_.empty() || running_commands_;
  }

  // If in a tune, it will return a double in [0,1] indicating
  // how far it is through the tune.
  maybe<double> progress() const {
    lock_guard<mutex> lock( mutex_ );
    return progress_;
  }

  // ************************************************************
  // Interface for MIDI Thread
  // ************************************************************
  void set_state( e_midiseq_state new_state ) {
    lock_guard<mutex> lock( mutex_ );
    state_ = new_state;
  }

  void set_running_commands( bool running_commands ) {
    lock_guard<mutex> lock( mutex_ );
    running_commands_ = running_commands;
  }

  bool has_commands() const {
    lock_guard<mutex> lock( mutex_ );
    return !commands_.empty();
  }

  void set_last_error( string error ) {
    lock_guard<mutex> lock( mutex_ );
    last_error_ = std::move( error );
  }

  void set_progress( maybe<double> progress ) {
    lock_guard<mutex> lock( mutex_ );
    progress_ = progress;
  }

  maybe<command_t> pop_cmd() {
    lock_guard<mutex> lock( mutex_ );
    if( !commands_.empty() ) {
      auto ret = commands_.front();
      commands_.pop();
      return ret;
    }
    return nothing;
  }

private:
  mutable mutex mutex_;
  // The midi thread holds its state here and updates this when-
  // ever it changes.
  e_midiseq_state  state_{ e_midiseq_state::stopped };
  string           last_error_{};
  queue<command_t> commands_{};
  maybe<double>    progress_{};
  bool             running_commands_{ false };
};

// Shared state between main thread and midi thread.
MidiCommunication g_midi_comm;

// This data structure holds the state of a midi file that is
// being played. It will be updated on each event and possibly
// between events as well. It is used so that we can play the
// midi file in a non-blocking way by playing one event at a time
// and then doing other things, then resuming playing using the
// data in this struct. Even the waiting between notes is done in
// small intervals to avoid blocking.
struct MidiPlayInfo {
  smf::MidiFile midifile;
  int           current_event;
  int           track;
  Time_t        start_time;
  maybe<Time_t> last_pause_time;
  Duration_t    stoppage;
  milliseconds  tune_duration;
};

// May fail to load the file. NOTE: this method must be callable
// from both the main thread and the MIDI thread simultaneously,
// so it should not change any state of the world apart from log-
// ging.
maybe<MidiPlayInfo> load_midi_file( fs::path const& file ) {
  MidiPlayInfo info;
  // TODO: check if midifile.read() is thread safe.
  if( !info.midifile.read( file ) ) return nothing;
  // This will cause the MidiEvent::seconds fields to be popu-
  // lated with the time at which an event should be sent to the
  // synth. And it does take into account meta events that cause
  // tempo changes.
  info.midifile.doTimeAnalysis();
  info.midifile.linkNotePairs(); // why do we need this?

  // Join/merge all tracks into one, otherwise we'd have to worry
  // about writing an algorithm that can play two tracks at once.
  info.midifile.joinTracks();

  info.tune_duration = from_seconds<milliseconds>(
      info.midifile.getFileDurationInSeconds() );
  info.current_event   = 0;
  info.track           = 0;
  info.start_time      = Clock_t::now();
  info.last_pause_time = nothing;
  info.stoppage        = 0us;
  lg.info( "loaded midi file: {} ({})", file,
           info.tune_duration );
  return info;
}

// Plays a single midi event and waits first if necessary.
void midi_play_event( MidiPlayInfo* info ) {
  // Always use track 0 because on loading the midi files we join
  // all tracks into one.
  if( info->current_event >=
      info->midifile[info->track].size() ) {
    // Finished playing this song.
    return;
  }

  auto const& e =
      info->midifile[info->track][info->current_event];

  // Get the time (from the start of the tune) at which this
  // event should be played.
  auto event_time_delta = from_seconds<Duration_t>( e.seconds );

  // This will yield the correct time taking into account any
  // amount of time that we've been paused during playing.
  event_time_delta += info->stoppage;

  // Convert to absolute time.
  auto event_time = info->start_time + event_time_delta;

  // How long do we have to wait before sending the event.
  auto wait_time = event_time - Clock_t::now();

  // Just in case the computer goes to sleep for a while then
  // wakes up, the `wait_time` will be a large negative value. In
  // this case we don't want to play any more events/sounds be-
  // cause then we'd end up playing all the rest of the notes in
  // the tune in an instant and it will produce an unpleasant
  // sound. This will have the effect of basically ending the
  // tune.
  auto time_delta_indicating_machine_went_2_sleep = 30s;
  if( wait_time < -time_delta_indicating_machine_went_2_sleep ) {
    info->current_event++;
    return;
  }

  // As a consequence of using clock time, note that this dura-
  // tion might be (slightly) negative at times, so clamp it.
  wait_time = std::max( Duration_t( 0 ), wait_time );

  // Sometimes some corrupt or incorrectly-composed MIDI files
  // can have abnormally long wait times in them, so avoid that.
  auto max_wait = 10s;
  if( wait_time > max_wait ) {
    lg.warn(
        "long waiting duration encountered: {}s.  Skipping.",
        duration_cast<seconds>( wait_time ).count() );
    // Must be smaller than max_blocking_wait below. So just make
    // it zero for simplicity.
    wait_time = 0s;
  }

  // Now wait before sending the next MIDI message. If we are
  // being asked to wait an amount of time larger than
  // `max_blocking_wait` before the next MIDI message then we
  // will split up the waiting into chunks of size `max_wait` so
  // that we can do it in a non-blocking way. E.g., if there is a
  // five second wait between two notes then we will wait five
  // seconds but we will return from this function every 200ms to
  // avoid blocking.
  Duration_t max_blocking_wait = 200000us; // 0.2 seconds
  wait_time = std::min( max_blocking_wait, wait_time );
  // Just in case there's some overhead in calling sleep_for.
  sleep( wait_time );

  if( wait_time == max_blocking_wait ) {
    // Must return here because we likely have more time to wait,
    // but we want to return so that we don't block for too long.
    return;
  }

  // Must send midi message AFTER sleeping; this is because the
  // duration that we have calculated above is the amount of time
  // we have to wait until we reach the absolute time
  // (event_time) where this new message is suppose to be sent.
  g_midi->send_midi_message( e );
  info->current_event++;
}

// Called by the MIDI thread when it wants to abort due to an
// error.
template<typename... Args>
void midi_thread_record_failure( Args... args ) {
  g_midi_comm.set_state( e_midiseq_state::failed );
  lg.error( std::forward<Args>( args )... );
  g_midi_comm.set_last_error(
      fmt::format( std::forward<Args>( args )... ) );
}

// The MIDI thread will just hang in this function for its entire
// lifetime until it is terminated.
void midi_thread_impl() {
  maybe<MidiPlayInfo> maybe_info;
  maybe<command_t>    cmd;
  maybe<fs::path>     stem;
  bool                time_to_go = false;
  while( true ) {
    // If there are commands to process then mark that we are
    // running commands. Need to do this before entering into the
    // while loop because, by the time we are there, the (pos-
    // sibly single) command in the queue will have been popped.
    //
    // If this is set to true then it should (and will) remain
    // true until this outter while loop comes back again to this
    // point. I.e., in order for the commands to be considered
    // fully "run" we need to finish this while loop just below
    // and then thw switch statement below it.
    g_midi_comm.set_running_commands(
        g_midi_comm.has_commands() );
    while( ( cmd = g_midi_comm.pop_cmd() ).has_value() ) {
      switch( cmd.value().to_enum() ) {
        case command::e::play: {
          auto& [file] = cmd.value().get<command::play>();
          g_midi_comm.set_state( e_midiseq_state::playing );
          g_midi.value().all_notes_off();
          // ****************************************************
          // FIXME: This reset is needed to avoid memory manage-
          // ment bugs in the smf::MidiFile move assignment oper-
          // ator. Fix is to trigger destructor before moving.
          // This can be removed once the issue is fixed. See:
          //   https://github.com/craigsapp/midifile/issues/69
          maybe_info.reset();
          // ****************************************************
          maybe_info = load_midi_file( file );
          if( !maybe_info ) {
            midi_thread_record_failure(
                "failed to load midi file {}", file );
            break;
          }
          // We've loaded the midi file. Now set all channel vol-
          // umes to maximum. This is because some MIDI files
          // (maybe old ones?) do not send MIDI events to set
          // their own volume, and so therefore they would play
          // at whatever the channel volumes currently are, which
          // we don't want.
          g_midi.value().reset_channel_volumes();
          stem = file.stem();
          break;
        }
        case command::e::stop: {
          g_midi.value().all_notes_off();
          g_midi_comm.set_state( e_midiseq_state::stopped );
          if( stem.has_value() )
            lg.info( "midi file {} has been stopped.", stem );
          maybe_info = nothing;
          stem       = nothing;
          g_midi_comm.set_progress( nothing );
          break;
        }
        case command::e::pause: {
          if( g_midi_comm.state() == e_midiseq_state::paused )
            break;
          g_midi.value().all_notes_off();
          g_midi_comm.set_state( e_midiseq_state::paused );
          if( maybe_info.has_value() )
            maybe_info.value().last_pause_time = Clock_t::now();
          break;
        }
        case command::e::resume: {
          if( !maybe_info.has_value() ) break;
          if( g_midi_comm.state() != e_midiseq_state::paused )
            break;
          // We're paused playing a tune.
          g_midi_comm.set_state( e_midiseq_state::playing );
          // We need to add the "missing" time into the stoppage
          // so that we don't skip over all the intervening MIDI
          // events, since the sequencer uses absolute time.
          auto& info = *maybe_info;
          if( info.last_pause_time.has_value() ) {
            info.stoppage +=
                Clock_t::now() - info.last_pause_time.value();
            info.last_pause_time = nothing;
          }
          break;
        }
        case command::e::off: {
          g_midi_comm.set_progress( nothing );
          time_to_go = true;
          stem       = nothing;
          break;
        }
        case command::e::volume: {
          auto& [value] = cmd.value().get<command::volume>();
          g_midi.value().set_master_volume( value );
          break;
        }
      }
    }
    if( time_to_go ) return;
    switch( g_midi_comm.state() ) {
      case e_midiseq_state::failed: return;
      case e_midiseq_state::off:
        lg.critical(
            "programmer error: should not be here ({}:{})",
            __FILE__, __LINE__ );
        return;
      case e_midiseq_state::paused:
      case e_midiseq_state::stopped:
        sleep( milliseconds( 100 ) );
        continue;
      case e_midiseq_state::playing: {
        // maybe_info should have a value at this point.
        if( !maybe_info.has_value() ) {
          lg.critical(
              "programmer error: should not be here ({}:{})",
              __FILE__, __LINE__ );
          midi_thread_record_failure(
              "MIDI Sequencer has experienced an internal "
              "error." );
          return;
        }

        auto& info = maybe_info.value();

        // Update progress meter.
        double progress =
            double( duration_cast<milliseconds>(
                        Clock_t::now() -
                        ( info.start_time + info.stoppage ) )
                        .count() ) /
            info.tune_duration.count();
        progress = std::clamp( progress, 0.0, 1.0 );
        g_midi_comm.set_progress( progress );

        midi_play_event( &info ); // play a single event.
        // If this midi file has finished playing then signal
        // that we should load the next one.
        if( info.current_event >=
            info.midifile[info.track].size() ) {
          lg.info( "midi file {} has finished.", stem );
          g_midi->all_notes_off();
          maybe_info = nothing;
          g_midi_comm.set_progress( nothing );
          g_midi_comm.set_state( e_midiseq_state::stopped );
        }
        break;
      }
    }
  }
}

// This is the function that drives the midi thread (i.e., it is
// given to the std::thread object). When this function finishes
// so does the MIDI thread.
void midi_thread() {
  midi_thread_impl();
  g_midi_comm.set_running_commands( false );
  lg.info( "midi thread exiting." );
  // This may already have been done, but just in case.
  g_midi->all_notes_off();
  if( g_midi_comm.state() == e_midiseq_state::failed ) {
    lg.error( "midi thread failed: {}",
              g_midi_comm.last_error() );
    g_midi.reset();
    // Not sure the best way to do this, but it seems that we get
    // a crash when destroying the std::thread object if the
    // thread has exited and we haven't called join(). Since here
    // we are still in the MIDI thread we probably shouldn't call
    // join, so we will call detach instead.
    g_midi_thread.value().detach();
    return; // exit thread
  }
  g_midi_comm.set_state( e_midiseq_state::off );
}

/****************************************************************
** Init/Cleanup
*****************************************************************/
void init_midiseq() {
  // Initilization of rt-midi i/o mechanism.
  auto maybe_midi_io = MidiIO::create();
  if( maybe_midi_io ) {
    g_midi.emplace( std::move( *maybe_midi_io ) );

    lg.info( "creating midi thread." );
    // Initialization of midi thread. This is only done if we
    // found a midi port.
    g_midi_thread = thread( midi_thread );
  }

  if( !g_midi.has_value() ) {
    lg.warn(
        "failed to initialize MIDI system; MIDI music will not "
        "play." );
  }
}

void cleanup_midiseq() {
  if( g_midi ) {
    // Cleanup midi thread.
    if( g_midi_thread.has_value() ) {
      lg.info( "sending `off` message to midi thread." );
      g_midi_comm.send_cmd( command::off{} );
      lg.info( "waiting for midi thread to join." );
      g_midi_thread->join();
      lg.info( "midi thread closed." );
      // This may have already been done by the midi thread, but
      // just in case...
      g_midi->all_notes_off();
    }

    // Cleanup rt-midi i/o.
    g_midi.reset();
  }
}

} // namespace

REGISTER_INIT_ROUTINE( midiseq );

/****************************************************************
** User API
*****************************************************************/
bool midiseq_enabled() {
  // These should usually always be the same (either both true or
  // both false, but could be different if some kind of internal
  // error happens).
  return g_midi.has_value() && g_midi_thread.has_value();
}

e_midiseq_state state() { return g_midi_comm.state(); }

bool is_processing_commands() {
  return g_midi_comm.processing_commands();
}

void send_command( command_t cmd ) {
  if( midiseq_enabled() )
    g_midi_comm.send_cmd( cmd );
  else
    lg.warn(
        "MIDI is not playable but MIDI commands are being "
        "received." );
}

maybe<Duration_t> can_play_tune( fs::path const& path ) {
  auto info = load_midi_file( path.string() );
  if( info.has_value() ) return info->tune_duration;
  return nothing;
}

maybe<double> progress() { return g_midi_comm.progress(); }

/****************************************************************
** Testing
*****************************************************************/
void test() {
  if( !g_midi ) return;
  vector<fs::path> midi_files;
  for( auto const& file : util::wildcard(
           "assets/music/midi/*.mid", /*with_folders=*/false ) )
    midi_files.push_back( file );
  double vol = 0.5;
  g_midi_comm.send_cmd( command::volume{ vol } );
  sleep( 500ms );
  while( true ) {
    if( g_midi_comm.state() == e_midiseq_state::failed ) {
      lg.info(
          "MIDI thread has failed and is no longer active." );
      break;
    }
    lg.info(
        "[p]lay next, p[a]use, [r]esume, [s]top, [u]p volume, "
        "[d]own volume, [P]rogress, "
        "[q]uit: " );
    string in;
    cin >> in;
    sleep( milliseconds( 20 ) );
    if( in == "p" ) {
      if( midi_files.empty() ) break;
      g_midi_comm.send_cmd( command::play{ midi_files.back() } );
      midi_files.pop_back();
      continue;
    }
    if( in == "a" ) {
      g_midi_comm.send_cmd( command::pause{} );
      continue;
    }
    if( in == "r" ) {
      g_midi_comm.send_cmd( command::resume{} );
      continue;
    }
    if( in == "s" ) {
      g_midi_comm.send_cmd( command::stop{} );
      continue;
    }
    if( in == "u" ) {
      vol += .1;
      vol = std::clamp( vol, 0.0, 1.0 );
      g_midi_comm.send_cmd( command::volume{ vol } );
      lg.info( "volume: {}", vol );
      continue;
    }
    if( in == "d" ) {
      vol -= .1;
      vol = std::clamp( vol, 0.0, 1.0 );
      g_midi_comm.send_cmd( command::volume{ vol } );
      lg.info( "volume: {}", vol );
      continue;
    }
    if( in == "P" ) {
      auto progress = g_midi_comm.progress();
      if( progress.has_value() )
        lg.info( "progress: {}", progress.value() * 100.0 );
      continue;
    }
    if( in == "q" ) break;
  }
}

} // namespace rn::midiseq
