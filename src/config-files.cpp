/****************************************************************
**config-files.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-11-29.
*
* Description: Handles config file data.
*
*****************************************************************/
#include "config-files.hpp"

// Revolution Now
#include "init.hpp"
#include "logger.hpp"

// Rcl
#include "rcl/model.hpp"
#include "rcl/parse.hpp"

// rds
#include "rds/config-helper.hpp"

// base
#include "base/error.hpp"

// C++ standard library
#include <string>

using namespace std;

namespace rn {

namespace {

bool g_configs_loaded = false;

string config_file_for_name( string const& name ) {
  return "config/rcl/" + name + ".rcl";
}

void init_configs() {
  rds::PopulatorsMap const& populators =
      rds::config_populators();
  for( auto const& [name, populator] : populators ) {
    string file = config_file_for_name( name );
    replace( file.begin(), file.end(), '_', '-' );
    base::expect<rcl::doc> doc = rcl::parse_file( file );
    CHECK( doc, "failed to load {}: {}", file, doc.error() );
    lg.debug( "running config populator for {}.", name );
    CHECK_HAS_VALUE( populator( doc->top_val() ) );
  }
  // Should be last.
  g_configs_loaded = true;
}

void cleanup_configs() {}

REGISTER_INIT_ROUTINE( configs );

} // namespace

} // namespace rn

namespace rn {

bool configs_loaded() { return g_configs_loaded; }

void linker_dont_discard_module_config_files() {}

} // namespace rn
