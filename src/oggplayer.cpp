/****************************************************************
**oggplayer.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-06-08.
*
* Description: Music Player for ogg files.
*
*****************************************************************/
#include "oggplayer.hpp"

// Revolution Now
#include "aliases.hpp"
#include "config-files.hpp"
#include "init.hpp"
#include "logging.hpp"

// Revolution Now (config)
#include "../config/ucl/music.inl"

// SDL
#include "SDL.h"
#include "SDL_mixer.h"

// C++ standard library
#include <cmath>

using namespace std;

namespace rn {

namespace {

class OggTune {
public:
  NON_COPYABLE( OggTune );

  OggTune( TuneId id, ::Mix_Music* music )
    : ptr_( music ), id_( id ) {
    DCHECK( music );
  }

  ~OggTune() {
    // ptr could be null if we've been moved out of.
    if( ptr_ ) ::Mix_FreeMusic( ptr_ );
  }

  OggTune( OggTune&& rhs ) noexcept {
    ptr_ = std::exchange( rhs.ptr_, nullptr );
    id_  = std::exchange( rhs.id_, 0 );
  }

  OggTune& operator=( OggTune&& rhs ) noexcept {
    rhs.swap( *this );
    return *this;
  }

  ::Mix_Music* ptr() { return ptr_; }
  TuneId       id() { return id_; }

  // TODO: add duration support. SDL_Mixer doesn't seem to have
  // anything in its API to get the duration of music, so it may
  // be necessary to add libogg directly as a dependency to find
  // the length.
  Opt<chrono::milliseconds> duration() { return nothing; }

  void swap( OggTune& rhs ) noexcept {
    ::std::swap( ptr_, rhs.ptr_ );
    ::std::swap( id_, rhs.id_ );
  }

private:
  ::Mix_Music* ptr_;
  TuneId       id_;
};
NOTHROW_MOVE( OggTune );

enum class e_ogg_state { playing, paused, stopped };

/****************************************************************
** Global State
*****************************************************************/
Opt<OggMusicPlayer> g_ogg_player{};
Opt<OggTune>        g_current_music{};
// Note that this state may not be updated when a tune stops
// playing on its own (but before the next one is played). If
// this turns out to be a problem then we would either need to
// hook into some callback from SDL Mixer or poll for it.
e_ogg_state g_state{ e_ogg_state::stopped };

/****************************************************************
** Impl Functions
*****************************************************************/
bool is_playing() { return ::Mix_PlayingMusic() != 0; }

void update_state_if_stopped() {
  if( !is_playing() ) {
    g_current_music.reset();
    g_state = e_ogg_state::stopped;
  }
}

void stop_music_if_playing() {
  if( is_playing() ) {
    CHECK( g_state == e_ogg_state::playing ||
           g_state == e_ogg_state::paused );
    CHECK( g_current_music, "Internal error." );
    ::Mix_HaltMusic();
    g_current_music.reset();
    g_state = e_ogg_state::stopped;
  } else {
    // May not be necessary, but...
    update_state_if_stopped();
  }
}

// Takes ownership of pointer.
ND bool play_impl( OggTune&& music ) {
  stop_music_if_playing();
  // Start Playback
  auto channel_used =
      ::Mix_PlayMusic( music.ptr(), /*loops=*/1 );
  if( channel_used < 0 ) {
    lg.error( "unexpected error: unable to play music: {}",
              ::SDL_GetError() );
    return false;
  }
  g_current_music = std::move( music );
  g_state         = e_ogg_state::playing;
  return true;
}

Opt<OggTune> load_tune( TuneId id ) {
  auto file = config_music.ogg_folder /
              fs::path( tune_stem_from_id( id ) + ".ogg" );
  auto* music = ::Mix_LoadMUS( file.string().c_str() );
  if( !music ) {
    lg.error(
        "Failed to load OGG file `{}` for tune `{}`: "
        "Mix_LoadMUS error: {}",
        file.string(), tune_stem_from_id( id ),
        ::SDL_GetError() );
    return nothing;
  }
  return OggTune( id, music );
}

void cleanup_oggplayer() {
  stop_music_if_playing();
  update_state_if_stopped();
}

} // namespace

// Outside of anonymous namespace otherwise it apparently cannot
// be a "friend" of the OggMusicPlayer class, which it needs
// to be to instantiate it.
void init_oggplayer() {
  g_state    = e_ogg_state::stopped;
  auto flags = ::Mix_Init( MIX_INIT_OGG );
  if( !( flags & MIX_INIT_OGG ) ) {
    lg.warn(
        "Failed to initialize SDL Mixer with OGG support; OGG "
        "player will not be enabled." );
    return;
  }

  lg.info(
      "SDL Mixer OGG support enabled: enabling Music Player." );
  g_ogg_player = OggMusicPlayer();
}

REGISTER_INIT_ROUTINE( oggplayer );

/****************************************************************
** OggMusicPlayer Implementation
*****************************************************************/
pair<MusicPlayerDesc, MaybeMusicPlayer>
OggMusicPlayer::player() {
  auto name = "OGG Music Player";
  auto description =
      "Music Player that players OGG Vorbis files.";
  auto how_it_works = "Uses SDL backend.";

  auto desc = MusicPlayerDesc{
      /*name=*/name,
      /*description=*/description,
      /*how_it_works=*/how_it_works,
  };

  if( g_ogg_player.has_value() ) {
    return {
        desc,
        &( *g_ogg_player ),
    };
  } else {
    return {
        desc,
        UNEXPECTED( "OGG player failed to initialize" ),
    };
  }
}

bool OggMusicPlayer::good() const {
  // If it initializes successfully once, then just assume that
  // it won't break thereafter.
  return g_ogg_player.has_value();
}

Opt<TunePlayerInfo> OggMusicPlayer::can_play_tune( TuneId id ) {
  if( !good() ) return {};

  auto ogg = load_tune( id );
  if( !ogg ) return nothing;

  return TunePlayerInfo{ /*id=*/id,
                         /*length=*/( *ogg ).duration(),
                         /*progress=*/nothing };
}

bool OggMusicPlayer::play( TuneId id ) {
  if( !good() ) return false;
  auto ogg = load_tune( id );
  if( !ogg ) return false;
  lg.debug( "OggMusicPlayer: playing tune `{}`",
            tune_display_name_from_id( id ) );
  return play_impl( std::move( *ogg ) );
}

void OggMusicPlayer::stop() {
  if( !good() ) return;
  stop_music_if_playing();
}

MusicPlayerDesc OggMusicPlayer::info() const {
  return OggMusicPlayer::player().first;
}

MusicPlayerState OggMusicPlayer::state() const {
  Opt<TunePlayerInfo> maybe_tune_info;
  update_state_if_stopped();
  bool is_playing = ( g_state == e_ogg_state::playing );
  bool is_paused  = ( g_state == e_ogg_state::paused );
  if( is_paused ) { CHECK( ::Mix_PausedMusic() ); }
  if( is_playing || is_paused ) {
    CHECK( g_current_music );
    maybe_tune_info = TunePlayerInfo{
        /*id=*/g_current_music->id(),
        /*length=*/g_current_music->duration(),
        /*progress=*/nothing,
    };
  }
  return { /*tune_info=*/maybe_tune_info,
           /*is_paused=*/is_paused };
}

MusicPlayerCapabilities OggMusicPlayer::capabilities() const {
  return {
      /*can_pause=*/true,
      /*has_volume=*/true,
      /*has_progress=*/false,
      /*has_tune_duration=*/false,
      /*can_seek=*/false,
  };
}

bool OggMusicPlayer::fence( Opt<Duration_t> /*unused*/ ) {
  if( !good() ) return true;
  // Since this music player is synchronous the `fence` operation
  // is a no-op.
  return true;
}

bool OggMusicPlayer::is_processing() const {
  if( !good() ) return false;
  // Since this music player is synchronous it is assumed to
  // never be in a state of processing commands (at least not
  // that we can observe).
  return false;
}

void OggMusicPlayer::pause() {
  if( !good() ) return;
  if( g_state == e_ogg_state::playing ) {
    CHECK( g_current_music );
    lg.debug( "OggMusicPlayer: pause" );
    g_state = e_ogg_state::paused;
    ::Mix_PauseMusic();
  }
}

void OggMusicPlayer::resume() {
  if( !good() ) return;
  if( g_state == e_ogg_state::paused ) {
    CHECK( g_current_music );
    CHECK( ::Mix_PausedMusic() );
    lg.debug( "OggMusicPlayer: resume" );
    g_state = e_ogg_state::playing;
    ::Mix_ResumeMusic();
  }
}

void OggMusicPlayer::set_volume( double volume ) {
  if( !good() ) return;
  auto sdl_volume = std::lround( volume * 127.0 );
  sdl_volume      = std::clamp( sdl_volume, 0l, 127l );
  ::Mix_VolumeMusic( sdl_volume );
}

} // namespace rn
