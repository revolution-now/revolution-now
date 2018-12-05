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

// Only include this in this cpp module.
#include "ucl++.h"

#include <string>
#include <unordered_map>

namespace rn {

namespace {

std::unordered_map<std::string, ucl::Ucl> ucl_configs;

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

std::string config_file_for_name( std::string const& name ) {
  return "config/" + name + ".ucl";
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

#define POPULATE_FIELD_IMPL( __type, __ucl_type, __ucl_getter ) \
  void populate_config_field(                                   \
      __type* dest, std::string const& name, int level,         \
      std::string const& config_name,                           \
      ConfigPath const&  parent_path,                           \
      std::string const& file ) {                               \
    auto path = parent_path;                                    \
    path.push_back( name );                                     \
    auto obj = ucl_from_path( config_name, path );              \
    CHECK_( obj.type() != ::UCL_NULL,                           \
            "UCL Config field `" << util::join( path, "." )     \
                                 << "` was not found in file "  \
                                 << file << "." );              \
    CHECK_( obj.type() == __ucl_type,                           \
            "expected `"                                        \
                << config_name << "."                           \
                << util::join( path, "." )                      \
                << "` to be of type " TO_STRING( __type ) );    \
    *dest = obj.__ucl_getter();                                 \
    CONFIG_LOG_STREAM << level << " populate: " << config_name  \
                      << "." << util::join( path, "." )         \
                      << " = " << util::to_string( *dest )      \
                      << "\n";                                  \
  }

// clang-format off
/****************************************************************
* Mapping From C++ Types to UCL Types
*
*                    C++ type      UCL Enum      Ucl::???
*                    -------------------------------------------*/
POPULATE_FIELD_IMPL( int,          UCL_INT,      int_value      )
POPULATE_FIELD_IMPL( bool,         UCL_BOOLEAN,  bool_value     )
POPULATE_FIELD_IMPL( double,       UCL_FLOAT,    number_value   )
POPULATE_FIELD_IMPL( std::string,  UCL_STRING,   string_value   )
/****************************************************************/
// clang-format on
} // namespace rn
