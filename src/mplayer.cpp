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
#include "time.hpp"

using namespace std;

namespace rn {

namespace {}

pair<MusicPlayerDesc, MaybeMusicPlayer>
SilentMusicPlayer::player() {
  static SilentMusicPlayer player;
  return {
      /*desc=*/{
          /*name=*/"Silent Music Player",
          /*description=*/"For testing; does not play music",
          /*how_it_works=*/"It doesn't.",
      },
      /*player=*/&player,
  };
}

bool SilentMusicPlayer::good() const { return true; }

// Implement MusicPlayer
Opt<TunePlayerInfo> SilentMusicPlayer::can_play_tune(
    TuneId id ) {
  return TunePlayerInfo{/*id=*/id,
                        /*length=*/chrono::minutes( 1 ),
                        /*progress=*/nullopt};
}

// Implement MusicPlayer
bool SilentMusicPlayer::play( TuneId id ) {
  logger->info( "SilentMusicPlayer: playing tune `{}`",
                tune_display_name_from_id( id ) );
  id_ = id;
  return true;
}

// Implement MusicPlayer
void SilentMusicPlayer::stop() { id_ = nullopt; }

MusicPlayerDesc SilentMusicPlayer::info() const {
  return SilentMusicPlayer::player().first;
}

// Implement MusicPlayer
MusicPlayerState SilentMusicPlayer::state() const {
  Opt<TunePlayerInfo> maybe_tune_info;
  if( id_.has_value() ) {
    maybe_tune_info =
        TunePlayerInfo{/*id=*/*id_,
                       /*length=*/chrono::minutes( 1 ),
                       /*progress=*/.5};
  }
  return {/*tune_info=*/maybe_tune_info,
          /*is_paused=*/is_paused_};
}

// Implement MusicPlayer
MusicPlayerCapabilities SilentMusicPlayer::capabilities() const {
  return {
      /*can_pause=*/true,
      /*has_volume=*/false,
      /*has_progress=*/true,
      /*has_tune_duration=*/true,
      /*can_seek=*/false,
  };
}

// Implement MusicPlayer
bool SilentMusicPlayer::fence( Opt<Duration_t> /*unused*/ ) {
  return true;
}

// Implement MusicPlayer
bool SilentMusicPlayer::is_processing() const { return false; }

// Implement MusicPlayer
void SilentMusicPlayer::pause() { is_paused_ = true; }

// Implement MusicPlayer
void SilentMusicPlayer::resume() { is_paused_ = false; }

void MusicPlayerState::log() const {
  logger->info( "MusicPlayerState:" );
  if( tune_info.has_value() ) {
    logger->info( "  tune_info.id:      {} ({})",
                  tune_info->id._,
                  tune_display_name_from_id( tune_info->id ) );
    if( tune_info->length.has_value() )
      logger->info( "  tune_info.length:  {}sec",
                    chrono::duration_cast<chrono::seconds>(
                        tune_info->length.value() )
                        .count() );
    if( tune_info->progress.has_value() )
      logger->info( "  progress:          {}%",
                    int( *tune_info->progress * 100 ) );
  }
  logger->info( "  is_paused:         {}", is_paused );
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

/****************************************************************
**Testing
*****************************************************************/
void test_music_player_impl( MusicPlayer& mplayer ) {
  if( !mplayer.good() ) {
    logger->error( "Music Player {} has failed.",
                   mplayer.info().name );
    return;
  }

  auto capabilities = mplayer.capabilities();

  logger->info( "Testing `{}`", mplayer.info().name );
  capabilities.log();

  double vol = 1.0;
  if( capabilities.has_volume ) mplayer.set_volume( vol );

  while( true ) {
    // Wait for music player to consume commands.
    mplayer.fence( /*timeout=*/1s );
    mplayer.state().log();
    logger->info(
        "[p]lay next, p[a]use, [r]esume, [s]top, [u]p volume, "
        "[d]own volume, s[t]ate, [q]uit: " );
    string in;
    cin >> in;
    sleep( chrono::milliseconds( 20 ) );
    if( in == "q" ) break;
    if( !mplayer.good() ) {
      logger->warn(
          "Music Player has failed and is no longer active." );
      continue;
    }
    if( in == "p" ) {
      auto tune = random_tune();
      logger->info( "play result: {}", mplayer.play( tune ) );
      continue;
    }
    if( in == "a" ) {
      mplayer.pause();
      continue;
    }
    if( in == "r" ) {
      mplayer.resume();
      continue;
    }
    if( in == "s" ) {
      mplayer.stop();
      continue;
    }
    if( in == "u" ) {
      if( capabilities.has_volume ) {
        vol += .1;
        vol = std::clamp( vol, 0.0, 1.0 );
        mplayer.set_volume( vol );
      }
      logger->info( "Volume: {}", vol );
      continue;
    }
    if( in == "d" ) {
      if( capabilities.has_volume ) {
        vol -= .1;
        vol = std::clamp( vol, 0.0, 1.0 );
        mplayer.set_volume( vol );
      }
      logger->info( "Volume: {}", vol );
      continue;
    }
    if( in == "t" ) { continue; }
  }
}

} // namespace rn
