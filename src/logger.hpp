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
** FmtStrAndLoc
*****************************************************************/
// This class is a helper that is implicitly constructed from a
// constexpr string, but also captures the source location in the
// process. It is used to automatically collect source location
// info when logging. We can't use the usual technique of making
// a defaulted SourceLoc parameter at the end of the argument
// list of the logging statements because they already need to
// have a variable number of arguments to support formatting.
// Note that we also store the format string in a format_string
// so that we get fmt's compile time format checking, which we
// would otherwise lose. We really want the compile-time format
// string checking afforded to us by fmt::format_string because
// otherwise format string syntax issues (or argument count dis-
// crepencies) would not be caught until runtime at which point
// fmt throws an exception which immediately terminates our pro-
// gram without much info for debugging where it happened. So
// even though we could bypass the compile time checking by using
// fmt::runtime(...) and perhaps save some compile time, we opt
// to keep the checking.
template<typename... Args>
struct FmtStrAndLoc {
  consteval FmtStrAndLoc(
      char const*     s,
      base::SourceLoc loc = base::SourceLoc::current() )
    : fs( s ), loc( loc ) {}
  fmt::format_string<Args...> fs;
  base::SourceLoc             loc;
};

/****************************************************************
** Logger Interface
*****************************************************************/
// The use of std::type_identity_t trick was taken from fmt's de-
// finition of fmt::format_string itself. It is needed otherwise
// the compiler will fail to match the first parameter... I don't
// understand it 100%, but I believe what is happening is that
// wrapping the Args in a std::type_identity_t (which is just a
// generic type identity function; no magic) in a function argu-
// ment will leave the types unchanged but will obstruct the com-
// piler from trying to infer Args from the type of the first ar-
// gument passed in (format string) argument, which it would fail
// to do, since that arg is just a string literal. Instead, it is
// now forced to infer Args from the subsequent parameters, which
// then fixes them for the first parameter. See the type_identity
// cppreference page for more info.
#define ILOGGER_LEVEL( level )                            \
  template<typename... Args>                              \
  void level( FmtStrAndLoc<std::type_identity_t<Args>...> \
                  fmt_str_and_loc,                        \
              Args&&... args ) {                          \
    log( e_log_level::level,                              \
         fmt::format( fmt_str_and_loc.fs,                 \
                      std::forward<Args>( args )... ),    \
         fmt_str_and_loc.loc );                           \
  }

// Subclasses of this must be thread safe with respect to them-
// selves as well as to any other loggers or global state.
struct ILogger {
  virtual ~ILogger() = default;

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

struct Terminal;
// This must be called when the Terminal object is created and
// destroyed (in the latter case, set it to nullptr).
void set_console_terminal( Terminal* terminal );

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
