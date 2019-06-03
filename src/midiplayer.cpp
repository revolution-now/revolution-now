/****************************************************************
**midiplayer.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-05-31.
*
* Description: MusicPlayer classes with various MIDI backends.
*
*****************************************************************/
#include "midiplayer.hpp"

// Revolution Now
#include "config-files.hpp"
#include "errors.hpp"
#include "init.hpp"
#include "logging.hpp"
#include "midiseq.hpp"
#include "time.hpp"

using namespace std;

namespace rn {

namespace {

Opt<MidiSeqMusicPlayer> g_midiseq_player;

void cleanup_midiplayer() {}

fs::path mid_file_from_id( TuneId id ) {
  return config_music.midi_folder /
         fs::path( tune_stem_from_id( id ) + ".mid" );
}

} // namespace

// Outside of anonymous namespace otherwise it apparently cannot
// be a "friend" of the MidiSeqMusicPlayer class, which it needs
// to be to instantiate it.
void init_midiplayer() {
  if( midiseq::midiseq_enabled() ) {
    logger->info(
        "MIDI Sequencer Enabled: enabling Music Player." );
    g_midiseq_player = MidiSeqMusicPlayer();
  } else {
    logger->info(
        "MIDI Sequencer Disabled: not enabling Music Player." );
  }
}

REGISTER_INIT_ROUTINE( midiplayer );

pair<MusicPlayerDesc, MaybeMusicPlayer>
MidiSeqMusicPlayer::player() {
  auto name         = "MIDI Sequencer Music Player";
  auto description  = "MIDI Music Player with Sequencer";
  auto how_it_works = "There must be a synth available.";

  auto desc = MusicPlayerDesc{
      /*name=*/name,
      /*description=*/description,
      /*how_it_works=*/how_it_works,
  };

  if( g_midiseq_player.has_value() ) {
    return {
        desc,
        &( *g_midiseq_player ),
    };
  } else {
    return {
        desc,
        UNEXPECTED( "MIDI Sequencer failed to initialize" ),
    };
  }
}

bool MidiSeqMusicPlayer::good() const {
  return midiseq::midiseq_enabled();
}

Opt<TunePlayerInfo> MidiSeqMusicPlayer::can_play_tune(
    TuneId id ) {
  if( !good() ) return {};
  auto maybe_duration =
      midiseq::can_play_tune( mid_file_from_id( id ) );
  if( !maybe_duration.has_value() ) return nullopt;
  if( !( *maybe_duration > chrono::seconds( 0 ) ) )
    return nullopt;
  return TunePlayerInfo{/*id=*/id,
                        /*length=*/*maybe_duration,
                        /*progress=*/nullopt};
}

bool MidiSeqMusicPlayer::play( TuneId id ) {
  if( !good() ) return false;
  auto maybe_info = can_play_tune( id );
  if( !maybe_info.has_value() ) return false;
  logger->debug( "MidiSeqMusicPlayer: playing tune `{}`",
                 tune_display_name_from_id( id ) );
  midiseq::send_command(
      midiseq::command::play{mid_file_from_id( id )} );
  last_played_tune_info_ = maybe_info;
  return true;
}

void MidiSeqMusicPlayer::stop() {
  if( !good() ) return;
  logger->debug( "MidiSeqMusicPlayer: stop" );
  midiseq::send_command( midiseq::command::stop{} );
}

MusicPlayerDesc MidiSeqMusicPlayer::info() const {
  return MidiSeqMusicPlayer::player().first;
}

MusicPlayerState MidiSeqMusicPlayer::state() const {
  Opt<TunePlayerInfo> maybe_tune_info;
  bool                is_paused =
      ( midiseq::state() == midiseq::e_midiseq_state::paused );
  if( midiseq::state() == midiseq::e_midiseq_state::playing ||
      midiseq::state() == midiseq::e_midiseq_state::paused ) {
    maybe_tune_info = last_played_tune_info_;
    // Need to update the progress since it would be stale.
    maybe_tune_info->progress = midiseq::progress();
  }
  return {/*tune_info=*/maybe_tune_info,
          /*is_paused=*/is_paused};
}

MusicPlayerCapabilities MidiSeqMusicPlayer::capabilities()
    const {
  return {
      /*can_pause=*/true,
      /*has_volume=*/true,
      /*has_progress=*/true,
      /*has_tune_duration=*/true,
      /*can_seek=*/false,
  };
}

bool MidiSeqMusicPlayer::fence( Opt<Duration_t> timeout ) {
  if( !good() ) return true;
  auto start_time   = Clock_t::now();
  auto keep_waiting = [&] {
    if( timeout.has_value() )
      return Clock_t::now() - start_time < timeout;
    else
      return true;
  };
  while( keep_waiting() && midiseq::is_processing_commands() )
    sleep( 200us );
  bool timed_out = !keep_waiting();
  return !timed_out;
}

bool MidiSeqMusicPlayer::is_processing() const {
  if( !good() ) return false;
  return midiseq::is_processing_commands();
}

void MidiSeqMusicPlayer::pause() {
  if( !good() ) return;
  logger->debug( "MidiSeqMusicPlayer: pause" );
  midiseq::send_command( midiseq::command::pause{} );
}

// TODO
void MidiSeqMusicPlayer::resume() {
  if( !good() ) return;
  logger->debug( "MidiSeqMusicPlayer: resume" );
  midiseq::send_command( midiseq::command::resume{} );
}

void MidiSeqMusicPlayer::set_volume( double volume ) {
  if( !good() ) return;
  midiseq::send_command( midiseq::command::volume{volume} );
}

} // namespace rn
