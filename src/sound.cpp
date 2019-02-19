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
#include "config-files.hpp"
#include "errors.hpp"
#include "init.hpp"
#include "logging.hpp"
#include "util.hpp"

// abseil
#include "absl/container/flat_hash_map.h"

// SDL
#include "SDL.h"
#include "SDL_mixer.h"

using namespace std;

namespace rn {

namespace {

::Mix_Music* current_music = nullptr;

void stop_music_if_playing() {
  if( ::Mix_PlayingMusic() != 0 ) {
    CHECK( current_music );
    ::Mix_HaltMusic();
    ::Mix_FreeMusic( current_music );
    current_music = nullptr;
  }
}

struct SfxDesc {
  string file;
  int    volume;
};

#define SFX_FILE( sound )                                   \
  case +e_sfx::sound:                                       \
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

absl::flat_hash_map<e_sfx, ::Mix_Chunk*> loaded_sfx;

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
  for( auto sound : values<e_sfx> ) load_sfx( sound );
}

void cleanup_sound() {
  stop_music_if_playing();
  for( auto& p : loaded_sfx ) ::Mix_FreeChunk( p.second );
}

} // namespace

REGISTER_INIT_ROUTINE( sound, init_sound, cleanup_sound );

bool play_music_file( char const* file ) {
  stop_music_if_playing();
  current_music = ::Mix_LoadMUS( file );
  if( current_music == nullptr ) {
    cerr << "Mix_LoadMUS error: " << ::SDL_GetError() << "\n";
    return false;
  }
  // Start Playback
  if( ::Mix_PlayMusic( current_music, 1 ) == 0 ) return true;

  cerr << "Mix_PlayMusic error: " << ::SDL_GetError() << "\n";
  ::Mix_FreeMusic( current_music );
  current_music = nullptr;
  return false;
}

void play_sound_effect( e_sfx sound ) {
  auto* chunk = load_sfx( sound );
  if( ::Mix_PlayChannel( -1, chunk, 0 ) == -1 )
    logger->warn( "Unable to play sound effect {}", sound );
}

} // namespace rn
