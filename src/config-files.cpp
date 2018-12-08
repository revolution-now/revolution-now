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
#include "errors.hpp"
#include "logging.hpp"
#include "util.hpp"

// base-util
#include "base-util/misc.hpp"
#include "base-util/string.hpp"

// libucl: only include this in this cpp module.
#include "ucl++.h"

// Abseil
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

// c++ standard library
#include <string>

using namespace std;

#undef CFG
#undef OBJ
#undef FLD
#undef FLD_OPT
#undef LIST

#define FLD( __type, __name )                                \
  static void __populate_##__name() {                        \
    populate_config_field( &( dest_ptr()->__name ),          \
                           TO_STRING( __name ), this_name(), \
                           this_path(), this_file() );       \
  }                                                          \
  static inline bool const __register_##__name = [] {        \
    populate_functions().push_back( __populate_##__name );   \
    return true;                                             \
  }();

#define FLD_OPT( type, name ) FLD( std::optional<type>, name )
#define LIST( type, name ) FLD( std::vector<type>, name )

#define OBJ( __name, __body )                                    \
  static auto*      __name##_parent_ptr() { return dest_ptr(); } \
  static ConfigPath __name##_parent_path() {                     \
    return this_path();                                          \
  }                                                              \
  struct __name##_t;                                             \
  __name##_t* __##__name;                                        \
                                                                 \
  struct __name##_t {                                            \
    static auto* dest_ptr() {                                    \
      return &( __name##_parent_ptr()->__name );                 \
    }                                                            \
    static ConfigPath this_path() {                              \
      auto path = __name##_parent_path();                        \
      path.push_back( TO_STRING( __name ) );                     \
      return path;                                               \
    }                                                            \
                                                                 \
    __body                                                       \
  };

#define CFG( __name, __body )                                     \
  config_##__name##_t const config_##__name{};                    \
                                                                  \
  struct shadow_config_##__name##_t {                             \
    static config_##__name##_t* dest_ptr() {                      \
      return const_cast<config_##__name##_t*>(                    \
          &( config_##__name ) );                                 \
    }                                                             \
    static ConfigPath this_path() { return {}; }                  \
    static string     this_name() { return TO_STRING( __name ); } \
    static string     this_file() {                               \
      return config_file_for_name( this_name() );             \
    }                                                             \
                                                                  \
    __body                                                        \
  };                                                              \
                                                                  \
  struct __startup_##__name {                                     \
    static void __load_##__name() {                               \
      config_files().push_back(                                   \
          {shadow_config_##__name##_t::this_name(),               \
           shadow_config_##__name##_t::this_file()} );            \
    }                                                             \
    static inline int const __register_##__name = [] {            \
      load_functions().push_back( __load_##__name );              \
      return 0;                                                   \
    }();                                                          \
  };

#define UCL_TYPE( input, ucl_enum, ucl_getter )               \
  template<>                                                  \
  struct ucl_type_of_t<input> {                               \
    static constexpr UclType_t value = ucl_enum;              \
  };                                                          \
  template<>                                                  \
  struct ucl_getter_for_type_t<input> {                       \
    using getter_t = decltype( &ucl::Ucl::ucl_getter );       \
    static constexpr getter_t getter = &ucl::Ucl::ucl_getter; \
  };

namespace rn {

namespace {

// List of field paths from the config that were found in the
// schema. This is used to warn the user of config variables
// on the config file but not in the schema.
absl::flat_hash_set<string> used_field_paths;

absl::flat_hash_map<string, ucl::Ucl> ucl_configs;

using ConfigPath = vector<string>;

ucl::Ucl ucl_from_path( string const&     name,
                        ConfigPath const& components ) {
  // This must be by value since we reassign it.
  ucl::Ucl obj = ucl_configs[name];
  for( auto const& s : components ) {
    obj = obj[s];
    if( !obj ) break;
  }
  return obj; // return it whether true or false
}

vector<pair<string, string>>& config_files() {
  static vector<pair<string, string>> pairs;
  return pairs;
}

using PopulateFunction = function<void( void )>;

vector<PopulateFunction>& populate_functions() {
  static vector<PopulateFunction> populate_functions;
  return populate_functions;
}

vector<function<void( void )>>& load_functions() {
  static vector<function<void( void )>> load_functions;
  return load_functions;
}

string config_file_for_name( string const& name ) {
  return "config/" + name + ".ucl";
}

// The type of UCL's enum representing types.
using UclType_t = decltype( ::UCL_INT );

template<typename T>
struct ucl_getter_for_type_t;

template<typename T>
struct ucl_type_of_t;

template<typename T>
auto ucl_getter_for_type = ucl_getter_for_type_t<T>::getter;

template<typename T>
UclType_t ucl_type_of_v = ucl_type_of_t<T>::value;

template<typename T>
void populate_config_field( T* dest, string const& name,
                            string const&     config_name,
                            ConfigPath const& parent_path,
                            string const&     file );

template<typename T>
void populate_config_field( optional<T>*      dest,
                            string const&     name,
                            string const&     config_name,
                            ConfigPath const& parent_path,
                            string const&     file ) {
  auto path = parent_path;
  path.push_back( name );
  auto obj = ucl_from_path( config_name, path );
  (void)file;
  if( obj.type() != ::UCL_NULL ) {
    CHECK_( obj.type() == ucl_type_of_v<T>,
            "expected non-null `"
                << config_name << "." << util::join( path, "." )
                << "` to be of type " TO_STRING( __type ) );
    *dest = (obj.*ucl_getter_for_type<T>)( {} );
  } else {
    // This could happen either if the field is absent or if it
    // is set to `null` without quotes.
    *dest = nullopt;
  }
  used_field_paths.insert( file + "." +
                           util::join( path, "." ) );
}

template<typename T>
void populate_config_field( vector<T>* dest, string const& name,
                            string const&     config_name,
                            ConfigPath const& parent_path,
                            string const&     file ) {
  auto path = parent_path;
  path.push_back( name );
  auto obj = ucl_from_path( config_name, path );
  CHECK_( obj.type() != ::UCL_NULL,
          "UCL Config field `" << util::join( path, "." )
                               << "` was not found in file "
                               << file << "." );
  CHECK_( obj.type() == ::UCL_ARRAY,
          "expected `" << config_name << "."
                       << util::join( path, "." )
                       << "` to be of type vector<" TO_STRING(
                              __type ) ">" );
  CHECK( dest->empty() );
  dest->reserve( obj.size() ); /* array size */
  for( auto elem : obj ) {
    CHECK_( elem.type() == ucl_type_of_v<T>,
            "expected elements in array `"
                << config_name << "." << util::join( path, "." )
                << "` to be of type " TO_STRING( __ucl_type ) );
    dest->push_back(
        static_cast<T>( (elem.*ucl_getter_for_type<T>)( {} ) ) );
  }
  CHECK( dest->size() == obj.size() );
  used_field_paths.insert( file + "." +
                           util::join( path, "." ) );
}

template<typename T>
void populate_config_field( T* dest, string const& name,
                            string const&     config_name,
                            ConfigPath const& parent_path,
                            string const&     file ) {
  LOG_TRACE( "populate_config_field for " #__type );
  auto path = parent_path;
  LOG_TRACE( "push_back( {} )", name );
  path.push_back( name );
  LOG_TRACE( "path: {}", util::to_string( path ) );
  auto obj = ucl_from_path( config_name, path );
  CHECK_( obj.type() != ::UCL_NULL,
          "UCL Config field `" << util::join( path, "." )
                               << "` was not found in file "
                               << file << "." );
  LOG_TRACE( "obj is non-null" );
  CHECK_( obj.type() == ucl_type_of_v<T>,
          "expected `"
              << config_name << "." << util::join( path, "." )
              << "` to be of type " TO_STRING( __type ) );
  LOG_TRACE( "obj is of expected type " #__ucl_type );
  LOG_TRACE( "object getter is " #__ucl_getter );
  LOG_TRACE( "dest: {}", uint64_t( dest ) );
  *dest = static_cast<T>( (obj.*ucl_getter_for_type<T>)( {} ) );
  LOG_TRACE( "*dest: {}", obj.__ucl_getter() );
  used_field_paths.insert( file + "." +
                           util::join( path, "." ) );
}

// clang-format off
//
//        C++ type    UCL type       Getter
//        -----------------------------------------
UCL_TYPE( int,        UCL_INT,       int_value     )
UCL_TYPE( bool,       UCL_BOOLEAN,   bool_value    )
UCL_TYPE( double,     UCL_FLOAT,     number_value  )
UCL_TYPE( string,     UCL_STRING,    string_value  )
UCL_TYPE( X,          UCL_INT,       int_value     )
UCL_TYPE( W,          UCL_INT,       int_value     )
//UCL_TYPE( Y,          UCL_INT,       int_value     )
//UCL_TYPE( H,          UCL_INT,       int_value     )

// clang-format on

// This will traverse the object recursively and return a list
// of fully-qualified paths to all fields, e.g.:
//
//   object1.field1
//   object1.field2
//   object1.object2.field1
//   object1.object2.field2
//   ...
//
// Objects with no fields are ignored.
vector<string> get_all_fields( ucl::Ucl const& obj ) {
  vector<string> res;
  for( auto f : obj ) {
    if( f.type() == ::UCL_OBJECT ) {
      auto children = get_all_fields( f );
      for( auto const& s : children )
        res.push_back( f.key() + "." + s );
    } else {
      res.push_back( f.key() );
    }
  }
  return res;
}

} // namespace

#include "../config/config-vars.schema"

void load_configs() {
  for( auto const& f : load_functions() ) f();
  for( auto [ucl_name, file] : config_files() ) {
    // cout << "Loading file " << file << "\n";
    auto&  ucl_obj = ucl_configs[ucl_name];
    string errors;
    ucl_obj = ucl::Ucl::parse_from_file( file, errors );
    CHECK_( ucl_obj,
            "failed to load " << file << ": " << errors );
  }
  for( auto const& populate : populate_functions() ) populate();
  // This loop tests that there are no fields in the config
  // files that are not in the schema.  The program can still
  // run in this case, but we emit a warning since it could
  // be a sign of a problem.
  for( auto const& [ucl_name, file] : config_files() ) {
    auto fields = get_all_fields( ucl_configs[ucl_name] );
    for( auto const& f : fields ) {
      auto full_name = file + "." + f;
      if( !util::has_key( used_field_paths, full_name ) ) {
        logger->warn( "config field `{}' unused", full_name );
      } else {
        LOG_DEBUG( "field loaded: {}", full_name );
      }
    }
  }
}

} // namespace rn
