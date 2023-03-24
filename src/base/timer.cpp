/****************************************************************
**timer.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-23.
*
* Description: For timing things.
*
*****************************************************************/
#include "timer.hpp"

using namespace std;

namespace base {

namespace {

using ::std::chrono::duration;
using ::std::chrono::duration_cast;
using ::std::chrono::hours;
using ::std::chrono::microseconds;
using ::std::chrono::milliseconds;
using ::std::chrono::minutes;
using ::std::chrono::nanoseconds;
using ::std::chrono::seconds;

} // namespace

namespace detail {

string format_duration( nanoseconds ns ) {
  using namespace std::literals::chrono_literals;

  auto h  = duration_cast<hours>( ns );
  auto m  = duration_cast<minutes>( ns );
  auto s  = duration_cast<seconds>( ns );
  auto ms = duration_cast<milliseconds>( ns );
  auto us = duration_cast<microseconds>( ns );

  static constexpr int64_t kMinutesInHour =
      hours{ 1 } / minutes{ 1 };
  static constexpr int64_t kSecondsInMinute =
      minutes{ 1 } / seconds{ 1 };
  static constexpr int64_t kMillisInSecond =
      seconds{ 1 } / milliseconds{ 1 };
  static constexpr int64_t kMicrosInMilli =
      milliseconds{ 1 } / microseconds{ 1 };
  static constexpr int64_t kNanosInMicro =
      microseconds{ 1 } / nanoseconds{ 1 };

  static constexpr hours        kSmallEnoughForMinutes{ 20 };
  static constexpr minutes      kSmallEnoughForSeconds{ 20 };
  static constexpr seconds      kSmallEnoughForMillis{ 10 };
  static constexpr milliseconds kSmallEnoughForMicros{ 10 };
  static constexpr microseconds kSmallEnoughForNanos{ 10 };

  // Note that the below, most of the time, should fit into the
  // small string buffer so there shouldn't be any allocations.
  string res;

  auto emit = [&]( auto dur, string_view suffix ) {
    res += fmt::to_string( dur.count() );
    res += suffix;
  };

  if( h > 0ns ) {
    emit( h, "h" );
    if( h < kSmallEnoughForMinutes )
      emit( m % kMinutesInHour, "m" );
  } else if( m > 0ns ) {
    emit( m, "m" );
    if( m < kSmallEnoughForSeconds )
      emit( s % kSecondsInMinute, "s" );
  } else if( s > 0ns ) {
    emit( s, "s" );
    if( s < kSmallEnoughForMillis )
      emit( ms % kMillisInSecond, "ms" );
  } else if( ms > 0ns ) {
    emit( ms, "ms" );
    if( ms < kSmallEnoughForMicros )
      emit( us % kMicrosInMilli, "us" );
  } else if( us > 0ns ) {
    emit( us, "us" );
    if( us < kSmallEnoughForNanos )
      emit( ns % kNanosInMicro, "ns" );
  } else if( ns >= 0ns ) {
    emit( ns, "ns" );
  }

  return res;
}

} // namespace detail

/****************************************************************
** ScopedTimer
*****************************************************************/
ScopedTimer::ScopedTimer( string                 total_label,
                          source_location const& loc ) {
  add_segment( total_, std::move( total_label ), loc );
}

void ScopedTimer::log_segment_result( Segment const& segment,
                                      string_view    prefix ) {
  nanoseconds const d =
      std::max( 0ns, segment.end - segment.start );
  string const res =
      fmt::format( "{}{}: {}", prefix, segment.label,
                   detail::format_duration( d ) );
  detail::timer_logger_hook( res, segment.source_loc );
}

ScopedTimer::~ScopedTimer() noexcept {
  string_view prefix = "";
  if( total_.has_value() ) {
    total_->end = clock::now();
    log_segment_result( *total_ );
    prefix = "  ";
  }
  flush_active_checkpoint();
  for( Segment const& segment : segments_ )
    log_segment_result( segment, prefix );
}

void ScopedTimer::checkpoint( string                 label,
                              source_location const& loc ) {
  add_segment( active_checkpoint_, std::move( label ), loc );
}

void ScopedTimer::flush_active_checkpoint() {
  if( !active_checkpoint_.has_value() ) return;
  segments_.push_back( std::move( *active_checkpoint_ ) );
  segments_.back().end = clock::now();
  active_checkpoint_.reset();
}

void ScopedTimer::add_segment( maybe<Segment>&        where,
                               string                 label,
                               source_location const& loc ) {
  flush_active_checkpoint();
  where = Segment{
      .label      = std::move( label ),
      .source_loc = loc,
      .start      = clock::now(),
  };
}

} // namespace base
