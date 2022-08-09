/****************************************************************
**config-helper.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-04-28.
*
* Description: Helper for registering/loading config data.
*
*****************************************************************/
#include "config-helper.hpp"

// base
#include "base/error.hpp"

// C++ standard library
#include <unordered_map>

using namespace std;

namespace rds {

/****************************************************************
** Globals.
*****************************************************************/
namespace {

PopulatorsMap& g_config_populators() {
  // Needs to be a static variable in a function in order to
  // avoid global initialization ordering issues.
  static PopulatorsMap m;
  return m;
}

}

/****************************************************************
** Private stuff.
*****************************************************************/
namespace detail {

void register_config_erased( string const& name,
                             PopulatorFunc populator ) {
  CHECK( !g_config_populators().contains( name ),
         "there are multiple config objects named `{}'.", name );
  g_config_populators()[name] = std::move( populator );
}

cdr::converter::options const& converter_options() {
  static cdr::converter::options opts{
      .allow_unrecognized_fields        = false,
      .default_construct_missing_fields = false,
  };
  return opts;
}

} // namespace detail

/****************************************************************
** Public API.
*****************************************************************/
PopulatorsMap const& config_populators() {
  return g_config_populators();
}

} // namespace rds
