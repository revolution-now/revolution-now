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
#undef LIST_OPT

#define FLD( __type, __name )                                 \
  static void __populate_##__name() {                         \
    auto path = this_path();                                  \
    path.push_back( #__name );                                \
    auto dotted = util::join( path, "." );                    \
    auto obj    = ucl_from_path( cfg_name(), dotted );        \
    used_field_paths.insert( this_file() + "." + dotted );    \
    populate_config_field(                                    \
        obj, const_cast<__type&>( dest_ptr()->__name ), path, \
        cfg_name(), this_file() );                            \
  }                                                           \
  static inline bool const __register_##__name = [] {         \
    populate_functions().push_back( __populate_##__name );    \
    return true;                                              \
  }();

#define FLD_OPT( type, name ) FLD( std::optional<type>, name )
#define LIST( type, name ) FLD( std::vector<type>, name )
#define LIST_OPT( type, name ) \
  FLD( std::optional<std::vector<type>>, name )

#define OBJ( __name, __body )                               \
  static auto* __name##_parent_ptr() { return dest_ptr(); } \
  static vector<string> __name##_parent_path() {            \
    return this_path();                                     \
  }                                                         \
  struct __name##_t;                                        \
  __name##_t* __##__name;                                   \
                                                            \
  struct __name##_t {                                       \
    static auto* dest_ptr() {                               \
      return &( __name##_parent_ptr()->__name );            \
    }                                                       \
    static vector<string> this_path() {                     \
      auto path = __name##_parent_path();                   \
      path.push_back( TO_STRING( __name ) );                \
      return path;                                          \
    }                                                       \
                                                            \
    __body                                                  \
  };

#define CFG( __name, __body )                                \
  config_##__name##_t const config_##__name{};               \
                                                             \
  struct shadow_config_##__name##_t {                        \
    static config_##__name##_t* dest_ptr() {                 \
      return const_cast<config_##__name##_t*>(               \
          &( config_##__name ) );                            \
    }                                                        \
    static vector<string> this_path() { return {}; }         \
    static string cfg_name() { return TO_STRING( __name ); } \
    static string this_file() {                              \
      return config_file_for_name( cfg_name() );             \
    }                                                        \
                                                             \
    __body                                                   \
  };                                                         \
                                                             \
  struct __startup_##__name {                                \
    static void __load_##__name() {                          \
      config_files().push_back(                              \
          {shadow_config_##__name##_t::cfg_name(),           \
           shadow_config_##__name##_t::this_file()} );       \
    }                                                        \
    static inline int const __register_##__name = [] {       \
      load_functions().push_back( __load_##__name );         \
      return 0;                                              \
    }();                                                     \
  };

#define UCL_TYPE( input, ucl_enum, ucl_getter )               \
  template<>                                                  \
  struct ucl_type_of_t<input> {                               \
    static constexpr UclType_t value = ucl_enum;              \
  };                                                          \
  template<>                                                  \
  struct ucl_type_name_of_t<input> {                          \
    static constexpr char const* value = #ucl_enum;           \
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

ucl::Ucl ucl_from_path( string const& name,
                        string const& dotted ) {
  // This must be by value since we reassign it.
  ucl::Ucl obj = ucl_configs[name];
  for( auto const& s : util::split( dotted, '.' ) ) {
    obj = obj[string( s )];
    if( !obj ) break;
  }
  return obj; // return it whether valid or not
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
struct ucl_type_name_of_t;

template<typename T>
auto ucl_getter_for_type = ucl_getter_for_type_t<T>::getter;

template<typename T>
UclType_t ucl_type_of_v = ucl_type_of_t<T>::value;

template<typename T>
char const* ucl_type_name_of_v = ucl_type_name_of_t<T>::value;

void check_field_exists( ucl::Ucl obj, string const& dotted,
                         string const& file ) {
  CHECK_( obj.type() != ::UCL_NULL,
          "UCL Config field `"
              << dotted << "` was not found in file " << file
              << "." );
}

void check_field_type( ucl::Ucl obj, UclType_t type,
                       string const& dotted,
                       string const& config_name,
                       string const& desc ) {
  CHECK_( obj.type() == type, "expected `"
                                  << config_name << "." << dotted
                                  << "` to contain " << desc );
}

#define DECLARE_POPULATE( type )                            \
  template<typename T>                                      \
  void populate_config_field(                               \
      ucl::Ucl obj, type& dest, vector<string> const& path, \
      string const& config_name, string const& file );

// Forward declare so that e.g. vector and optional variants
// can access each other if we have a nested type and are not
// always able to declare the containing type after the
// contained type.
DECLARE_POPULATE( vector<T> )
DECLARE_POPULATE( optional<T> )

// T (catch-all)
template<typename T>
void populate_config_field( ucl::Ucl obj, T& dest,
                            vector<string> const& path,
                            string const&         config_name,
                            string const&         file ) {
  auto dotted = util::join( path, "." );
  check_field_exists( obj, dotted, file );
  check_field_type(
      obj, ucl_type_of_v<T>, dotted, config_name,
      string( "item(s) of type " ) + ucl_type_name_of_v<T> );
  dest = static_cast<T>( (obj.*ucl_getter_for_type<T>)( {} ) );
}

// Coord
template<>
void populate_config_field( ucl::Ucl obj, Coord& dest,
                            vector<string> const& path,
                            string const&         config_name,
                            string const&         file ) {
  auto dotted = util::join( path, "." );
  check_field_exists( obj, dotted, file );
  check_field_type( obj, UCL_OBJECT, dotted, config_name,
                    "a coordinate pair object" );
  check_field_type(
      obj["x"], UCL_INT, dotted, config_name,
      "a coordinate pair with UCL_INT fields `x` and `y`" );
  check_field_type(
      obj["y"], UCL_INT, dotted, config_name,
      "a coordinate pair with UCL_INT fields `x` and `y`" );
  dest.x = obj["x"].int_value();
  dest.y = obj["y"].int_value();
}

// optional<T>
template<typename T>
void populate_config_field( ucl::Ucl obj, optional<T>& dest,
                            vector<string> const& path,
                            string const&         config_name,
                            string const&         file ) {
  if( obj.type() != ::UCL_NULL ) {
    dest = T{}; // must do this before calling .value()
    populate_config_field( obj, dest.value(), path, config_name,
                           file );
  } else {
    dest = nullopt;
  }
}

// vector<T>
template<typename T>
void populate_config_field( ucl::Ucl obj, vector<T>& dest,
                            vector<string> const& path,
                            string const&         config_name,
                            string const&         file ) {
  auto dotted = util::join( path, "." );
  check_field_exists( obj, dotted, file );
  check_field_type( obj, UCL_ARRAY, dotted, config_name,
                    "an array" );
  dest.resize( obj.size() );
  size_t idx = 0;
  for( auto elem : obj )
    populate_config_field( elem, dest[idx++], path, config_name,
                           file );
  CHECK( dest.size() == obj.size() );
}

// clang-format off
// ============================================================
//
//        C++ type    UCL type       Getter
//        -----------------------------------------
UCL_TYPE( int,        UCL_INT,       int_value      )
UCL_TYPE( bool,       UCL_BOOLEAN,   bool_value     )
UCL_TYPE( double,     UCL_FLOAT,     number_value   )
UCL_TYPE( string,     UCL_STRING,    string_value   )
UCL_TYPE( Coord,      UCL_OBJECT,    type /*dummy*/ )
UCL_TYPE( X,          UCL_INT,       int_value      )
UCL_TYPE( W,          UCL_INT,       int_value      )
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
