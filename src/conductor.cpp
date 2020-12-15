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
#include "frame.hpp"
#include "init.hpp"
#include "logging.hpp"
#include "menu.hpp"
#include "midiplayer.hpp"
#include "mplayer.hpp"
#include "oggplayer.hpp"
#include "rand.hpp"
#include "ranges.hpp"
#include "time.hpp"
#include "window.hpp"

// Revolution Now (config)
#include "../config/ucl/music.inl"

// base-util
#include "base-util/algo.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

// magic enum
#include "magic_enum.hpp"

// Range-v3
#include "range/v3/iterator/operations.hpp"
#include "range/v3/view/enumerate.hpp"
#include "range/v3/view/filter.hpp"
#include "range/v3/view/take.hpp"
#include "range/v3/view/transform.hpp"

using namespace std;

namespace rn::conductor {

namespace {

// The pointers to music players are setup once duration initial-
// ization and will never change, even if a music player fails.
// If a music player fails then will mark it as disable in the
// associated MusicPlayerInfo struct.
absl::flat_hash_map<e_music_player, MaybeMusicPlayer> g_mplayers;

absl::flat_hash_map<e_music_player, MusicPlayerDesc>
    g_mplayer_descs;
absl::flat_hash_map<e_music_player, MusicPlayerInfo>
    g_mplayer_infos;

absl::flat_hash_map<e_special_music_event, TuneId>
    g_special_tunes;

maybe<e_music_player> g_active_mplayer;

size_t      g_playlist_pos{};
Vec<TuneId> g_playlist;

bool g_autoplay{ true };

double g_master_volume{ 1.0 };

#define ADD_MUSIC_PLAYER( enum, prefix )    \
  g_mplayer_descs[e_music_player::enum] =   \
      prefix##MusicPlayer::player().first;  \
  g_mplayers.emplace( e_music_player::enum, \
                      prefix##MusicPlayer::player().second );

auto enabled_mplayers_ptrs() {
  return g_mplayers                                            //
         | rv::filter( L( g_mplayer_infos[_.first].enabled ) ) //
         | rv::transform( []( auto& pair ) -> decltype( auto ) {
             return *( pair.second );
           } );
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

TuneId current_tune_in_playlist() {
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

#define ACTIVE_MUSIC_PLAYER_OR_RETURN( var )                  \
  if( !g_active_mplayer.has_value() ) return;                 \
  auto const& expect_mplayer = g_mplayers[*g_active_mplayer]; \
  DCHECK( expect_mplayer.has_value() );                       \
  auto& var = *expect_mplayer;                                \
  if( !var.good() ) return

#define CONDUCTOR_INFO_OR_RETURN( var )  \
  auto expect_info = state();            \
  if( !expect_info.has_value() ) return; \
  auto const& var = *expect_info

#define CONDUCTOR_INFO_OR_RETURN_FALSE( var )  \
  auto expect_info = state();                  \
  if( !expect_info.has_value() ) return false; \
  auto const& var = *expect_info

void play_impl( TuneId id ) {
  ACTIVE_MUSIC_PLAYER_OR_RETURN( mplayer );
  lg.info( "playing \"{}\".", tune_display_name_from_id( id ) );
  mplayer.play( id );
}

// This will get called roughly once per second and, if autoplay
// is enabled, it will start playing the next song if the music
// player has stopped.
//
// TODO: Consider checking in here the state of the music player
// and dealing with it if it is in a failed state.
void conductor_tick() {
  CONDUCTOR_INFO_OR_RETURN( info );
  if( info.autoplay ) {
    if( info.music_state == e_music_state::stopped ) {
      next();
      play();
    }
  }
}

// This API for playing a specific tune is only supposed to be
// used internally. Outside callers should go through the Conduc-
// tor's request:: api to request playing a particular kind of
// tune.
//
// This method will always cause the given tune to start playing
// from the beginning.
void play( TuneId id ) { play_impl( id ); }

/****************************************************************
** Requests
*****************************************************************/

absl::flat_hash_map<e_request, TuneVecDimensions>&
dimensions_for_request() {
  static absl::flat_hash_map<e_request, TuneVecDimensions> m;
  return m;
}

#define DIM( name, _, ... ) \
  dims.name = { EVAL(       \
      PP_MAP_PREPEND_NS( e_tune_##name, __VA_ARGS__ ) ) };

#define REQUEST( name, ... )                          \
  {                                                   \
    CHECK( !dimensions_for_request().contains(        \
        e_request::name ) );                          \
    TuneVecDimensions dims;                           \
    __VA_ARGS__                                       \
    dimensions_for_request()[e_request::name] = dims; \
  }

void register_requests() {
#include "../config/c++/tune-requests.inl"
  for( auto req : magic_enum::enum_values<e_request>() ) {
    CHECK( dimensions_for_request().contains( req ),
           "The Conductor request category `{}` has not been "
           "given a definition.",
           req );
  }
}

/****************************************************************
** Init / Cleanup
*****************************************************************/

void init_conductor() {
  // Generate a random playlist.
  playlist_generate();
  CHECK( g_playlist.size() > 0 );

  register_requests();

  subscribe_to_frame_tick( conductor_tick, 1s );

  // Gather a list of all available music players.
  ADD_MUSIC_PLAYER( silent, Silent );
  ADD_MUSIC_PLAYER( midiseq, MidiSeq );
  ADD_MUSIC_PLAYER( ogg, Ogg );

  // Check each music player for viability and populate info.
  for( auto mplayer :
       magic_enum::enum_values<e_music_player>() ) {
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
      // FIXME: put the error message here when switching to
      // expect<>.
      reason = "not enabled.";
    } else {
      // We have been given a reference to the music player. So
      // now check that it is in good health.
      MusicPlayer& pl = *g_mplayers[mplayer];
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
      lg.warn( "music player `{}` not enabled: {}.",
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

  absl::flat_hash_set<string> used_stems;

  // Now check that there is a tune assigned to each special
  // music event. That is defined as a game event where there
  // should always be the tune played.
  for( auto event :
       magic_enum::enum_values<e_special_music_event>() ) {
    CHECK( m.contains( event ),
           "There is no tune set to be played for the special "
           "event `{}`.",
           event );
    auto stem = m[event];
    for( auto tune_id : all_tunes() ) {
      if( tune_stem_from_id( tune_id ) == stem ) {
        lg.debug( "tune `{}` will be played for event `{}`.",
                  tune_stem_from_id( tune_id ), event );
        CHECK( !used_stems.contains( stem ),
               "The tune `{}` is set to be used in multiple "
               "special events.",
               tune_stem_from_id( tune_id ) );
        used_stems.insert( stem );
        g_special_tunes[event] = tune_id;
        break;
      }
    }
    CHECK( g_special_tunes.contains( event ),
           "The tune stem `{}`, set to be played for the "
           "special event `{}`, does not exist.",
           stem, event );
  }

  CHECK(
      int( g_special_tunes.size() ) ==
      rg::distance(
          magic_enum::enum_values<e_special_music_event>() ) );

  // This will set the music player if possible, make sure all
  // music is stopped, etc.
  reset();

  auto info = state();
  if( info.has_value() ) ( *info ).log();
}

void cleanup_conductor() {
  g_mplayer_infos.clear();
  g_mplayer_descs.clear();
  g_mplayers.clear();
}

absl::flat_hash_map<e_conductor_event,
                    vector<ConductorEventFunc>>&
subscriptions() {
  static absl::flat_hash_map<e_conductor_event,
                             vector<ConductorEventFunc>>
      subs;
  return subs;
}

void send_notifications( e_conductor_event event ) {
  for( auto const& func : subscriptions()[event] ) func();
}

void silence_all_music_players() {
  // Need fence() here?  Don't think so...
  for( auto& mplayer : enabled_mplayers_ptrs() ) {
    mplayer.stop();
    auto capabilities = mplayer.capabilities();
    if( capabilities.has_volume )
      mplayer.set_volume( g_master_volume );
  }
}

} // namespace

REGISTER_INIT_ROUTINE( conductor );

void play_request( e_request             request,
                   e_request_probability probability ) {
  CONDUCTOR_INFO_OR_RETURN( info );
  DCHECK( dimensions_for_request().contains( request ) );
  auto const& dims = dimensions_for_request()[request];
  double      prob = 1.0;
  switch( probability ) {
    case e_request_probability::always: prob = 1.0; break;
    case e_request_probability::sometimes: prob = .3; break;
    case e_request_probability::rarely: prob = .1; break;
  }
  // In the below we use fuzzy_match=true because we always want
  // to guarantee some tunes returned regardless of our search
  // criteria.
  if( rng::flip_coin( prob ) ) {
    auto tune_id = find_tunes( dims, /*fuzzy_match=*/true,
                               /*not_like=*/false )[0];
    // Only play it if we're not already playing it.
    if( info.playing_now &&
        ( *info.playing_now ).id != tune_id ) {
      lg.info( "requesting music for `{}`", request );
      play( tune_id );
    }
  }
}

void ConductorInfo::log() const {
  lg.debug( "ConductorInfo:" );
  lg.debug( "  mplayer:     {}", mplayer );
  lg.debug( "  music_state: {}", music_state );
  lg.debug( "  volume:      {}", volume );
  lg.debug( "  autoplay:    {}", autoplay );
  if( playing_now.has_value() ) ( *playing_now ).log();
}

void MusicPlayerInfo::log() const {
  lg.debug( "MusicPlayerInfo:" );
  lg.debug( "  enabled:      {}", enabled );
  lg.debug( "  name:         {}", name );
  lg.debug( "  description:  {}", description );
  lg.debug( "  how_it_works: {}", how_it_works );
}

MusicPlayerInfo const& music_player_info(
    e_music_player mplayer ) {
  DCHECK( g_mplayer_infos.contains( mplayer ) );
  return g_mplayer_infos[mplayer];
}

bool set_music_player( e_music_player mplayer ) {
  if( !g_mplayer_infos[mplayer].enabled ) {
    lg.warn(
        "attempt to set music player `{}` but it is disabled.",
        g_mplayer_infos[mplayer].name );
    return false;
  }
  if( g_active_mplayer == mplayer ) return true;
  // Probably only need to silence the active one at this point,
  // but just in case...
  silence_all_music_players();
  g_active_mplayer = mplayer;
  send_notifications( e_conductor_event::mplayer_changed );
  CONDUCTOR_INFO_OR_RETURN_FALSE( info );
  if( info.autoplay ) {
    next();
    play();
  }
  return true;
}

maybe<ConductorInfo> state() {
  if( !g_active_mplayer.has_value() ) return nothing;
  auto const& expect_mplayer = g_mplayers[*g_active_mplayer];
  DCHECK( expect_mplayer.has_value() );
  auto&         mplayer  = *expect_mplayer;
  auto          mp_state = mplayer.state();
  e_music_state st       = e_music_state::stopped;
  if( mp_state.tune_info.has_value() ) {
    if( mp_state.is_paused )
      st = e_music_state::paused;
    else
      st = e_music_state::playing;
  }
  return ConductorInfo{
      /*mplayer=*/*g_active_mplayer,
      /*music_state=*/st,
      /*playing_now=*/mp_state.tune_info,
      /*volume=*/g_master_volume,
      /*autoplay=*/g_autoplay,
  };
}

void set_autoplay( bool enabled ) {
  g_autoplay  = enabled;
  auto on_off = enabled ? "on" : "off";
  lg.info( "music autoplay is {}", on_off );
}

void reset() {
  lg.info( "resetting conductor state." );
  // Try to select which music player to use by taking some hints
  // from the config file. If those don't work out that just take
  // the first one (in the ordering of e_music_player enum val-
  // ues) that is enabled.
  if( g_mplayer_infos[config_music.first_choice_music_player]
          .enabled ) {
    g_active_mplayer = config_music.first_choice_music_player;
    lg.info( "using first choice music player `{}`.",
             g_active_mplayer );
  } else if( g_mplayer_infos[config_music
                                 .second_choice_music_player]
                 .enabled ) {
    g_active_mplayer = config_music.second_choice_music_player;
    lg.info(
        "First choice music player not available; using second "
        "choice: {}",
        g_active_mplayer );
  } else {
    if( auto maybe_first_available =
            head( enabled_mplayers_enums() );
        maybe_first_available.has_value() ) {
      g_active_mplayer = *maybe_first_available;
      lg.info(
          "Preferred music players not available; using {}.",
          g_active_mplayer );
    }
  }

  if( g_active_mplayer.has_value() ) {
    send_notifications( e_conductor_event::mplayer_changed );
  } else {
    lg.warn(
        "No usable music player found; music will not be "
        "played." );
  }

  g_master_volume = config_music.initial_volume;
  send_notifications( e_conductor_event::volume_changed );

  silence_all_music_players();

  g_autoplay = config_music.autoplay;
}

void play() {
  ACTIVE_MUSIC_PLAYER_OR_RETURN( mplayer );
  CONDUCTOR_INFO_OR_RETURN( info );
  switch( info.music_state ) {
    case e_music_state::playing: //
      // We're already playing, so do nothing.
      break;
    case e_music_state::paused: //
      mplayer.resume();
      break;
    case e_music_state::stopped: //
      play_impl( current_tune_in_playlist() );
      break;
  }
}

void prev() {
  // In this function we alter our behavior depending on whether
  // a tune is playing or not and, if it is, whether or not the
  // progress has reached beyond a threshold. If it has then we
  // simply rewind back to the beginning of the tune. If not then
  // we move to the previous tune. This is to emulate the be-
  // havior of most compact disc players.
  CONDUCTOR_INFO_OR_RETURN( info );
  // If progress and/or duration are not available then behave as
  // if it is zero.
  Duration_t    progress_time = 0s;
  maybe<TuneId> id;
  if( info.playing_now.has_value() ) {
    if( ( *info.playing_now ).progress.has_value() ) {
      double progress = *( *info.playing_now ).progress;
      // Just use this as a dummy in case we dont' have duration.
      // This means that if a music player does not provide dura-
      // tion then we effectively consider the threshold to be a
      // percentage (i.e., a threshold in the progess as opposed
      // to threshold in elapsed time).
      Duration_t tune_duration = 120s;
      if( ( *info.playing_now ).length.has_value() )
        tune_duration = *( *info.playing_now ).length;
      progress_time = chrono::seconds( int(
          chrono::duration_cast<chrono::seconds>( tune_duration )
              .count() *
          progress ) );
    }
    id = ( *info.playing_now ).id;
  }
  switch( info.music_state ) {
    case e_music_state::playing: {
      DCHECK( id );
      if( progress_time >
          config_music.threshold_previous_tune_secs )
        play_impl( *id );
      else
        play_impl( prev_tune_in_playlist() );
      break;
    }
    case e_music_state::paused: {
      DCHECK( id );
      stop();
      prev_tune_in_playlist();
      break;
    }
    case e_music_state::stopped: //
      prev_tune_in_playlist();
      break;
  }
}

void next() {
  CONDUCTOR_INFO_OR_RETURN( info );
  switch( info.music_state ) {
    case e_music_state::playing: {
      play_impl( next_tune_in_playlist() );
      break;
    }
    case e_music_state::paused: {
      play_impl( next_tune_in_playlist() );
      break;
    }
    case e_music_state::stopped: //
      next_tune_in_playlist();
      break;
  }
}

void stop() {
  ACTIVE_MUSIC_PLAYER_OR_RETURN( mplayer );
  mplayer.stop();
}

void pause() {
  ACTIVE_MUSIC_PLAYER_OR_RETURN( mplayer );
  auto capabilities = mplayer.capabilities();
  if( !capabilities.can_pause ) {
    lg.warn( "music player `{}` does not support pausing.",
             mplayer.info().name );
    return;
  }
  CONDUCTOR_INFO_OR_RETURN( info );
  switch( info.music_state ) {
    case e_music_state::playing: {
      mplayer.pause();
      break;
    }
    case e_music_state::paused: {
      break;
    }
    case e_music_state::stopped: //
      break;
  }
}

void resume() {
  ACTIVE_MUSIC_PLAYER_OR_RETURN( mplayer );
  auto capabilities = mplayer.capabilities();
  if( !capabilities.can_pause ) {
    lg.warn( "music player `{}` does not support pausing.",
             mplayer.info().name );
    return;
  }
  CONDUCTOR_INFO_OR_RETURN( info );
  switch( info.music_state ) {
    case e_music_state::playing: {
      break;
    }
    case e_music_state::paused: {
      mplayer.resume();
      break;
    }
    case e_music_state::stopped: //
      break;
  }
}

void set_volume( double vol ) {
  ACTIVE_MUSIC_PLAYER_OR_RETURN( mplayer );
  vol = std::clamp( vol, 0.0, 1.0 );

  g_master_volume = vol;
  send_notifications( e_conductor_event::volume_changed );

  auto capabilities = mplayer.capabilities();
  if( !capabilities.has_volume ) {
    lg.warn(
        "Music player `{}` does not support setting volume.",
        mplayer.info().name );
    return;
  }
  mplayer.set_volume( vol );
}

void seek( double pos ) {
  ACTIVE_MUSIC_PLAYER_OR_RETURN( mplayer );
  auto capabilities = mplayer.capabilities();
  if( !capabilities.can_seek ) {
    lg.warn( "music player `{}` does not support seeking.",
             mplayer.info().name );
    return;
  }
  CONDUCTOR_INFO_OR_RETURN( info );
  switch( info.music_state ) {
    case e_music_state::playing: {
      mplayer.seek( pos );
      break;
    }
    case e_music_state::paused: {
      mplayer.seek( pos );
      break;
    }
    case e_music_state::stopped: //
      break;
  }
}

void playlist_generate() {
  // For this it will first generate a random list of tunes,
  // choosing among all the tunes who have purpose=standard. The
  // length of this list will be 10 times the total number of
  // tunes. This list must obey two constraints: 1. it cannot
  // play a tune that was already played in the last five items
  // in the playlist, unless there are fewer than five total
  // tunes. 2. the last set of tunes shall not overlap with the
  // first set of tunes, since the playlist needs to wrap around.
  lg.info( "generating playlist." );
  CHECK( config_music.tunes.size() > 0 );
  auto   num_tunes = config_music.tunes.size();
  size_t no_overlap_size =
      ( num_tunes > 5 ) ? 5 : ( num_tunes - 1 );
  CHECK( no_overlap_size < 100000 ); // sanity check
  vector<TuneId> last_n;
  for( auto id : all_tunes() | rv::take( no_overlap_size ) )
    last_n.push_back( id );
  CHECK( last_n.size() == no_overlap_size );
  auto overlaps = [&]( TuneId id ) {
    return util::find( last_n, id ) != last_n.end();
  };
  vector<TuneId>   res;
  constexpr size_t num_chunks        = 10;
  size_t const     playlist_size     = num_tunes * num_chunks;
  size_t           timeout_countdown = playlist_size * 100;
  for( size_t i = 0; i < playlist_size; ++i ) {
    while( true ) {
      timeout_countdown--;
      if( timeout_countdown == 0 ) break;
      auto id = random_tune();
      if( tune_dimensions( id ).purpose ==
          e_tune_purpose::special_event )
        continue;
      if( overlaps( id ) ) continue;
      res.push_back( id );
      last_n.erase( last_n.begin() );
      last_n.push_back( id );
      break;
    }
    if( timeout_countdown == 0 ) {
      lg.warn( "max cycles reached when generating playlist." );
      break;
    }
  }
  lg.trace( "playlist:" );
  for( auto [idx, id] : res | rv::enumerate )
    lg.trace( " {: >4}. {}", idx + 1,
              tune_display_name_from_id( id ) );
  CHECK( res.size() > 0 );

  g_playlist     = res;
  g_playlist_pos = 0;
}

void subscribe_to_conductor_event( e_conductor_event  event,
                                   ConductorEventFunc func ) {
  subscriptions()[event].push_back( std::move( func ) );
}

/****************************************************************
** Menu Handlers
*****************************************************************/
void menu_music_play() {
  play();
  set_autoplay( true );
}
bool menu_music_play_enabled() {
  CONDUCTOR_INFO_OR_RETURN_FALSE( info );
  return info.music_state == e_music_state::stopped;
}

void menu_music_stop() {
  stop();
  set_autoplay( false );
}
bool menu_music_stop_enabled() {
  CONDUCTOR_INFO_OR_RETURN_FALSE( info );
  return info.music_state == e_music_state::playing ||
         info.music_state == e_music_state::paused;
}

void menu_music_pause() { pause(); }
bool menu_music_pause_enabled() {
  CONDUCTOR_INFO_OR_RETURN_FALSE( info );
  return info.music_state == e_music_state::playing;
}

void menu_music_resume() { resume(); }
bool menu_music_resume_enabled() {
  CONDUCTOR_INFO_OR_RETURN_FALSE( info );
  return info.music_state == e_music_state::paused;
}

void menu_music_next() { next(); }
bool menu_music_next_enabled() {
  CONDUCTOR_INFO_OR_RETURN_FALSE( info );
  return info.music_state == e_music_state::playing ||
         info.music_state == e_music_state::paused;
}

void menu_music_prev() { prev(); }
bool menu_music_prev_enabled() {
  CONDUCTOR_INFO_OR_RETURN_FALSE( info );
  return info.music_state == e_music_state::playing ||
         info.music_state == e_music_state::paused;
}

void menu_music_vol_up() {
  CONDUCTOR_INFO_OR_RETURN( info );
  if( !info.volume.has_value() ) return;
  set_volume( std::clamp( *info.volume + 0.1, 0.0, 1.0 ) );
}
bool menu_music_vol_up_enabled() {
  CONDUCTOR_INFO_OR_RETURN_FALSE( info );
  if( !info.volume.has_value() ) return false;
  return *info.volume < 1.0;
}

void menu_music_vol_down() {
  CONDUCTOR_INFO_OR_RETURN( info );
  if( !info.volume.has_value() ) return;
  set_volume( std::clamp( *info.volume - 0.1, 0.0, 1.0 ) );
}
bool menu_music_vol_down_enabled() {
  CONDUCTOR_INFO_OR_RETURN_FALSE( info );
  if( !info.volume.has_value() ) return false;
  return *info.volume > 0.0;
}

void menu_music_set_player() {
  ui::select_box_enum<e_music_player>(
      "Select Music Player",
      /*on_result=*/[]( e_music_player result ) {
        if( !set_music_player( result ) )
          (void)ui::message_box(
              "The \"{}\" music player is not available.",
              result );
      } );
}
bool menu_music_set_player_enabled() {
  CONDUCTOR_INFO_OR_RETURN_FALSE( info );
  (void)info;
  return true;
}

MENU_ITEM_HANDLER( music_play, menu_music_play,
                   menu_music_play_enabled );
MENU_ITEM_HANDLER( music_stop, menu_music_stop,
                   menu_music_stop_enabled );
MENU_ITEM_HANDLER( music_pause, menu_music_pause,
                   menu_music_pause_enabled );
MENU_ITEM_HANDLER( music_resume, menu_music_resume,
                   menu_music_resume_enabled );
MENU_ITEM_HANDLER( music_next, menu_music_next,
                   menu_music_next_enabled );
MENU_ITEM_HANDLER( music_prev, menu_music_prev,
                   menu_music_prev_enabled );
MENU_ITEM_HANDLER( music_vol_up, menu_music_vol_up,
                   menu_music_vol_up_enabled );
MENU_ITEM_HANDLER( music_vol_down, menu_music_vol_down,
                   menu_music_vol_down_enabled );
MENU_ITEM_HANDLER( music_set_player, menu_music_set_player,
                   menu_music_set_player_enabled );

// Testing
void test() {
  lg.info( "testing Music Conductor" );
  ACTIVE_MUSIC_PLAYER_OR_RETURN( mplayer );

  double vol = 1.0;

  while( true ) {
    // Wait for music player to consume commands.
    mplayer.fence();
    auto st = state();
    if( !st ) break;
    st.value().log();
    lg.info(
        "[p]lay, [n]ext, pre[v], p[a]use, [r]esume, [s]top, "
        "[u]p volume, [d]own volume, s[t]ate, [q]uit: " );
    string in;
    cin >> in;
    sleep( chrono::milliseconds( 20 ) );
    if( in == "q" ) break;
    if( in == "p" ) {
      play();
      continue;
    }
    if( in == "n" ) {
      next();
      continue;
    }
    if( in == "v" ) {
      prev();
      continue;
    }
    if( in == "a" ) {
      pause();
      continue;
    }
    if( in == "r" ) {
      resume();
      continue;
    }
    if( in == "s" ) {
      stop();
      continue;
    }
    if( in == "u" ) {
      vol += .1;
      vol = std::clamp( vol, 0.0, 1.0 );
      set_volume( vol );
      continue;
    }
    if( in == "d" ) {
      vol -= .1;
      vol = std::clamp( vol, 0.0, 1.0 );
      set_volume( vol );
      continue;
    }
    if( in == "t" ) { continue; }
  }
}

} // namespace rn::conductor
