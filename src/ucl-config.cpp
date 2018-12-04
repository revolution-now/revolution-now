/****************************************************************
**ucl-config.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-11-29.
*
* Description: Handles config file data.
*
*****************************************************************/
#include "ucl-config.hpp"

#include "macros.hpp"

#include <string>

namespace rn {

namespace {} // namespace

ConfigLoadPairs& config_files() {
  static ConfigLoadPairs pairs;
  return pairs;
}

std::vector<RankedFunction>& config_registration_functions() {
  static std::vector<RankedFunction>
      config_registration_functions;
  return config_registration_functions;
}

std::vector<std::function<void( void )>>&
load_registration_functions() {
  static std::vector<std::function<void( void )>>
      load_registration_functions;
  return load_registration_functions;
}

void load_configs() {
  for( auto const& f : load_registration_functions() ) f();
  for( auto [ucl_obj, file] : config_files() ) {
    // std::cout << "Loading file " << file << "\n";
    std::string errors;
    ucl_obj.get() = ucl::Ucl::parse_from_file( file, errors );
    ucl::Ucl& o   = ucl_obj.get();
    ASSERT( o, "failed to load " << file << ": " << errors );
  }
  sort( config_registration_functions().begin(),
        config_registration_functions().end(),
        []( RankedFunction const& left,
            RankedFunction const& right ) {
          return left.first < right.first;
        } );
  for( auto const& p : config_registration_functions() )
    p.second();
}

ucl::Ucl ucl_from_path( ucl::Ucl const&   ucl_config,
                        ConfigPath const& components ) {
  ucl::Ucl obj = ucl_config;
  for( auto const& s : components ) {
    obj = obj[s];
    if( !obj ) break;
  }
  return obj; // return it whether true or false
}

} // namespace rn
