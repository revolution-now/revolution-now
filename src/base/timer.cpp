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

// base
#include "to-str-ext-chrono.hpp"

using namespace std;

namespace base {

namespace {

using ::std::chrono::duration_cast;
using ::std::chrono::nanoseconds;

} // namespace

/****************************************************************
** ScopedTimer
*****************************************************************/
ScopedTimer::ScopedTimer( string                 total_label,
                          source_location const& loc ) {
  if( options_.disable ) return;
  total_.emplace();
  add_segment( *total_, std::move( total_label ), loc );
}

void ScopedTimer::log_segment_result( Segment const& segment,
                                      string_view    prefix ) {
  nanoseconds const d =
      std::max( 0ns, duration_cast<nanoseconds>(
                         segment.end - segment.start ) );
  string const res = fmt::format(
      "{}{}: {}", prefix, segment.label, format_duration( d ) );
  detail::timer_logger_hook( res, segment.source_loc );
}

ScopedTimer::~ScopedTimer() noexcept {
  if( options_.disable ) return;

  // Before we do anything else, we need to nail down the end
  // times so that they are as accurate as possible.
  flush_latest_segment();
  if( total_.has_value() ) total_->end = clock::now();

  // Now we can log.
  if( total_.has_value() ) log_segment_result( *total_ );
  if( options_.no_checkpoints_logging ) return;
  string_view const prefix = total_.has_value() ? "  " : "";
  for( Segment const& segment : segments_ )
    log_segment_result( segment, prefix );
}

void ScopedTimer::checkpoint( string                 label,
                              source_location const& loc ) {
  if( options_.disable ) return;
  flush_latest_segment();
  Segment& new_segment = segments_.emplace_back();
  add_segment( new_segment, std::move( label ), loc );
}

void ScopedTimer::flush_latest_segment() {
  if( segments_.empty() ) return;
  Segment& segment = segments_.back();
  if( segment.end != time_point{} ) return;
  segment.end = clock::now();
}

void ScopedTimer::add_segment( Segment& segment, string label,
                               source_location const& loc ) {
  segment = Segment{
      .label      = std::move( label ),
      .source_loc = loc,
      .start      = clock::now(),
  };
}

} // namespace base
