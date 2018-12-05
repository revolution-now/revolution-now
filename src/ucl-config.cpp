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

#include "errors.hpp"

#include <string>
#include <unordered_map>

namespace rn {

namespace {

std::unordered_map<std::string, ucl::Ucl> ucl_configs;

} // namespace

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
  for( auto [ucl_name, file] : config_files() ) {
    // std::cout << "Loading file " << file << "\n";
    auto&       ucl_obj = ucl_configs[ucl_name];
    std::string errors;
    ucl_obj = ucl::Ucl::parse_from_file( file, errors );
    CHECK_( ucl_obj,
            "failed to load " << file << ": " << errors );
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

ucl::Ucl ucl_from_path( std::string const& name,
                        ConfigPath const&  components ) {
  // This must be by value since we reassign it.
  ucl::Ucl obj = ucl_configs[name];
  for( auto const& s : components ) {
    obj = obj[s];
    if( !obj ) break;
  }
  return obj; // return it whether true or false
}

} // namespace rn
