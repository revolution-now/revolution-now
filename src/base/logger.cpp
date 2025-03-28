/****************************************************************
**logger.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-30.
*
* Description: The logger.
*
*****************************************************************/
#include "logger.hpp"

// base
#include "ansi.hpp"
#include "env.hpp"
#include "error.hpp"
#include "fs.hpp"

// C++ standard library
#include <atomic>
#include <mutex>
#include <sstream>
#include <unordered_map>

using namespace std;

namespace base {

/****************************************************************
** Log Level
*****************************************************************/
namespace {

// The log level must only be accessed while holding the
// level_mutex.
atomic<e_log_level> g_level = e_log_level::off;

string const& to_colored_level_name( e_log_level level ) {
  using namespace base::ansi;
  CHECK( level != e_log_level::off );
  static unordered_map<e_log_level, string> const colored{
    { e_log_level::trace,
      fmt::format( "{}TRCE{}", magenta, reset ) },
    { e_log_level::debug,
      fmt::format( "{}DEBG{}", cyan, reset ) },
    { e_log_level::info,
      fmt::format( "{}INFO{}", green, reset ) },
    { e_log_level::warn,
      fmt::format( "{}{}WARN{}", yellow, bold, reset ) },
    { e_log_level::error,
      fmt::format( "{}{}ERRO{}", red, bold, reset ) },
    { e_log_level::critical,
      fmt::format( "{}{}{}CRIT{}", white, on_red, bold,
                   reset ) },
    // Should not be used.
    { e_log_level::off, fmt::format( "OFF" ) },
  };
  return colored.find( level )->second;
}

} // namespace

e_log_level global_log_level() { return g_level; }

void set_global_log_level( e_log_level level ) {
  g_level = level;
}

/****************************************************************
** Console Logger
*****************************************************************/
struct ConsoleLogger final : public ILogger {
  void log( e_log_level target, std::string_view what,
            source_location const& ) override {
    if( target < global_log_level() ) return;
    if( log_fn_.has_value() )
      // Note that the console has its own mutex, so we don't
      // need to guard this.
      ( *log_fn_ )( what );
  }
  maybe<TerminalLoggerFn> log_fn_;
};

namespace {
ConsoleLogger& console_logger_storage() {
  static ConsoleLogger l;
  return l;
}
}

ILogger& console_logger() { return console_logger_storage(); }

void set_console_terminal( maybe<TerminalLoggerFn const&> fn ) {
  // Could be nothing or not.
  console_logger_storage().log_fn_ = fn;
}

/****************************************************************
** Terminal Logger
*****************************************************************/
namespace {

// The console has its own mutex and so does not need one here.
// We only need the mutex for the terminal logger.
mutex& terminal_mutex() {
  static mutex m;
  return m;
}

} // namespace

struct TerminalLogger final : public ILogger {
  void log( e_log_level target, std::string_view what,
            source_location const& loc ) override {
    if( target < global_log_level() ) return;
    fs::path const module_name =
        fs::path( loc.file_name() ).filename();

    auto now = chrono::system_clock::now();
    auto d   = now.time_since_epoch();
    auto millis =
        chrono::duration_cast<chrono::milliseconds>( d );
    auto secs = chrono::duration_cast<chrono::seconds>( d );
    millis -= secs; // isolate milliseconds.

    auto now_c = std::chrono::system_clock::to_time_t( now );
    ostringstream ss;
    ss << put_time( localtime( &now_c ), "%H:%M:%S" );
    ss << fmt::format( ".{:03} {} {}:{}: {}", millis.count(),
                       to_colored_level_name( target ),
                       module_name.string(), loc.line(), what );

    lock_guard<mutex> lock( terminal_mutex() );
    fmt::print( "{}\n", ss.str() );
  }
};

ILogger& terminal_logger() {
  static TerminalLogger l;
  return l;
}

/****************************************************************
** Hybrid Logger
*****************************************************************/
struct HybridLogger final : public ILogger {
  void log( e_log_level target, std::string_view what,
            source_location const& loc ) override {
    // Each logger should check this, but maybe this will save a
    // bit of time.
    if( target < global_log_level() ) return;
    terminal_logger().log( target, what, loc );
    console_logger().log( target, what, loc );
  }
};

ILogger& hybrid_logger() {
  static HybridLogger l;
  return l;
}

/****************************************************************
** Initialization
*****************************************************************/
void init_logger() {
  e_log_level
#ifdef RN_TRACE
      level = level::trace;
#else
      level = e_log_level::debug;
  // DEBUG_RELEASE( e_log_level::debug, e_log_level::info );
#endif
  set_global_log_level( level );
}

void init_logger( e_log_level level ) {
  set_global_log_level( level );
}

/****************************************************************
** Printing Helpers
*****************************************************************/
string fmt_bar( char c, string_view msg ) {
  auto maybe_cols = os_terminal_columns();
  // If we're printing the width of the terminal then don't print
  // a new line since we will automatically move to the next line
  // by exhausting all columns.
  string_view maybe_newline = maybe_cols.has_value() ? "" : "\n";
  string fmt = fmt::format( "{{:{}^{{}}}}{}", c, maybe_newline );
  int const cols = maybe_cols.value_or( 65 );
  // TODO: replace with std::runtime_format when possible.
  return std::vformat( fmt, std::make_format_args( msg, cols ) );
}

void print_bar( char c, string_view msg ) {
  if( global_log_level() > e_log_level::info ) return;
  lock_guard<mutex> lock( terminal_mutex() );
  fmt::print( "{}", fmt_bar( c, msg ) );
}

} // namespace base
