/****************************************************************
* sound.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-29.
*
* Description: All things sound.
*
*****************************************************************/
#include "sound.hpp"

#include "base-util.hpp"
#include "macros.hpp"

#include <SDL.h>
#include <SDL_mixer.h>

#include <iostream>

using namespace std;

namespace rn {

namespace {

::Mix_Music* current_music = nullptr;

void stop_music_if_playing() {
  if( ::Mix_PlayingMusic() ) {
    CHECK( current_music );
    ::Mix_HaltMusic();
    ::Mix_FreeMusic( current_music );
    current_music = nullptr;
  }
}

} // namespace

void cleanup_sound() {
  stop_music_if_playing();
}

bool play_music_file( char const* file ) {
  stop_music_if_playing();
  current_music = ::Mix_LoadMUS( file );
  if( !current_music ) {
    cerr << "Mix_LoadMUS error: " << ::SDL_GetError() << "\n";
    return false;
  }
  // Start Playback
  if( ::Mix_PlayMusic( current_music, 1) == 0 )
    return true;

  cerr << "Mix_PlayMusic error: " << ::SDL_GetError() << "\n";
  ::Mix_FreeMusic( current_music );
  current_music = nullptr;
  return false;
}

} // namespace rn

