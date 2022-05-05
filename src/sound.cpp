/****************************************************************
**sound.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-29.
*
* Description: All things sound.
*
*****************************************************************/
#include "sound.hpp"

// Revolution Now
#include "error.hpp"
#include "init.hpp"
#include "logger.hpp"
#include "sdl-util.hpp"
#include "util.hpp"

// config
#include "config/sound.rds.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// SDL
#include "SDL.h"
#include "SDL_mixer.h"

// C++ standard library
#include <unordered_map>

using namespace std;

namespace rn {

namespace {

struct SfxDesc {
  string file;
  int    volume;
};
NOTHROW_MOVE( SfxDesc );

#define SFX_FILE( sound )                                   \
  case e_sfx::sound:                                        \
    return {                                                \
      config_sound.sfx.sound, config_sound.sfx.volume.sound \
    }

SfxDesc sfx_file_for( e_sfx sound ) {
  switch( sound ) {
    SFX_FILE( move );
    SFX_FILE( attacker_lost );
    SFX_FILE( attacker_won );
  }
  SHOULD_NOT_BE_HERE;
}

unordered_map<e_sfx, ::Mix_Chunk*> loaded_sfx;

auto* load_sfx( e_sfx sound ) {
  if( !loaded_sfx.contains( sound ) ) {
    auto [file, vol] = sfx_file_for( sound );
    auto* chunk      = ::Mix_LoadWAV( file.c_str() );
    CHECK( chunk, "failed to load sound effect file {}", file );
    loaded_sfx[sound] = chunk;
    ::Mix_VolumeChunk( chunk, vol );
  }
  return loaded_sfx[sound];
}

void init_sound() {
  // Make sure that we're dynamically linked with a version of
  // SDL_mixer approximately like the one we compiled with.
  ::SDL_version      compiled_version;
  SDL_version const* link_version = ::Mix_Linked_Version();
  SDL_MIXER_VERSION( &compiled_version );
  check_SDL_compile_link_version( "Mixer", *link_version,
                                  compiled_version );

  // Seemingly not needed?
  //::Mix_Init( ... );

  // Open Audio device
  CHECK( !Mix_OpenAudio( config_sound.general.frequency,
                         AUDIO_S16SYS,
                         config_sound.general.channels,
                         config_sound.general.chunk_size ),
         "could not open audio: Mix_OpenAudio error: {}",
         ::Mix_GetError() );

  // Verify settings.
  int      frequency{ 0 };
  uint16_t format{ 0 };
  int      channels{ 0 };
  auto     audio_opened =
      ::Mix_QuerySpec( &frequency, &format, &channels );

  CHECK(
      audio_opened,
      "unexpected: SDL Mixer audio has not been initialized." );

  lg.info( "opening audio with {} channels @ {}Hz.", channels,
           frequency );

  for( int i = 0; i < ::SDL_GetNumAudioDrivers(); ++i )
    lg.debug( "audio driver #{}: {}", i,
              ::SDL_GetAudioDriver( i ) );

  lg.info( "using audio driver: {}",
           ::SDL_GetCurrentAudioDriver() );

  // Set Volume for all channels.
  //
  // TODO: is this necessary given that we also set the volume
  // per chunk for sfx and set the volume for music separately?
  constexpr int default_volume{ 10 }; // [0, 128)
  ::Mix_Volume( -1 /*=all channels*/, default_volume );

  for( auto sound : refl::enum_values<e_sfx> ) load_sfx( sound );
}

void cleanup_sound() {
  for( auto& p : loaded_sfx ) ::Mix_FreeChunk( p.second );
  // FIXME: On Linux sometimes the program will hang inside this
  // Mix_CloseAudio function. This has been determined to be due
  // to pulseaudio, specifically, when another application is
  // using pulseaudio (or possibly alsa which then uses pulseau-
  // dio) this function will do a thread_join and hang while
  // waiting for the other application to stop using pulseaudio
  // (closing that other application will then immediately cause
  // the program to proceed). E.g., if fluidsynth is running and
  // we are hanging here then quitting fluidsynth should stop the
  // hanging. Not yet clear what the proper fix is for this.
  ::Mix_CloseAudio();
  ::Mix_Quit();
}

} // namespace

REGISTER_INIT_ROUTINE( sound );

void play_sound_effect( e_sfx sound ) {
  auto* chunk = load_sfx( sound );
  if( ::Mix_PlayChannel( -1, chunk, 0 ) == -1 )
    lg.warn( "unable to play sound effect {}", sound );
}

void linker_dont_discard_module_sound() {}

} // namespace rn
