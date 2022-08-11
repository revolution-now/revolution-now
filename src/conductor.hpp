/****************************************************************
**conductor.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-06-02.
*
* Description: Main interface for music playing.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "expect.hpp"
#include "maybe.hpp"
#include "tune.hpp"

// Rds
#include "conductor.rds.hpp"

// C++ standard library
#include <functional>

namespace rn {
struct IRand;
}

namespace rn::conductor {

// Add more events here as they are needed.
enum class e_conductor_event {
  volume_changed, //
  mplayer_changed //
};

struct ConductorInfo {
  e_music_player        mplayer;
  e_music_state         music_state;
  maybe<TunePlayerInfo> playing_now;
  maybe<double>         volume;
  bool                  autoplay;

  void log() const;
};
NOTHROW_MOVE( ConductorInfo );

struct MusicPlayerInfo {
  valid_or<std::string> enabled = "uninitialized";
  std::string           name;
  std::string           description;
  std::string           how_it_works;

  void log() const;
};
NOTHROW_MOVE( MusicPlayerInfo );

MusicPlayerInfo const& music_player_info(
    e_music_player mplayer );

// Stops any playing music and changes the music player. Returns
// true if success and false otherwise. The new music player must
// be enabled; if not then this will log an error and be a no-op.
bool set_music_player( e_music_player mplayer );

// Get the current state of the conductor. If no music players
// are available then we get an `unexpected`.
maybe<ConductorInfo> state();

using ConductorEventFunc = std::function<void( void )>;

// This will allow other code to recieve messages when the con-
// ductor performs certain events. It will always send the noti-
// fication after the event. All functions are called in the
// game's main thread.
void subscribe_to_conductor_event( e_conductor_event  event,
                                   ConductorEventFunc func );

// If this is off then the conductor will never automatically ad-
// vance to the next tune when one is complete.
void set_autoplay( bool enabled );

// This will reset the conductor state to initial settings that
// it would have just after initialization.  Music will stop.
void reset();

// Play. If paused this will resume. If not paused it will start
// playing the next tune in the playlist. When that tune is fin-
// ished it will continue playing the playlist if autoplay is en-
// abled. If already playing this is a no-op.
void play();

// Skip to beginning of this tune if > 5% progress, or previous
// tune if < 5% progress. If progress is not available then just
// skip to previous tune.
void prev();

// Skip to next tune in playlist. If we are paused this will go
// into the stop state after advancing to the next tune. If we
// are stopped then it will just advance to the next tune. In any
// case it will not start player.
void next();

// Stop playing current tune (if playing) and forget current
// place in tune.
void stop();

// If supported, stop playing but remember place in tune.
void pause();

// If supported, and if paused, resume playing.
void resume();

// If supported, sets the volume.
void set_volume( double vol );

// If supported, jump to a certain place [0,1.0] in tune.
void seek( double pos );

// Playlists.

// Generate a random playlist. If one exists it will be regener-
// ated (we will not merely shuffle here because the playlist
// must satisfy some invariants).
void playlist_generate( IRand& rand );

enum class e_request_probability { always, sometimes, rarely };

void play_request( IRand& rand, e_request request,
                   e_request_probability probability );

// Testing
void test();

} // namespace rn::conductor
