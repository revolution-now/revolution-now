/****************************************************************
**midiplayer.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-05-31.
*
* Description: MusicPlayer classes with various MIDI backends.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "mplayer.hpp"

namespace rn {

// A MusicPlayer with a MIDI sequencer backend.
class MidiSeqMusicPlayer : public MusicPlayer {
 public:
  static std::pair<MusicPlayerDesc, MaybeMusicPlayer> player();

  // Implement MusicPlayer
  bool good() const override;

  // Implement MusicPlayer
  maybe<TunePlayerInfo> can_play_tune( TuneId id ) override;

  // Implement MusicPlayer
  bool play( TuneId id ) override;

  // Implement MusicPlayer
  void stop() override;

  MusicPlayerDesc info() const override;

  // Implement MusicPlayer
  MusicPlayerState state() const override;

  // Implement MusicPlayer
  MusicPlayerCapabilities capabilities() const override;

  // Implement MusicPlayer
  bool fence( maybe<Duration_t> /*unused*/ ) override;

  // Implement MusicPlayer
  bool is_processing() const override;

  // Implement MusicPlayer
  void pause() override;

  // Implement MusicPlayer
  void resume() override;

  // Implement MusicPlayer
  void set_volume( double volume ) override;

 private:
  MidiSeqMusicPlayer() = default;
  friend void           init_midiplayer();
  maybe<TunePlayerInfo> last_played_tune_info_;
};

} // namespace rn
