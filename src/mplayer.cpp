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
#include "logger.hpp"
#include "time.hpp"

// C++ standard library
#include <iostream>

using namespace std;

namespace rn {

namespace {}

pair<MusicPlayerDesc, MaybeMusicPlayer>
SilentMusicPlayer::player() {
  static SilentMusicPlayer player;
  return make_pair<MusicPlayerDesc, MaybeMusicPlayer>(
      /*desc=*/{
          /*name=*/"Silent Music Player",
          /*description=*/"For testing; does not play music",
          /*how_it_works=*/"It doesn't.",
      },
      /*player=*/player );
}

bool SilentMusicPlayer::good() const { return true; }

// Implement MusicPlayer
maybe<TunePlayerInfo> SilentMusicPlayer::can_play_tune(
    TuneId id ) {
  return TunePlayerInfo{ /*id=*/id,
                         /*length=*/chrono::minutes( 1 ),
                         /*progress=*/nothing };
}

// Implement MusicPlayer
bool SilentMusicPlayer::play( TuneId id ) {
  lg.debug( "SilentMusicPlayer: playing tune `{}`",
            tune_display_name_from_id( id ) );
  id_ = id;
  return true;
}

// Implement MusicPlayer
void SilentMusicPlayer::stop() { id_ = nothing; }

MusicPlayerDesc SilentMusicPlayer::info() const {
  return SilentMusicPlayer::player().first;
}

// Implement MusicPlayer
MusicPlayerState SilentMusicPlayer::state() const {
  maybe<TunePlayerInfo> maybe_tune_info;
  if( id_.has_value() ) {
    maybe_tune_info =
        TunePlayerInfo{ /*id=*/*id_,
                        /*length=*/chrono::minutes( 1 ),
                        /*progress=*/.5 };
  }
  return { /*tune_info=*/maybe_tune_info,
           /*is_paused=*/is_paused_ };
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
bool SilentMusicPlayer::fence( maybe<Duration_t> /*unused*/ ) {
  return true;
}

// Implement MusicPlayer
bool SilentMusicPlayer::is_processing() const { return false; }

// Implement MusicPlayer
void SilentMusicPlayer::pause() { is_paused_ = true; }

// Implement MusicPlayer
void SilentMusicPlayer::resume() { is_paused_ = false; }

void MusicPlayerState::log() const {
  lg.debug( "MusicPlayerState:" );
  if( tune_info.has_value() ) {
    lg.info( "  tune_info.id:      {} ({})", tune_info->id,
             tune_display_name_from_id( tune_info->id ) );
    if( tune_info->length.has_value() )
      lg.info( "  tune_info.length:  {}sec",
               chrono::duration_cast<chrono::seconds>(
                   tune_info->length.value() )
                   .count() );
    if( tune_info->progress.has_value() )
      lg.info( "  progress:          {}%",
               int( *tune_info->progress * 100 ) );
  }
  lg.info( "  is_paused:         {}", is_paused );
}

void MusicPlayerCapabilities::log() const {
  lg.debug( "MusicPlayerCapabilities:" );
  lg.debug( "  can_pause:         {}", can_pause );
  lg.debug( "  has_volume:        {}", has_volume );
  lg.debug( "  has_progress:      {}", has_progress );
  lg.debug( "  has_tune_duration: {}", has_tune_duration );
  lg.debug( "  can_seek:          {}", can_seek );
}

bool MusicPlayer::fence( maybe<Duration_t> /*unused*/ ) {
  return true;
}

bool MusicPlayer::is_processing() const { return false; }

void MusicPlayer::pause() {
  auto msg = fmt::format(
      "music Player `{}` does not support pausing/resuming.",
      info().name );
  DCHECK( capabilities().can_pause, "{}", msg );
  lg.error( msg );
}

void MusicPlayer::resume() {
  auto msg = fmt::format(
      "music Player `{}` does not support pausing/resuming.",
      info().name );
  DCHECK( capabilities().can_pause, "{}", msg );
  lg.error( msg );
}

void MusicPlayer::set_volume( double /*unused*/ ) {
  auto msg = fmt::format(
      "music Player `{}` does not support setting volume.",
      info().name );
  DCHECK( capabilities().has_volume, "{}", msg );
  lg.error( msg );
}

void MusicPlayer::seek( double /*unused*/ ) {
  auto msg =
      fmt::format( "music Player `{}` does not support seeking.",
                   info().name );
  DCHECK( capabilities().can_seek, "{}", msg );
  lg.error( msg );
}

} // namespace rn
