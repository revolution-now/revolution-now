/****************************************************************
**logger.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-07.
*
* Description: Interface to logger.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// base
#include "base/fmt.hpp"
#include "base/source-loc.hpp"

// C++ standard library
#include <string>
#include <string_view>

namespace rn {

/****************************************************************
** Log Level
*****************************************************************/
enum class e_log_level {
  trace,
  debug,
  info,
  warn,
  error,
  critical,
  off
};

e_log_level global_log_level();
void        set_global_log_level( e_log_level level );

/****************************************************************
** StringAndLoc
*****************************************************************/
// This class is a helper that can be implicitely constructed
// from a string_view, but also captures the source location in
// the process. It is used to automatically collect source loca-
// tion info when logging. We can't use the usual technique of
// making a defaulted SourceLoc parameter at the end of the argu-
// ment list of the logging statements because they already need
// to have a variable number of arguments to support formatting.
struct StringAndLoc {
  template<typename T>
  constexpr StringAndLoc(
      T&&             what_,
      base::SourceLoc loc_ = base::SourceLoc::current() )
    : what( what_ ), loc( loc_ ) {}
  std::string_view const what;
  base::SourceLoc const  loc;
};

/****************************************************************
** Logger Interface
*****************************************************************/
#define ILOGGER_LEVEL( level )                             \
  template<typename... Args>                               \
  void level( StringAndLoc str_and_loc, Args&&... args ) { \
    log( e_log_level::level,                               \
         fmt::format( str_and_loc.what,                    \
                      std::forward<Args>( args )... ),     \
         str_and_loc.loc );                                \
  }

// Subclasses of this must be thread safe with respect to them-
// selves as well as to any other loggers or global state.
struct ILogger {
  ILOGGER_LEVEL( trace );
  ILOGGER_LEVEL( debug );
  ILOGGER_LEVEL( info );
  ILOGGER_LEVEL( warn );
  ILOGGER_LEVEL( error );
  ILOGGER_LEVEL( critical );

  // Should not call this one.
  virtual void log( e_log_level level, std::string_view what,
                    base::SourceLoc const& loc ) = 0;
};

/****************************************************************
** Loggers
*****************************************************************/
// These are the loggers that are available.
ILogger& terminal_logger();
ILogger& console_logger();
ILogger& hybrid_logger();

// The game's default standard logger sends output to both the
// terminal and the in-game console.
inline ILogger& lg = hybrid_logger();

/****************************************************************
** Initialization
*****************************************************************/
// This will initialize the log level.  If a level is not
// specified then one will be chosen according to the build type.
void init_logger();
void init_logger( e_log_level level );

/****************************************************************
** Printing Helpers
*****************************************************************/
// FIXME: move these out of here.
std::string fmt_bar( char c, std::string_view msg = "" );
void        print_bar( char c, std::string_view msg = "" );

} // namespace rn
