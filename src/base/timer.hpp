/****************************************************************
**timer.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-23.
*
* Description: For timing things.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "fmt.hpp"
#include "maybe.hpp"

// C++ standard library
#include <chrono>
#include <string>
#include <string_view>
#include <vector>

namespace base {

namespace detail {

// FIXME: temporary until we move logging into the base module.
// This needs to be available at link time somewhere in the bi-
// nary.
void timer_logger_hook( std::string_view            msg,
                        std::source_location const& loc );

}

/****************************************************************
** ScopedTimer
*****************************************************************/
// A ScopedTimer times the execution of a scope (possibly divided
// into multiple "checkpoints") and logs the results on scope
// exit. Examples:
//
//  The following uses a single top-level timed scope:
//
//    {
//      ScopedTimer timer( "some computation" );
//      ...
//    }
//
//  which results in this on scope exit:
//
//    "some computation: 1m2s"
//
//  The following uses multiple checkpoints in addition to a to-
//  tal:
//
//    {
//      ScopedTimer timer( "total" );
//      timer.checkpoint( "one" );
//      ...
//      timer.checkpoint( "two" );
//      ...
//      timer.checkpoint( "three" );
//      ...
//    }
//
//  which results in this on scope exit:
//
//    "total: 1m2s"
//    "  one: 30s"
//    "  two: 10s"
//    "  three: 22s"
//
//  Note that the indentation of one/two/three would be omitted
//  if there were no total specified upon construction of the
//  ScopedTimer.
//
struct ScopedTimer {
  struct Options {
    // When this is true, this class will act as a no-op.
    bool disable = false;
    // When this is true, only the "total" will be logged, if
    // there is one.
    bool no_checkpoints_logging = false;
  };

  using clock      = std::chrono::system_clock;
  using time_point = clock::time_point;

 public:
  ScopedTimer() = default;
  ScopedTimer( std::string                 total_label,
               std::source_location const& loc =
                   std::source_location::current() );
  ~ScopedTimer() noexcept;

  void checkpoint( std::string                 label,
                   std::source_location const& loc =
                       std::source_location::current() );

  // For convenience.
  template<typename Arg, typename... Rest>
  requires( !std::is_same_v<std::source_location,
                            std::remove_cvref_t<Arg>> )
  void checkpoint( FmtStrAndLoc<std::type_identity_t<Arg>,
                                std::type_identity_t<Rest>...>
                         fmt_str_and_loc,
                   Arg&& arg, Rest&&... rest ) {
    return checkpoint(
        fmt::format( fmt_str_and_loc.fs,
                     std::forward<Arg>( arg ),
                     std::forward<Rest>( rest )... ),
        fmt_str_and_loc.loc );
  }

  Options& options() { return options_; }

 private:
  struct Segment {
    std::string          label      = {};
    std::source_location source_loc = {};
    time_point           start      = {};
    time_point           end        = {};
  };

  static void add_segment( Segment& segment, std::string label,
                           std::source_location const& loc );

  void flush_latest_segment();

  static void log_segment_result( Segment const&   segment,
                                  std::string_view prefix = "" );

  Options              options_ = {};
  std::vector<Segment> segments_;
  // Measures the time from the creation of this object to the
  // destruction, but only if a name is provided upon construc-
  // tion.
  maybe<Segment> total_;
};

/****************************************************************
** Convenience methods.
*****************************************************************/
// Will start, run, stop, and return the result of the function.
// Seems to work also for functions that return void.
template<typename FuncT>
auto timer(
    std::string name, FuncT&& func,
    std::source_location loc = std::source_location::current() )
    -> std::invoke_result_t<FuncT> {
  ScopedTimer timer( name, loc );
  return func();
}

// Example usage:
// auto res = TIMER( "my function", f( 1, 2, 3 ) );
#define TIMER( name, code ) \
  ::base::timer( name, [&]() { return code; } );

} // namespace base
