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

using namespace std;

using namespace spdlog;

namespace rn {

namespace {} // namespace

void init_logging( optional<level::level_enum> level ) {
  if( !level.has_value() ) {
    level = DEBUG_RELEASE( level::debug, level::warn );
  }
  spdlog::set_level( *level );
}

} // namespace rn
