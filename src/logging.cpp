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
#include "console.hpp"
#include "fmt-helper.hpp"

using namespace std;

using namespace spdlog;

namespace rn {

namespace {} // namespace

void init_logging( optional<level::level_enum> level ) {
  if( !level.has_value() ) {
#ifdef RN_TRACE
    level = level::trace;
#else
    level = DEBUG_RELEASE( level::debug, level::warn );
#endif
  }
  spdlog::set_level( *level );
}

void debug_console_sink_log_impl( std::string const& msg ) {
  log_to_debug_console( msg );
}

void debug_console_sink_log_impl( std::string&& msg ) {
  log_to_debug_console( msg );
}

} // namespace rn
