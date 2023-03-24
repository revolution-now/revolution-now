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

namespace {}        // namespace

namespace detail {} // namespace detail

/****************************************************************
** ScopedTimer
*****************************************************************/
ScopedTimer::ScopedTimer( string                 total_label,
                          source_location const& loc ) {
  add_segment( total_, std::move( total_label ), loc );
}

void ScopedTimer::log_segment_result( Segment const& segment,
                                      string_view    prefix ) {
  chrono::nanoseconds const d =
      std::max( 0ns, segment.end - segment.start );
  string const res = fmt::format(
      "{}{}: {}", prefix, segment.label, format_duration( d ) );
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
