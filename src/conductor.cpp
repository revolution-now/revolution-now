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
#include "init.hpp"
#include "logging.hpp"

using namespace std;

namespace rn::conductor {

namespace {

void init_conductor() {
  //
}

void cleanup_conductor() {
  //
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
