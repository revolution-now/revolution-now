/****************************************************************
**mplayer.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-05-31.
*
* Description: Uniform interface for music player subsystems.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "errors.hpp"
#include "tune.hpp"

// base-util
#include "base-util/non-copyable.hpp"

namespace rn {

class MusicPlayer;

struct MusicPlayerInfo {
  // E.g. "MIDI File Player"
  std::string name;

  // E.g. "Plays MIDI files using a player-supplied softsynth"
  std::string description;

  // E.g. "To use this music player you must have..."
  std::string how_it_works;

  // Will be non-null if the player successfully initialized and
  // is able to play music. Otherwise will contain an error indi-
  // cating why.
  expect<Ref<MusicPlayer>> player;
};

struct MusicPlayerState {
  void log() const;

  // TuneInfo struct for currently playing tune, if any.
  Opt<TuneInfo> tune_info;
  // If the player is currently playing a tune then it will re-
  // turn a number in [0,1.0] representing the progress through
  // the tune. Returns `nullopt` if no tune is playing or if the
  // most recent tune has finished.
  Opt<double> progress;
  // If the player is paused in the middle of a tune.
  bool is_paused;
  // If the player has a notion of settable volume then this will
  // be populated.
  Opt<double> volume;
};

struct MusicPlayerCapabilities {
  void log() const;

  bool can_pause{false}; // implies that it can also resume
  bool has_volume{false};
  bool has_progress{false};
  bool has_tune_duration{false};
  bool can_seek{false};
};

// It is important to note when using this class that, in gen-
// eral, calling the member functions may not cause instantaneous
// change to the real underlying music player for which this
// class is a facade. This is because some music players run in
// other threads and so these interface methods may simply add
// commands into a queue to be consumed asynchronously by the
// music-playing thread. Hence, change may not take effect imme-
// diately and in some cases operations may happen out of order.
//
// To help with this there is a method called `fence`. Calling it
// will cause the calling thread to block until all previous com-
// mands sent to the music player have been processed. It also
// accepts a timeout to avoid hanging if something goes wrong.
class MusicPlayer : public util::movable_only {
public:
  // Check if the player is enabled. If the Music Player object
  // exists then it should be enabled, unless either a) it failed
  // to initialize or b) a fatal error happens within the player
  // (even after initialization) in which case it may disable it-
  // self. This method should be checked before calling API.
  virtual bool good() const = 0;

  // Verifies that the tune exists on the filesystem in a format
  // that can be read by this player and that the file can be
  // opened. If successfull then it will return a TuneInfo (so
  // this means that it actually has to load and parse the file).
  virtual Opt<TuneInfo> can_play_tune( TuneId id ) = 0;

  // Start playing the given tune from the beginning. When the
  // tune finishes it stops playing until it is given another
  // play command. Returns false if the tune cannot be played.
  virtual bool play( TuneId id ) = 0;

  // If playing will stop the player from playing music.
  virtual void stop() = 0;

  // Returns some general information about this music player.
  virtual MusicPlayerInfo info() const = 0;

  // Returns a structure giving the current state of the player,
  // such as whether it is playing a tune, progress through the
  // tune, volume, etc.
  virtual MusicPlayerState state() const = 0;

  // Returns info on what this music player is capable of doing.
  // E.g., can it seek within a tune, can it adjust volume, etc.
  virtual MusicPlayerCapabilities capabilities() const = 0;

  // -- Synchronization ---------------------------------------
  // To help ensure that operations on the music player are
  // well-ordered one calls `fence` before an operation that must
  // wait until previous operations have completed before run-
  // ning. Calling it will cause the calling thread to block
  // until all previous commands sent to the music player have
  // been processed.
  //
  // It also accepts a timeout to avoid hanging if something goes
  // wrong. If a timeout occurs then the function returns false,
  // otherwise true.
  virtual bool fence( Opt<Duration_t> timeout = std::nullopt );

  // This one is a more passive aid in making sure that music
  // player commands are well-ordered. It simply returns true the
  // music player is still processing previous commands. It is
  // guaranteed safe to send new commands to the music player re-
  // gardless of whether it is processing or not, but one will
  // might want to call this to aid in e.g. disabling GUI ele-
  // ments while waiting for the music player to enact the last
  // command.
  virtual bool is_processing() const;

  // -- Optional Extensions -----------------------------------
  // The functions below may or may not be supported by a given
  // music player. If called on a player that doesn't support
  // them they will throw an exception on a debug build and just
  // log an error on a release build.

  // Pause playing of current tune. No-op if no tune is playing
  // or most recent tune has finished.
  virtual void pause();

  // Resume playing of paused tune. No-op if no tune is playing
  // or if most recent tune has finished.
  virtual void resume();

  // Get/Set current volume [0, 1.0].
  virtual void set_volume( double volume );

  // Seek to given position in current tune. position in [0,1].
  virtual void seek( double position );
};

// For testing
void test_music_player_impl( MusicPlayer& mplayer );

template<typename MusicPlayerT>
void test_music_player() {
  auto mplayer_info = MusicPlayerT::player();
  if( mplayer_info.player.has_value() )
    test_music_player_impl( *mplayer_info.player );
}

} // namespace rn
