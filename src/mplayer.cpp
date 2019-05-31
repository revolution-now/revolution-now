/****************************************************************
**mplayer.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-05-31.
*
* Description: Uniform interface for music player subsystems.
*
*****************************************************************/
#include "mplayer.hpp"

// Revolution Now
#include "logging.hpp"

using namespace std;

namespace rn {

namespace {

class SilentMusicPlayer : public MusicPlayer {
public:
  static MusicPlayerInfo player() {
    static SilentMusicPlayer player;
    return {
        /*name=*/"Silent Music Player",
        /*description=*/"For testing; does not play music",
        /*how_it_works=*/"It doesn't.",
        /*player=*/player,
    };
  }

  // Implement MusicPlayer
  Opt<TuneInfo> can_play_tune( TuneId id ) override {
    logger->info( "SilentMusicPlayer: can_play_tune" );
    return TuneInfo{/*id=*/id,
                    /*length=*/chrono::minutes( 1 )};
  }

  // Implement MusicPlayer
  bool play( TuneId id ) override {
    logger->info( "SilentMusicPlayer: playing tune `{}`",
                  tune_display_name_from_id( id ) );
    id_ = id;
    return true;
  }

  // Implement MusicPlayer
  void stop() override {
    logger->info( "SilentMusicPlayer: stop" );
    id_ = nullopt;
  }

  MusicPlayerInfo info() const override {
    return SilentMusicPlayer::player();
  }

  // Implement MusicPlayer
  MusicPlayerState state() const override {
    Opt<TuneInfo> maybe_tune_info;
    if( id_.has_value() ) {
      maybe_tune_info =
          TuneInfo{/*id=*/*id_,
                   /*length=*/chrono::minutes( 1 )};
    }
    return {/*tune_info=*/maybe_tune_info,
            /*progress=*/0.5,
            /*is_paused=*/is_paused_,
            /*volume=*/nullopt};
  }

  // Implement MusicPlayer
  MusicPlayerCapabilities capabilities() const override {
    return {
        /*can_pause=*/true,
        /*has_volume=*/false,
        /*has_progress=*/true,
        /*has_tune_duration=*/true,
        /*can_seek=*/false,
    };
  }

  // Implement MusicPlayer
  bool fence( Opt<Duration_t> /*unused*/ ) override {
    return true;
  }

  // Implement MusicPlayer
  bool is_processing() const override { return false; }

  // Implement MusicPlayer
  void pause() override {
    logger->info( "SilentMusicPlayer: pause" );
    is_paused_ = true;
  }

  // Implement MusicPlayer
  void resume() override {
    logger->info( "SilentMusicPlayer: resume" );
    is_paused_ = false;
  }

private:
  SilentMusicPlayer() = default;

  bool        is_paused_{false};
  Opt<TuneId> id_{};
};

} // namespace

void MusicPlayerState::log() const {
  logger->info( "MusicPlayerState:" );
  if( tune_info.has_value() ) {
    logger->info( "  tune_info.id:     {} ({})", tune_info->id._,
                  tune_display_name_from_id( tune_info->id ) );
    if( tune_info->length.has_value() )
      logger->info( "  tune_info.length: {}sec",
                    chrono::duration_cast<chrono::seconds>(
                        tune_info->length.value() )
                        .count() );
  }
  if( progress.has_value() )
    logger->info( "  progress:         {}", *progress );
  logger->info( "  is_paused:        {}", is_paused );
  if( volume.has_value() )
    logger->info( "  volume:           {}", *volume );
}

void MusicPlayerCapabilities::log() const {
  logger->info( "MusicPlayerCapabilities:" );
  logger->info( "  can_pause:         {}", can_pause );
  logger->info( "  has_volume:        {}", has_volume );
  logger->info( "  has_progress:      {}", has_progress );
  logger->info( "  has_tune_duration: {}", has_tune_duration );
  logger->info( "  can_seek:          {}", can_seek );
}

bool MusicPlayer::fence( Opt<Duration_t> /*unused*/ ) {
  return true;
}

bool MusicPlayer::is_processing() const { return false; }

void MusicPlayer::pause() {
  auto msg = fmt::format(
      "Music Player `{}` does not support pausing/resuming.",
      info().name );
  DCHECK( capabilities().can_pause, "{}", msg );
  logger->error( msg );
}

void MusicPlayer::resume() {
  auto msg = fmt::format(
      "Music Player `{}` does not support pausing/resuming.",
      info().name );
  DCHECK( capabilities().can_pause, "{}", msg );
  logger->error( msg );
}

void MusicPlayer::set_volume( double /*unused*/ ) {
  auto msg = fmt::format(
      "Music Player `{}` does not support setting volume.",
      info().name );
  DCHECK( capabilities().has_volume, "{}", msg );
  logger->error( msg );
}

void MusicPlayer::seek( double /*unused*/ ) {
  auto msg =
      fmt::format( "Music Player `{}` does not support seeking.",
                   info().name );
  DCHECK( capabilities().can_seek, "{}", msg );
  logger->error( msg );
}

void test_music_player() {
  auto info = SilentMusicPlayer::player();
  CHECK_UNEXPECTED( info.player );
  auto& mplayer = info.player.value().get();
  auto  tune    = random_tune();
  logger->info( "can_play_tune: {}",
                mplayer.can_play_tune( tune ).has_value() );
  logger->info( "play: {}", mplayer.play( tune ) );
  mplayer.state().log();
  mplayer.capabilities().log();
  mplayer.fence();
  logger->info( "is_processing: {}", mplayer.is_processing() );
  mplayer.pause();
  mplayer.resume();
  // Would cause an error.
  // mplayer.seek( .5 );
}

} // namespace rn
