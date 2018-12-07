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

#define FLD( __type, __name )                                \
  static void __populate_##__name() {                        \
    populate_config_field( &( dest_ptr()->__name ),          \
                           TO_STRING( __name ), this_name(), \
                           this_path(), this_file() );       \
  }                                                          \
  static inline bool const __register_##__name = [] {        \
    populate_functions().push_back(                          \
        {this_level() + 1, __populate_##__name} );           \
    return true;                                             \
  }();

#define OBJ( __name, __body )                                    \
  static auto*      __name##_parent_ptr() { return dest_ptr(); } \
  static ConfigPath __name##_parent_path() {                     \
    return this_path();                                          \
  }                                                              \
  static int __name##_parent_level() { return this_level(); }    \
                                                                 \
  struct __name##_object;                                        \
  __name##_object* __##__name;                                   \
                                                                 \
  struct __name##_object {                                       \
    static auto* dest_ptr() {                                    \
      return &( __name##_parent_ptr()->__name );                 \
    }                                                            \
    static ConfigPath this_path() {                              \
      auto path = __name##_parent_path();                        \
      path.push_back( TO_STRING( __name ) );                     \
      return path;                                               \
    }                                                            \
    static int this_level() {                                    \
      return __name##_parent_level() + 1;                        \
    }                                                            \
                                                                 \
    __body                                                       \
  };

#define CFG( __name, __body )                                     \
  config_##__name##_object const config_##__name{};               \
                                                                  \
  struct shadow_config_##__name##_object {                        \
    static config_##__name##_object* dest_ptr() {                 \
      return const_cast<config_##__name##_object*>(               \
          &( config_##__name ) );                                 \
    }                                                             \
    static ConfigPath this_path() { return {}; }                  \
    static int        this_level() { return 0; }                  \
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
          {shadow_config_##__name##_object::this_name(),          \
           shadow_config_##__name##_object::this_file()} );       \
    }                                                             \
    static inline int const __register_##__name = [] {            \
      load_functions().push_back( __load_##__name );              \
      return 0;                                                   \
    }();                                                          \
  };

#define POPULATE_FIELD( __type, __ucl_type, __ucl_getter )      \
  namespace {                                                   \
  inline void populate_config_field(                            \
      __type* dest, string const& name,                         \
      string const& config_name, ConfigPath const& parent_path, \
      string const& file ) {                                    \
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
    used_field_paths.insert( file + "." +                       \
                             util::join( path, "." ) );         \
  }                                                             \
                                                                \
  inline void populate_config_field(                            \
      optional<__type>* dest, string const& name,               \
      string const& config_name, ConfigPath const& parent_path, \
      string const& file ) {                                    \
    auto path = parent_path;                                    \
    path.push_back( name );                                     \
    auto obj = ucl_from_path( config_name, path );              \
    (void)file;                                                 \
    if( obj.type() != ::UCL_NULL ) {                            \
      CHECK_( obj.type() == __ucl_type,                         \
              "expected non-null `"                             \
                  << config_name << "."                         \
                  << util::join( path, "." )                    \
                  << "` to be of type " TO_STRING( __type ) );  \
      *dest = obj.__ucl_getter();                               \
    } else {                                                    \
      /* This could happen either if the field is absent */     \
      /* or if it is set to `null` without quotes. */           \
      *dest = nullopt;                                          \
    }                                                           \
    used_field_paths.insert( file + "." +                       \
                             util::join( path, "." ) );         \
  }                                                             \
                                                                \
  inline void populate_config_field(                            \
      vector<__type>* dest, string const& name,                 \
      string const& config_name, ConfigPath const& parent_path, \
      string const& file ) {                                    \
    auto path = parent_path;                                    \
    path.push_back( name );                                     \
    auto obj = ucl_from_path( config_name, path );              \
    CHECK_( obj.type() != ::UCL_NULL,                           \
            "UCL Config field `" << util::join( path, "." )     \
                                 << "` was not found in file "  \
                                 << file << "." );              \
    CHECK_( obj.type() == ::UCL_ARRAY,                          \
            "expected `"                                        \
                << config_name << "."                           \
                << util::join( path, "." )                      \
                << "` to be of type vector<" TO_STRING(         \
                       __type ) ">" );                          \
    CHECK( dest->empty() );                                     \
    dest->reserve( obj.size() ); /* array size */               \
    for( auto elem : obj ) {                                    \
      CHECK_(                                                   \
          elem.type() == __ucl_type,                            \
          "expected elements in array `"                        \
              << config_name << "." << util::join( path, "." )  \
              << "` to be of type " TO_STRING( __ucl_type ) );  \
      dest->push_back(                                          \
          static_cast<__type>( elem.__ucl_getter() ) );         \
    }                                                           \
    CHECK( dest->size() == obj.size() );                        \
    used_field_paths.insert( file + "." +                       \
                             util::join( path, "." ) );         \
  }                                                             \
  } /* namespace */                                             \
  /* This is just to suppress `unused function` warnings. */    \
  /* Note that it must be outside of the anonymouse namespace*/ \
  bool dummy_##__type = []( bool b ) {                          \
    if( b ) {                                                   \
      populate_config_field( ( __type* ){}, {}, {}, {}, {} );   \
      populate_config_field( ( vector<__type>* ){}, {}, {}, {}, \
                             {} );                              \
      populate_config_field( ( optional<__type>* ){}, {}, {},   \
                             {}, {} );                          \
    }                                                           \
    return true;                                                \
  }( false );

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

using RankedFunction = pair<int, function<void( void )>>;

vector<RankedFunction>& populate_functions() {
  static vector<RankedFunction> populate_functions;
  return populate_functions;
}

vector<function<void( void )>>& load_functions() {
  static vector<function<void( void )>> load_functions;
  return load_functions;
}

string config_file_for_name( string const& name ) {
  return "config/" + name + ".ucl";
}

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
//
// NOTE: at the time of this writing, this won't work properly
// when there are null fields in an object because libucl has
// a bug where iteration of fields stops upon encountering a
// field with a null value.
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

// clang-format off
/****************************************************************
* Mapping From C++ Types to UCL Types
* Requirements:
*   the `C++ type` must be constructable and static_cast'able
*   from the type returned by `Ucl Getter`
*
*               C++ type        UCL Enum        Ucl Getter
*               ------------------------------------------------*/
POPULATE_FIELD( int,            UCL_INT,        int_value      )
POPULATE_FIELD( bool,           UCL_BOOLEAN,    bool_value     )
POPULATE_FIELD( double,         UCL_FLOAT,      number_value   )
POPULATE_FIELD( string,         UCL_STRING,     string_value   )
POPULATE_FIELD( X,              UCL_INT,        int_value      )
POPULATE_FIELD( Y,              UCL_INT,        int_value      )
POPULATE_FIELD( W,              UCL_INT,        int_value      )
POPULATE_FIELD( H,              UCL_INT,        int_value      )
/****************************************************************/
// clang-format on

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
  sort( populate_functions().begin(), populate_functions().end(),
        []( RankedFunction const& left,
            RankedFunction const& right ) {
          return left.first < right.first;
        } );
  for( auto const& populate : populate_functions() )
    populate.second();
  // This loop tests that there are no fields in the config
  // files that are not in the schema.  The program can still
  // run in this case, but we emit a warning since it could
  // be a sign of a problem.
  for( auto const& [ucl_name, file] : config_files() ) {
    auto fields = get_all_fields( ucl_configs[ucl_name] );
    for( auto const& f : fields ) {
      auto full_name = file + "." + f;
      if( !util::has_key( used_field_paths, full_name ) )
        console->warn( "config field `{}' unused", full_name );
    }
  }
}

} // namespace rn
