/****************************************************************
**conductor.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-06-02.
*
* Description: Main interface for music playing.
*
*****************************************************************/
#include "conductor.hpp"

// Revolution Now
#include "config-files.hpp"
#include "init.hpp"
#include "logging.hpp"
#include "midiplayer.hpp"
#include "mplayer.hpp"
#include "ranges.hpp"

using namespace std;

namespace rn::conductor {

namespace {

// The pointers to music players are setup once duration initial-
// ization and will never change, even if a music player fails.
// If a music player fails then will mark it as disable in the
// associated MusicPlayerInfo struct.
FlatMap<e_music_player, MaybeMusicPlayer> g_mplayers;

FlatMap<e_music_player, MusicPlayerDesc> g_mplayer_descs;
FlatMap<e_music_player, MusicPlayerInfo> g_mplayer_infos;

FlatMap<e_special_music_event, TuneId> g_special_tunes;

Opt<e_music_player> g_active_mplayer;

size_t      g_playlist_pos{};
Vec<TuneId> g_playlist;

#define ADD_MUSIC_PLAYER( enum, prefix )           \
  std::tie( g_mplayer_descs[e_music_player::enum], \
            g_mplayers[e_music_player::enum] ) =   \
      prefix##MusicPlayer::player()

auto enabled_mplayers_ptrs() {
  return g_mplayers                                            //
         | rv::filter( L( g_mplayer_infos[_.first].enabled ) ) //
         | rv::transform( L( *( _.second ) ) );
}

auto enabled_mplayers_enums() {
  return g_mplayers                                            //
         | rv::filter( L( g_mplayer_infos[_.first].enabled ) ) //
         | rv::transform( L( _.first ) );
}

TuneId next_tune_in_playlist() {
  g_playlist_pos++;
  g_playlist_pos %= g_playlist.size();
  return g_playlist[g_playlist_pos];
}

TuneId prev_tune_in_playlist() {
  if( g_playlist_pos == 0 )
    g_playlist_pos = g_playlist.size() - 1;
  else
    g_playlist_pos--;
  CHECK( g_playlist_pos < g_playlist.size() );
  return g_playlist[g_playlist_pos];
}

#define ACTIVE_MUSIC_PLAYER_OR_RETURN( var )           \
  if( !g_active_mplayer.has_value() ) return;          \
  auto expect_mplayer = g_mplayers[*g_active_mplayer]; \
  DCHECK( expect_mplayer.has_value() );                \
  auto var = *expect_mplayer;                          \
  if( !var->good() ) return

#define CONDUCTOR_INFO_OR_RETURN( var )  \
  auto expect_info = state();            \
  if( !expect_info.has_value() ) return; \
  auto const& info = *expect_info

void play_impl( TuneId id ) {
  ACTIVE_MUSIC_PLAYER_OR_RETURN( mplayer );
  logger->info( "Playing \"{}\"",
                tune_display_name_from_id( id ) );
  mplayer->play( id );
}

void init_conductor() {
  // Generate a random playlist.
  playlist_generate();
  CHECK( g_playlist.size() > 0 );

  // Gather a list of all available music players.
  ADD_MUSIC_PLAYER( silent, Silent );
  ADD_MUSIC_PLAYER( midiseq, MidiSeq );

  // Check each music player for viability and populate info.
  for( auto mplayer : values<e_music_player> ) {
    CHECK( g_mplayers.contains( mplayer ),
           "Music player `{}` not added to list.", mplayer );
    CHECK( g_mplayer_descs.contains( mplayer ),
           "Music player `{}` not added to list.", mplayer );

    bool   enable_mplayer = true;
    string reason;

    // The music player will be enabled if we have been given a
    // reference to it and if it is capable of playing all tunes.
    if( !g_mplayers[mplayer].has_value() ) {
      enable_mplayer = false;
      reason         = g_mplayers[mplayer].error().what;
    } else {
      // We have been given a reference to the music player. So
      // now check that it is in good health.
      MusicPlayer* ptr = *g_mplayers[mplayer];
      CHECK( ptr != nullptr );
      MusicPlayer& pl = *ptr;
      if( !pl.good() ) {
        enable_mplayer = false;
        reason         = "encountered internal error.";
      } else {
        // The music player reports good health. So now check
        // that it can play all the tunes.
        for( auto id : all_tunes() ) {
          if( !pl.can_play_tune( id ) ) {
            enable_mplayer = false;
            reason =
                fmt::format( "cannot play tune `{}`",
                             tune_display_name_from_id( id ) );
            break;
          }
        }
      }
    }

    if( !enable_mplayer ) {
      logger->warn( "Music player `{}` not enabled: {}",
                    g_mplayer_descs[mplayer].name, reason );
    }
    MusicPlayerInfo info{
        /*enabled=*/enable_mplayer,
        g_mplayer_descs[mplayer].name,
        g_mplayer_descs[mplayer].description,
        g_mplayer_descs[mplayer].how_it_works,
    };
    g_mplayer_infos[mplayer] = info;
  }

  // Copy this so that we can use the operator[].
  auto m = config_music.special_event_tunes;

  // Now check that there is a tune assigned to each special
  // music event. That is defined as a game event where there
  // should always be the tune played.
  for( auto event : values<e_special_music_event> ) {
    CHECK( m.contains( event ),
           "There is no tune set to be played for the special "
           "event `{}`.",
           event );
    auto stem = m[event];
    for( auto tune_id : all_tunes() ) {
      if( tune_stem_from_id( tune_id ) == stem ) {
        logger->debug(
            "tune `{}` will be played for event `{}`.",
            tune_stem_from_id( tune_id ), event );
        g_special_tunes[event] = tune_id;
        break;
      }
    }
    CHECK( g_special_tunes.contains( event ),
           "The tune stem `{}`, set to be played for the "
           "special event `{}`, does not exist.",
           stem, event );
  }

  CHECK( g_special_tunes.size() ==
         rg::distance( values<e_special_music_event> ) );

  // This will set the music player if possible, make sure all
  // music is stopped, etc.
  reset();
}

void cleanup_conductor() {
  g_mplayer_infos.clear();
  g_mplayer_descs.clear();
  g_mplayers.clear();
}

} // namespace

REGISTER_INIT_ROUTINE( conductor );

void ConductorInfo::log() const {
  logger->info( "ConductorInfo:" );
  logger->info( "  mplayer:     {}", mplayer );
  logger->info( "  music_state: {}", music_state );
  logger->info( "  volume:      {}", volume );
  logger->info( "  autoplay:    {}", autoplay );
  if( playing_now.has_value() ) ( *playing_now ).log();
}

void MusicPlayerInfo::log() const {
  logger->info( "MusicPlayerInfo:" );
  logger->info( "  enabled:      {}", enabled );
  logger->info( "  name:         {}", name );
  logger->info( "  description:  {}", description );
  logger->info( "  how_it_works: {}", how_it_works );
}

MusicPlayerInfo const& music_player_info(
    e_music_player mplayer ) {
  DCHECK( g_mplayer_infos.contains( mplayer ) );
  return g_mplayer_infos[mplayer];
}

bool set_music_player( e_music_player mplayer ) {
  reset();
  if( !g_mplayer_infos[mplayer].enabled ) {
    logger->error(
        "attempt to set music player `{}` but it is disabled.",
        g_mplayer_infos[mplayer].name );
    g_active_mplayer = nullopt;
    return false;
  }
  g_active_mplayer = mplayer;
  return true;
}

expect<ConductorInfo> state() {
  NOT_IMPLEMENTED; //
}

void subscribe_to_event( e_conductor_event,
                         function<void( void )> ) {
  NOT_IMPLEMENTED; //
}

void set_autoplay( bool enabled ) {
  NOT_IMPLEMENTED; //
  (void)enabled;
}

void reset() {
  logger->info( "Resetting Conductor state." );
  // Try to select which music player to use by taking some hints
  // from the config file. If those don't work out that just take
  // the first one (in the ordering of e_music_player enum val-
  // ues) that is enabled.
  if( g_mplayer_infos[config_music.first_choice_music_player]
          .enabled ) {
    g_active_mplayer = config_music.first_choice_music_player;
    logger->info( "Using first choice music player `{}`.",
                  g_active_mplayer );
  } else if( g_mplayer_infos[config_music
                                 .second_choice_music_player]
                 .enabled ) {
    g_active_mplayer = config_music.first_choice_music_player;
    logger->info(
        "First choice music player not available; using second "
        "choice." );
  } else {
    if( auto maybe_first_available =
            head( enabled_mplayers_enums() );
        maybe_first_available.has_value() ) {
      g_active_mplayer = *maybe_first_available;
      logger->info(
          "Preferred music players not available; using {}",
          g_active_mplayer );
    }
  }

  if( !g_active_mplayer.has_value() )
    logger->warn(
        "No usable music player found; music will not be "
        "played." );

  // Need fence() here?  Don't think so...
  for( auto* mplayer : enabled_mplayers_ptrs() ) mplayer->stop();
}

void play() {
  ACTIVE_MUSIC_PLAYER_OR_RETURN( mplayer );
  CONDUCTOR_INFO_OR_RETURN( info );
  switch( info.music_state ) {
    case +e_music_state::playing: //
      // We're already playing, so do nothing.
      break;
    case +e_music_state::paused: //
      mplayer->resume();
      break;
    case +e_music_state::stopped: //
      play_impl( next_tune_in_playlist() );
      break;
  }
}

void prev() {
  CONDUCTOR_INFO_OR_RETURN( info );
  // If progress is not available then behave as if it is zero.
  double      progress = 0.0;
  Opt<TuneId> id;
  if( info.playing_now.has_value() ) {
    if( ( *info.playing_now ).progress.has_value() )
      progress = *( *info.playing_now ).progress;
    id = ( *info.playing_now ).id;
  }
  switch( info.music_state ) {
    case +e_music_state::playing: {
      DCHECK( id );
      if( progress > config_music.threshold_previous_tune )
        play_impl( *id );
      else
        play_impl( prev_tune_in_playlist() );
      break;
    }
    case +e_music_state::paused: {
      DCHECK( id );
      stop();
      prev_tune_in_playlist();
      break;
    }
    case +e_music_state::stopped: //
      prev_tune_in_playlist();
      break;
  }
}

void next() {
  CONDUCTOR_INFO_OR_RETURN( info );
  switch( info.music_state ) {
    case +e_music_state::playing: {
      play_impl( next_tune_in_playlist() );
      break;
    }
    case +e_music_state::paused: {
      stop();
      next_tune_in_playlist();
      break;
    }
    case +e_music_state::stopped: //
      next_tune_in_playlist();
      break;
  }
}

void stop() {
  ACTIVE_MUSIC_PLAYER_OR_RETURN( mplayer );
  mplayer->stop();
}

void pause() {
  ACTIVE_MUSIC_PLAYER_OR_RETURN( mplayer );
  auto capabilities = mplayer->capabilities();
  if( !capabilities.can_pause ) {
    logger->warn( "Music player `{}` does not support pausing.",
                  mplayer->info().name );
    return;
  }
  CONDUCTOR_INFO_OR_RETURN( info );
  switch( info.music_state ) {
    case +e_music_state::playing: {
      mplayer->pause();
      break;
    }
    case +e_music_state::paused: {
      break;
    }
    case +e_music_state::stopped: //
      break;
  }
}

void resume() {
  ACTIVE_MUSIC_PLAYER_OR_RETURN( mplayer );
  auto capabilities = mplayer->capabilities();
  if( !capabilities.can_pause ) {
    logger->warn( "Music player `{}` does not support pausing.",
                  mplayer->info().name );
    return;
  }
  CONDUCTOR_INFO_OR_RETURN( info );
  switch( info.music_state ) {
    case +e_music_state::playing: {
      break;
    }
    case +e_music_state::paused: {
      mplayer->resume();
      break;
    }
    case +e_music_state::stopped: //
      break;
  }
}

void seek( double pos ) {
  ACTIVE_MUSIC_PLAYER_OR_RETURN( mplayer );
  auto capabilities = mplayer->capabilities();
  if( !capabilities.can_seek ) {
    logger->warn( "Music player `{}` does not support seeking.",
                  mplayer->info().name );
    return;
  }
  CONDUCTOR_INFO_OR_RETURN( info );
  switch( info.music_state ) {
    case +e_music_state::playing: {
      mplayer->seek( pos );
      break;
    }
    case +e_music_state::paused: {
      mplayer->seek( pos );
      break;
    }
    case +e_music_state::stopped: //
      break;
  }
}

namespace request {

void won_battle_europeans() {}
void won_battle_natives() {}
void lost_battle_europeans() {}
void lost_battle_natives() {}

void slow_sad() {}
void medium_tempo() {}
void happy_fast() {}

void orchestrated() {}
void fiddle_tune() {}
void fife_drum_sad() {}
void fife_drum_slow() {}
void fife_drum_fast() {}
void fife_drum_happy() {}

void native_sad() {}
void native_happy() {}

void king_happy() {}
void king_sad() {}
void king_war() {}

} // namespace request

// Playlists.

void playlist_generate() {
  NOT_IMPLEMENTED; //
}

// Testing
void test() { logger->info( "Testing Music Conductor" ); }

} // namespace rn::conductor
