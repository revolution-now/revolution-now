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

Vec<TuneId> g_playlist;

FlatMap<e_music_player, MusicPlayerDesc>  g_mplayer_descs;
FlatMap<e_music_player, MaybeMusicPlayer> g_mplayers;
FlatMap<e_music_player, MusicPlayerInfo>  g_mplayer_infos;

FlatMap<e_special_music_event, TuneId> g_special_tunes;

Opt<e_music_player> g_active_mplayer;

#define ADD_MUSIC_PLAYER( enum, prefix )           \
  std::tie( g_mplayer_descs[e_music_player::enum], \
            g_mplayers[e_music_player::enum] ) =   \
      prefix##MusicPlayer::player()

void init_conductor() {
  // Generate a random playlist.
  playlist_generate();

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

  // Now try to select which music player to use by taking some
  // hints from the config file. If those don't work out that
  // just take the first one (in the ordering of e_music_player
  // enum values) that is enabled.
  if( g_mplayer_infos[config_music.first_choice_music_player]
          .enabled )
    g_active_mplayer = config_music.first_choice_music_player;
  else if( g_mplayer_infos[config_music
                               .second_choice_music_player]
               .enabled ) {
    g_active_mplayer = config_music.first_choice_music_player;
    logger->info(
        "First choice music player not available; using second "
        "choice." );
  } else {
    for( auto mplayer : values<e_music_player> ) {
      if( g_mplayer_infos[mplayer].enabled ) {
        g_active_mplayer = mplayer;
        logger->info(
            "Preferred music players not available; using {}",
            mplayer );
        break;
      }
    }
  }

  if( !g_active_mplayer.has_value() )
    logger->warn(
        "No usable music player found; music will not be "
        "played." );

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
            "tune `{}` will be played for event `{}`.", tune_id,
            event );
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
}

void cleanup_conductor() {
  g_mplayer_infos.clear();
  g_mplayer_descs.clear();
  g_mplayers.clear();
}

} // namespace

REGISTER_INIT_ROUTINE( conductor );

void ConductorInfo::log() const {}

void MusicPlayerInfo::log() const {}

MusicPlayerInfo const& music_player_info(
    e_music_player mplayer ) {
  NOT_IMPLEMENTED; //
  (void)mplayer;
}

bool set_music_player( e_music_player mplayer ) {
  NOT_IMPLEMENTED; //
  (void)mplayer;
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

void play() {
  NOT_IMPLEMENTED; //
}

void prev() {
  NOT_IMPLEMENTED; //
}

void next() {
  NOT_IMPLEMENTED; //
}

void stop() {
  NOT_IMPLEMENTED; //
}

void pause() {
  NOT_IMPLEMENTED; //
}

void resume() {
  NOT_IMPLEMENTED; //
}

void seek( double pos ) {
  NOT_IMPLEMENTED; //
  (void)pos;
}

namespace request {

void won_battle_europeans() {
  NOT_IMPLEMENTED; //
}
void won_battle_natives() {
  NOT_IMPLEMENTED; //
}
void lost_battle_europeans() {
  NOT_IMPLEMENTED; //
}
void lost_battle_natives() {
  NOT_IMPLEMENTED; //
}

void slow_sad() {
  NOT_IMPLEMENTED; //
}
void medium_tempo() {
  NOT_IMPLEMENTED; //
}
void happy_fast() {
  NOT_IMPLEMENTED; //
}

void orchestrated() {
  NOT_IMPLEMENTED; //
}
void fiddle_tune() {
  NOT_IMPLEMENTED; //
}
void fife_drum_sad() {
  NOT_IMPLEMENTED; //
}
void fife_drum_slow() {
  NOT_IMPLEMENTED; //
}
void fife_drum_fast() {
  NOT_IMPLEMENTED; //
}
void fife_drum_happy() {
  NOT_IMPLEMENTED; //
}

void native_sad() {
  NOT_IMPLEMENTED; //
}
void native_happy() {
  NOT_IMPLEMENTED; //
}

void king_happy() {
  NOT_IMPLEMENTED; //
}
void king_sad() {
  NOT_IMPLEMENTED; //
}
void king_war() {
  NOT_IMPLEMENTED; //
}

} // namespace request

// Playlists.

void playlist_generate() {
  NOT_IMPLEMENTED; //
}

// Testing
void test() { logger->info( "Testing Music Conductor" ); }

} // namespace rn::conductor
