/****************************************************************
**logging.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-07.
*
* Description: Interface to logging
*
*****************************************************************/
#include "logging.hpp"

// Revolution Now
#include "ansi.hpp"
#include "console.hpp"
#include "errors.hpp"
#include "fmt-helper.hpp"
#include "macros.hpp"
#include "terminal.hpp"
#include "util.hpp"

// C++ standard library
#include <mutex>

using namespace std;
using namespace spdlog;

namespace rn {

namespace {

// This single mutex guards all in-game console logging actions
// for all loggers in all translation units. This is needed be-
// cause the code in the sink may not be thread safe and, in par-
// ticular, the code in the `console` module is definitely not
// thread safe.
mutex g_dbg_console_impl_mutex;

} // namespace

spdlog::level::level_enum to_spdlog_level( e_log_level level ) {
  switch( level ) {
    case e_log_level::trace: return spdlog::level::trace;
    case e_log_level::debug: return spdlog::level::debug;
    case e_log_level::info: return spdlog::level::info;
    case e_log_level::warn: return spdlog::level::warn;
    case e_log_level::error: return spdlog::level::err;
    case e_log_level::critical: return spdlog::level::critical;
    case e_log_level::off: return spdlog::level::off;
  }
}

// A "sink" that goes to the in-game console in a thread-safe
// way.
class debug_console_sink final : public spdlog::sinks::sink {
public:
  debug_console_sink()           = default;
  ~debug_console_sink() override = default;

  debug_console_sink( debug_console_sink const& ) = delete;
  debug_console_sink& operator                    =(
      debug_console_sink const& other ) = delete;

  void log( spdlog::details::log_msg const& msg ) override {
    lock_guard<mutex> lock( g_dbg_console_impl_mutex );
    // FIXME: reconfigure formatting to add module name back in
    //        (but not timestamp) and then renable this code.
    // fmt::memory_buffer formatted;
    // formatter_->format( msg, formatted );
    // string res( formatted.data(), formatted.size() );
    string res( msg.payload.data(), msg.payload.size() );
    term::log( std::move( res ) );
  }

  void flush() final {
    lock_guard<mutex> lock( g_dbg_console_impl_mutex );
    // ...
  }

  void set_pattern( const string& pattern ) final {
    lock_guard<mutex> lock( g_dbg_console_impl_mutex );
    formatter_ = unique_ptr<spdlog::formatter>(
        new spdlog::pattern_formatter( pattern ) );
  }

  void set_formatter(
      unique_ptr<spdlog::formatter> sink_formatter ) override {
    lock_guard<mutex> lock( g_dbg_console_impl_mutex );
    formatter_ = std::move( sink_formatter );
  }
};

namespace {

// %n = module name
// %^ = start color (color used is determined by the sink)
// %l = log level (defined in logging.hpp, our header file)
// %$ = end color
// %v = message
string const pattern() { return "%M:%S.%e %^%l%$ %v"; }

spdlog::sink_ptr default_dbg_console_sink() {
  static spdlog::sink_ptr p = [] {
    auto p = make_shared<debug_console_sink>();
    p->set_pattern( pattern() );
    return p;
  }();
  return p;
}

spdlog::sink_ptr default_terminal_sink() {
  static spdlog::sink_ptr p = [] {
    auto p =
        make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
    p->set_pattern( pattern() );
    return p;
  }();
  return p;
}

} // namespace

void init_logging( optional<level::level_enum> level ) {
  if( !level.has_value() ) {
#ifdef RN_TRACE
    level = level::trace;
#else
    level = DEBUG_RELEASE( level::debug, level::info );
#endif
  }
  spdlog::set_level( *level );
}

namespace detail {

// In the following functions we create loggers with a new in-
// stance of their respective sinks, then we replace that sync
// with our global instance of the sink. We want a global in-
// stance because we want all loggers of a particular type to use
// the same sink object so that thread safety can be maintained.
// However, there doesn't appear to be a spdlog api for creating
// a logger by specifying a sink object (only sink type).

shared_ptr<spdlog::logger> create_dbg_console_logger(
    string const& logger_name ) {
  auto lager = spdlog::default_factory::template create<
      debug_console_sink>(
      fmt::format( "{: <13}", "~" + logger_name + "~" ) );
  CHECK( lager->sinks().size() == 1 );
  // Replace sink with global.
  lager->sinks()[0] = default_dbg_console_sink();
  return lager;
}

shared_ptr<spdlog::logger> create_terminal_logger(
    string const& logger_name ) {
  auto lager = spdlog::stdout_color_mt(
      fmt::format( "{: <13}", "." + logger_name + "." ) );
  CHECK( lager->sinks().size() == 1 );
  // Replace sink with global.
  lager->sinks()[0] = default_terminal_sink();
  return lager;
}

shared_ptr<spdlog::logger> create_hybrid_logger(
    string const& logger_name, shared_ptr<spdlog::logger> trm,
    shared_ptr<spdlog::logger> dbg ) {
  auto lgr = spdlog::stdout_color_mt(
      fmt::format( "{: <13}", logger_name ) );

  // NOTE: if these checks fail the may cause a "core dump" be-
  // cause this code runs at global variable initialization time.

  CHECK( dbg->sinks().size() == 1 );
  CHECK( trm->sinks().size() == 1 );
  CHECK( lgr->sinks().size() == 1 );

  // For this logger we want two sinks: first, get rid of the new
  // terminal sink and replace it with the existing one from this
  // module (for the sake of tidiness) and then add in the
  // in-game console sink.
  lgr->sinks()[0] = trm->sinks()[0];
  lgr->sinks().push_back( dbg->sinks()[0] );

  CHECK( dbg->sinks().size() == 1 );
  CHECK( trm->sinks().size() == 1 );
  CHECK( lgr->sinks().size() == 2 );

  return lgr;
}

} // namespace detail

string fmt_bar( char c, string_view msg ) {
  auto maybe_cols = os_terminal_columns();
  // If we're printing the width of the terminal then don't print
  // a new line since we will automatically move to the next line
  // by exhausting all columns.
  string_view maybe_newline = maybe_cols.has_value() ? "" : "\n";
  string fmt = fmt::format( "{{:{}^{{}}}}{}", c, maybe_newline );
  return fmt::format( fmt, msg, maybe_cols.value_or( 65 ) );
}

void print_bar( char c, string_view msg ) {
  fmt::print( "{}", fmt_bar( c, msg ) );
}

} // namespace rn
