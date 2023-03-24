/****************************************************************
**to-str-ext-chrono.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-23.
*
* Description: String formatting for std::chrono stuff.
*
*****************************************************************/
#include "to-str-ext-chrono.hpp"

using namespace std;

namespace base {

/****************************************************************
** Helpers.
*****************************************************************/
string format_duration( chrono::nanoseconds ns ) {
  using namespace std::literals::chrono_literals;

  using ::std::chrono::duration;
  using ::std::chrono::duration_cast;
  using ::std::chrono::hours;
  using ::std::chrono::microseconds;
  using ::std::chrono::milliseconds;
  using ::std::chrono::minutes;
  using ::std::chrono::nanoseconds;
  using ::std::chrono::seconds;

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
    minutes const remainder = m % kMinutesInHour;
    if( h < kSmallEnoughForMinutes && remainder > 0ns )
      emit( remainder, "m" );
  } else if( m > 0ns ) {
    emit( m, "m" );
    seconds const remainder = s % kSecondsInMinute;
    if( m < kSmallEnoughForSeconds && remainder > 0ns )
      emit( remainder, "s" );
  } else if( s > 0ns ) {
    emit( s, "s" );
    milliseconds const remainder = ms % kMillisInSecond;
    if( s < kSmallEnoughForMillis && remainder > 0ns )
      emit( remainder, "ms" );
  } else if( ms > 0ns ) {
    emit( ms, "ms" );
    microseconds const remainder = us % kMicrosInMilli;
    if( ms < kSmallEnoughForMicros && remainder > 0ns )
      emit( remainder, "us" );
  } else if( us > 0ns ) {
    emit( us, "us" );
    nanoseconds const remainder = ns % kNanosInMicro;
    if( us < kSmallEnoughForNanos && remainder > 0ns )
      emit( remainder, "ns" );
  } else if( ns >= 0ns ) {
    emit( ns, "ns" );
  }

  return res;
}

} // namespace base
