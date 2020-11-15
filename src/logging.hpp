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

// base
#include "base/fs.hpp"

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
//   CRITICAL            lg.critical(...)
//   ERROR               lg.error(...)
//   WARN                lg.warn(...)
//   INFO                lg.info(...)
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

// Customization point.
#define SPDLOG_LEVEL_NAMES \
  { "TRCE", "DEBG", "INFO", "WARN", "ERRO", "CRIT", "OFF" }

// FIXME: try to get spdlog out of here so that we don't have to
// compile it in every translation unit.
// clang-format will reorder these headers which then generates
// an error because they need to be included in a certian order.
// clang-format off
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
// clang-format on

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
      lg.level( "{} changed: {}", #var, var );           \
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

// Logs the variable "v: <value of v>".
#define LOG_VAR( level, v ) lg.level( #v ": {}", v )

namespace rn {

spdlog::level::level_enum to_spdlog_level( e_log_level level );

namespace detail {
// Functions in this namespace should not be called externally.
std::shared_ptr<spdlog::logger> create_dbg_console_logger(
    std::string const& logger_name );

std::shared_ptr<spdlog::logger> create_terminal_logger(
    std::string const& logger_name );

std::shared_ptr<spdlog::logger> create_hybrid_logger(
    std::string const&              logger_name,
    std::shared_ptr<spdlog::logger> trm,
    std::shared_ptr<spdlog::logger> dbg );
} // namespace detail

/****************************************************************
** Loggers
*
* The following are global logger objects that are available for
* logging. The logger objects are 1) in an anonymous namespace,
* and 2) are defined in this header file. The reason for this is
* that each translation unit gets its own logger object (since
* they will have formatting strings that are module-specific in
* that they include the module name).
*
* The name of the loggers for a given module will be the stem of
* the filename of the cpp file. Note that using __FILE__ would
* yield the name of this header file which would not be useful.
*
* Each of these loggers is itself threadsafe meaning that it
* should be safe for multiple threads to use the same logger at
* the same time. It should also be fine for two threads to use
* two difference loggers at the same time. It should also be fine
* for two threads to use two different loggers of the same type
* (e.g. from different modules). That last guarantee comes from
* the fact that all loggers of a given type (across all modules)
* will use the same underlying sink object (this is implemented
* by us, not spdlog) and that sink object will use mutexes.
*
* The two basic loggers are the terminal logger and the in-game
* console logger. The terminal logger goes to stdout (terminal)
* while the in-game console logger goes to an in-memory log that
* is visible by selecting the menu Debug --> Enable Console.
*
* The default game logger is one that feeds into both of the
* above loggers so that a single typical logging statement will
* go to both the terminal and the in-game console.
*****************************************************************/
namespace {

auto __rn_module_name =
    fs::path( __BASE_FILE__ ).filename().stem().string();

// Use pointers to std::shared_ptr in the below (instead of just
// std::shared_ptr) to avoid global variables with non-trivial
// destructors and associated issues with ASan.

// Create one debug console logger for each translation unit.
// This one will send logging to the in-game console only.
auto* dbg_console_logger = new std::shared_ptr<spdlog::logger>(
    detail::create_dbg_console_logger( __rn_module_name ) );

// Create one stdout color terminal logger for each translation
// unit.
auto* terminal_logger = new std::shared_ptr<spdlog::logger>(
    detail::create_terminal_logger( __rn_module_name ) );

// Create one hybrid logger for each translation unit. This one
// will send its logging to the sinks of both the terminal logger
// and dbg console logger so that one logging statement can log
// to both of those at once, which is the most common use case.
auto* hybrid_logger = new std::shared_ptr<spdlog::logger>(
    detail::create_hybrid_logger( __rn_module_name,
                                  *terminal_logger,
                                  *dbg_console_logger ) );

// The game's default standard logger sends output to both the
// terminal and the in-game console.
auto& lg = **hybrid_logger;

} // namespace

// This will initialize the log level.  If a level is not
// specified then one will be chosen according to the build type.
void init_logging(
    std::optional<spdlog::level::level_enum> level );

} // namespace rn
