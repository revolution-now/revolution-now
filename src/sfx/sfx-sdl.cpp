/****************************************************************
**sfx-sdl.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-27.
*
* Description: Implementation of ISfx using an SDL Mixer backend.
*
*****************************************************************/
#include "sfx-sdl.hpp"

// config
#include "config/sound.rds.hpp"

// sdl
#include "sdl/include-sdl-mixer.hpp"
#include "sdl/init.hpp"

// refl
#include "refl/enum-map.hpp"
#include "refl/to-str.hpp"

// base
#include "base/fmt.hpp"
#include "base/logger.hpp"

using namespace std;

namespace sfx {

namespace {

using ::base::lg;
using ::refl::enum_map;
using ::refl::enum_values;
using ::rn::config_sound;
using ::rn::e_sfx;

} // namespace

/****************************************************************
** SfxSDL::Impl
*****************************************************************/
struct SfxSDL::Impl {
  ::Mix_Chunk* load_effect( e_sfx const sfx ) {
    if( !effects_[sfx] ) {
      auto const& conf = config_sound.effects[sfx];
      auto* chunk      = ::Mix_LoadWAV( conf.file.c_str() );
      CHECK( chunk, "failed to load sound effect file {}",
             conf.file );
      effects_[sfx] = chunk;
      ::Mix_VolumeChunk( chunk, conf.volume );
    }
    return effects_[sfx];
  }

  void play_sound_effect( e_sfx const sfx ) const {
    ::Mix_Chunk* chunk = effects_[sfx];
    CHECK( chunk != nullptr,
           "the {} sound effect has not been loaded.", sfx );
    if( ::Mix_PlayChannel( -1, chunk, 0 ) == -1 )
      lg.warn( "unable to play sound effect {}", sfx );
  }

  void load_all_sfx() {
    for( e_sfx const sfx : enum_values<e_sfx> )
      load_effect( sfx );
  }

  void free_all_sfx() {
    for( e_sfx const sfx : enum_values<e_sfx> ) {
      ::Mix_Chunk* chunk = effects_[sfx];
      CHECK( chunk != nullptr,
             "the {} sound effect has not been loaded.", sfx );
      ::Mix_FreeChunk( chunk );
    }
  }

  void init_mixer() {
    // Make sure that we're dynamically linked with a version of
    // SDL_mixer approximately like the one we compiled with.
    ::SDL_version compiled_version;
    SDL_version const* link_version = ::Mix_Linked_Version();
    SDL_MIXER_VERSION( &compiled_version );
    sdl::check_SDL_compile_link_version( "Mixer", *link_version,
                                         compiled_version );

    // Seemingly not needed?
    //::Mix_Init( ... );

    // Open Audio device
    CHECK( !::Mix_OpenAudio( config_sound.general.frequency,
                             AUDIO_S16SYS,
                             config_sound.general.channels,
                             config_sound.general.chunk_size ),
           "could not open audio: Mix_OpenAudio error: {}",
           ::Mix_GetError() );

    // Verify settings.
    int frequency{ 0 };
    uint16_t format{ 0 };
    int channels{ 0 };
    auto audio_opened =
        ::Mix_QuerySpec( &frequency, &format, &channels );

    CHECK( audio_opened,
           "unexpected: SDL Mixer audio has not been "
           "initialized." );

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
  }

  void deinit_mixer() {
    // FIXME: On Linux sometimes the program will hang inside
    // this Mix_CloseAudio function. This has been determined to
    // be due to pulseaudio, specifically, when another applica-
    // tion is using pulseaudio (or possibly alsa which then uses
    // pulseaudio) this function will do a thread_join and hang
    // while waiting for the other application to stop using
    // pulseaudio (closing that other application will then imme-
    // diately cause the program to proceed). E.g., if fluidsynth
    // is running and we are hanging here then quitting flu-
    // idsynth should stop the hanging. Not yet clear what the
    // proper fix is for this.
    ::Mix_CloseAudio();
    ::Mix_Quit();
  }

  enum_map<e_sfx, ::Mix_Chunk*> effects_;
};

/****************************************************************
** SfxSDL
*****************************************************************/
SfxSDL::SfxSDL() : pimpl_( new Impl ) {}

SfxSDL::~SfxSDL() = default;

void SfxSDL::play_sound_effect( e_sfx sound ) const {
  pimpl_->play_sound_effect( sound );
}

void SfxSDL::load_all_sfx() { return pimpl_->load_all_sfx(); }

void SfxSDL::free_all_sfx() { return pimpl_->free_all_sfx(); }

void SfxSDL::init_mixer() { return pimpl_->init_mixer(); }

void SfxSDL::deinit_mixer() { return pimpl_->deinit_mixer(); }

} // namespace sfx
