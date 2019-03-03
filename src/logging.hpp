/****************************************************************
**logging.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-07.
*
* Description: Interface to logging
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// C++ standard library
#include <type_traits>

#ifdef SPDLOG_ACTIVE_LEVEL
#  error "SPDLOG_ACTIVE_LEVEL should not be defined!"
#endif

// To log at INFO level and below (meaning INFO, WARN, ERROR, and
// CRITICAL) use the non-macro statements so that we can control
// those levels at runtime (e.g. in a release build).  For DEBUG
// and TRACE debugging use the macro forms so that they can be
// fully removed by the compiler when not needed:
//
//   Level               Command
//   ------------------------------------------------------------
//   CRITICAL            logger->critical(...)
//   ERROR               logger->error(...)
//   WARN                logger->warn(...)
//   INFO                logger->info(...)
//   DEBUG               LOG_DEBUG( ... )
//   TRACE               LOG_TRACE( ... )
//
// Compile-time log levels are controlled by defining the
// SPDLOG_ACTIVE_LEVEL to one of these (before including
// spdlog.h):
//
//   SPDLOG_LEVEL_TRACE,
//   SPDLOG_LEVEL_DEBUG,
//   SPDLOG_LEVEL_INFO,
//   SPDLOG_LEVEL_WARN,
//   SPDLOG_LEVEL_ERROR,
//   SPDLOG_LEVEL_CRITICAL,
//   SPDLOG_LEVEL_OFF
//
// When a particular level is chosen all logging for the levels
// above it will be disabled.  If using macros to log then the
// disabled logging statements will disappear completely.  If
// using non-macro logging statements then the disabled logging
// statements will be no-opts (though might not disappear
// completely, especially in debug builds).  However, note that
// when a given level is disabled then it will be disabled
// whether using macro or not.  The macro just allows for
// compile- time removal if it is disabled.
//
// In this code base we do not allow the build system to choose
// the log level explicitly; instead, we select it based on some
// other preprocessor variables.

// Decide how to set SPDLOG_ACTIVE_LEVEL.
#ifdef RN_TRACE
// If the build system has requested a trace level then abide.
// This will enable ALL logging statements.
#  define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#else
#  ifdef NDEBUG // Release build
#    define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#  else // Debug build
#    define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#  endif
#endif

// clang-format will reorder these headers which then generates
// an error because they need to be included in a certian order.
// clang-format off
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
// clang-format on

// c++ standard library
#include <experimental/filesystem>

// Enabled if the log level is high enough
#define LOG_DEBUG( ... ) \
  SPDLOG_LOGGER_DEBUG( logger, __VA_ARGS__ )
#define LOG_TRACE( ... ) \
  SPDLOG_LOGGER_TRACE( logger, __VA_ARGS__ )

// Will log a variable whenever it changes. This involves
// copying, so should probably only be used while debugging.
#define LOG_WHEN_CHANGE( level, var )                    \
  {                                                      \
    static std::remove_cv_t<decltype( var )> __to_log{}; \
    if( __to_log != var ) {                              \
      logger->level( "{} changed: {}", #var, var );      \
      __to_log = var;                                    \
    }                                                    \
  }

// Will call a callable each time the variable changes. This
// involves copying, so should probably only be used while
// debugging.
#define WHEN_CHANGE_DO( var, func )                      \
  {                                                      \
    static std::remove_cv_t<decltype( var )> __cached{}; \
    if( __cached != var ) {                              \
      func();                                            \
      __cached = var;                                    \
    }                                                    \
  }

namespace rn {

void debug_console_sink_log_impl( std::string const& msg );
void debug_console_sink_log_impl( std::string&& msg );

// This is only needed if we to take advantage of spdlog's
// formatting of log messages, such as adding in the timestamp or
// module name.
class debug_console_sink final : public spdlog::sinks::sink {
public:
  debug_console_sink()           = default;
  ~debug_console_sink() override = default;

  debug_console_sink( debug_console_sink const& ) = delete;
  debug_console_sink& operator                    =(
      debug_console_sink const& other ) = delete;

  void log( spdlog::details::log_msg const& msg ) override {
    // FIXME: reconfigure formatting to add module name back in
    //        (but not timestamp) and then renable this code.
    // fmt::memory_buffer formatted;
    // formatter_->format( msg, formatted );
    // std::string res( formatted.data(), formatted.size() );
    std::string res( msg.payload.data(), msg.payload.size() );
    debug_console_sink_log_impl( std::move( res ) );
  }

  void flush() final {}

  void set_pattern( const std::string& pattern ) final {
    formatter_ = std::unique_ptr<spdlog::formatter>(
        new spdlog::pattern_formatter( pattern ) );
  }

  void set_formatter( std::unique_ptr<spdlog::formatter>
                          sink_formatter ) override {
    formatter_ = std::move( sink_formatter );
  }
};

inline std::shared_ptr<spdlog::logger> create_dbg_console(
    std::string const& logger_name ) {
  return spdlog::default_factory::template create<
      debug_console_sink>( logger_name );
}

namespace {

// This will create one debug console logger for each translation
// unit. The name of the logger will be the stem of the filename
// of the cpp file. Using __FILE__ would yield the name of this
// header file which would not be useful.
auto dbg_console = rn::create_dbg_console( fmt::format(
    "{: ^16}",
    "~" + fs::path( __BASE_FILE__ ).filename().stem().string() +
        "~" ) );

} // namespace

// This is a wrapper around spdlog loggers.
class Logger {
public:
  Logger( std::shared_ptr<spdlog::logger>&& logger )
    : logger_( logger ) {}
  template<typename... Args>
  void info( Args... args ) {
    logger_->info( args... );
    dbg_console->info( args... );
  }
  template<typename... Args>
  void debug( Args... args ) {
    logger_->debug( args... );
    dbg_console->debug( args... );
  }
  template<typename... Args>
  void trace( Args... args ) {
    logger_->trace( args... );
    dbg_console->trace( args... );
  }
  template<typename... Args>
  void warn( Args... args ) {
    logger_->warn( args... );
    dbg_console->warn( args... );
  }
  template<typename... Args>
  void error( Args... args ) {
    logger_->error( args... );
    dbg_console->error( args... );
  }
  template<typename... Args>
  void critical( Args... args ) {
    logger_->critical( args... );
    dbg_console->critical( args... );
  }

private:
  std::shared_ptr<spdlog::logger> logger_;
};

} // namespace rn

namespace {

// This will create one logger for each translation unit.  The
// name of the logger will be the stem of the filename of the
// cpp file.  Using __FILE__ would yield the name of this header
// file which would not be useful.
auto logger = new rn::Logger( spdlog::stdout_color_mt(
    fmt::format( "{: ^16}", fs::path( __BASE_FILE__ )
                                .filename()
                                .stem()
                                .string() ) ) );

} // namespace

namespace rn {

// This will initialize the log level.  If a level is not
// specified then one will be chosen according to the build type.
void init_logging(
    std::optional<spdlog::level::level_enum> level );

} // namespace rn
