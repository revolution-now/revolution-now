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

// Revolution Now (config)
#include "config/sound.inl"

// abseil
#include "absl/container/flat_hash_map.h"

// SDL
#include "SDL.h"
#include "SDL_mixer.h"

using namespace std;

namespace rn {

namespace {

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
  // Make sure that we're dynamically linked with a version of
  // SDL_mixer approximately like the one we compiled with.
  ::SDL_version      compiled_version;
  SDL_version const* link_version = ::Mix_Linked_Version();
  SDL_MIXER_VERSION( &compiled_version );
  logger->info( "SDL mixer: compiled with version: {}.{}.{}",
                compiled_version.major, compiled_version.minor,
                compiled_version.patch );
  logger->info( "SDL mider: running with version: {}.{}.{}",
                link_version->major, link_version->minor,
                link_version->patch );
  CHECK(
      compiled_version.major == link_version->major,
      "This game was compiled with a version of SDL Mixer whose "
      "major version number ({}) is different from the major "
      "version number of the SDL Mixer runtime library ({})",
      compiled_version.major, link_version->major );

  if( compiled_version.minor != link_version->minor ) {
    logger->warn(
        "This game was compiled with a version of SDL Mixer "
        "whose "
        "minor version number ({}) is different from the minor "
        "version number of the SDL Mixer runtime library ({})",
        compiled_version.minor, link_version->minor );
  }

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
  int      frequency{0};
  uint16_t format{0};
  int      channels{0};
  auto     audio_opened =
      ::Mix_QuerySpec( &frequency, &format, &channels );

  CHECK(
      audio_opened,
      "unexpected: SDL Mixer audio has not been initialized." );

  logger->info( "Opening audio with {} channels @ {}Hz.",
                channels, frequency );

  // Set Volume for all channels.
  //
  // TODO: is this necessary given that we also set the volume
  // per chunk for sfx and set the volume for music separately?
  constexpr int default_volume{10}; // [0, 128)
  ::Mix_Volume( -1 /*=all channels*/, default_volume );

  for( auto sound : values<e_sfx> ) load_sfx( sound );
}

void cleanup_sound() {
  for( auto& p : loaded_sfx ) ::Mix_FreeChunk( p.second );
  ::Mix_CloseAudio();
  ::Mix_Quit();
}

} // namespace

REGISTER_INIT_ROUTINE( sound );

void play_sound_effect( e_sfx sound ) {
  auto* chunk = load_sfx( sound );
  if( ::Mix_PlayChannel( -1, chunk, 0 ) == -1 )
    logger->warn( "Unable to play sound effect {}", sound );
}

} // namespace rn
