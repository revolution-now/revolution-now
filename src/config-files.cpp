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
#include "color.hpp"
#include "conductor.hpp"
#include "coord.hpp"
#include "error.hpp"
#include "font.hpp"
#include "init.hpp"
#include "logger.hpp"
#include "mv-points.hpp"
#include "nation.hpp"
#include "tune.hpp"
#include "typed-int.hpp"
#include "utype.hpp"

// Rcl
#include "rcl/ext-base.hpp"
#include "rcl/ext-builtin.hpp"
#include "rcl/ext-std.hpp"
#include "rcl/ext.hpp"
#include "rcl/model.hpp"
#include "rcl/parse.hpp"

// Rds
#include "rds/helper/rcl.hpp"

// Revolution Now (config inl files)
#include "../config/all-rcl.inl"

// base-util
#include "base-util/pp.hpp"
#include "base-util/string.hpp"

// c++ standard library
#include <string>
#include <typeinfo>

using namespace std;
using namespace std::chrono;

#undef CFG
#undef OBJ
#undef FLD
#undef LNK

#define FLD( __type, __name )                              \
  static void __populate_##__name() {                      \
    auto path = this_path();                               \
    path.push_back( #__name );                             \
    auto dotted = util::join( path, "." );                 \
    used_field_paths.insert( this_file() + "." + dotted ); \
    rcl::value const& v =                                  \
        value_from_path( cfg_name(), dotted );             \
    rcl::convert_err<__type> res =                         \
        rcl::convert_to<__type>( v );                      \
    CHECK( res.has_value(),                                \
           "failed to produce type {} from {}.{}: {}",     \
           TO_STRING( __type ), cfg_name(), dotted,        \
           res.error() );                                  \
    const_cast<__type&>( dest_ptr()->__name ) =            \
        std::move( *res );                                 \
  }                                                        \
  static inline bool const __register_##__name = [] {      \
    populate_functions().push_back( __populate_##__name ); \
    return true;                                           \
  }();

template<typename T>
void assign_link( T const* const& from, T const& to ) {
  auto& strip_const = const_cast<T const*&>( from );
  strip_const       = &to;
}

#define LNK( __from, __to )                                 \
  static void __populate_##__from() {                       \
    /* for this to be allowed we'd have to guarantee */     \
    /* the ordering of initialization so that we ensure */  \
    /* that a link gets populated before any other links */ \
    /* that need to traverse it.  Until then, forbid it. */ \
    CHECK( !util::contains( #__to, "->" ),                  \
           "Link path should not traverse another link" );  \
    auto path = this_path();                                \
    path.push_back( #__from );                              \
    auto dotted = util::join( path, "." );                  \
    assign_link( dest_ptr()->__from, config_##__to );       \
    LOG_TRACE( "link assigned: {} -> {}",                   \
               "config_" + cfg_name() + "." + dotted,       \
               TO_STRING( config_##__to ) );                \
  }                                                         \
  static inline bool const __register_##__from = [] {       \
    populate_functions().push_back( __populate_##__from );  \
    return true;                                            \
  }();

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
  };                                                        \
  static void __populate_##__name() {                       \
    auto path = this_path();                                \
    path.push_back( #__name );                              \
    auto dotted = util::join( path, "." );                  \
    used_object_paths.insert( this_file() + "." + dotted ); \
  }                                                         \
  static inline bool const __register_##__name = [] {       \
    populate_functions().push_back( __populate_##__name );  \
    return true;                                            \
  }();

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
          { shadow_config_##__name##_t::cfg_name(),          \
            shadow_config_##__name##_t::this_file() } );     \
    }                                                        \
    static inline int const __register_##__name = [] {       \
      load_functions().push_back( __load_##__name );         \
      return 0;                                              \
    }();                                                     \
  };

namespace rn {

namespace {

unordered_map<string, rcl::doc> rcl_configs;

// List of field paths from the config that were found in the
// schema. This is used to warn the user of config variables on
// the config file but not in the schema.
unordered_set<string> used_field_paths;
unordered_set<string> used_object_paths;

rcl::value const& value_from_path( string const& name,
                                   string const& dotted ) {
  auto it = rcl_configs.find( name );
  CHECK( it != rcl_configs.end() );
  // This must be by value since we reassign it.
  rcl::value const* val = &it->second.top_val();
  for( auto const& s : util::split( dotted, '.' ) ) {
    auto maybe_tbl = val->get_if<unique_ptr<rcl::table>>();
    CHECK(
        maybe_tbl.has_value(),
        "config field path {}.{} does not exist or is invalid.",
        name, dotted );
    DCHECK( *maybe_tbl );
    auto& tbl = **maybe_tbl;
    CHECK( tbl.has_key( s ),
           "config field path {}.{} does not exist.", name,
           dotted );
    val = &tbl[s];
  }
  return *val;
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
  return "config/rcl/" + name + ".rcl";
}

void get_all_unused_fields_impl( string const&     parent_path,
                                 rcl::value const& v,
                                 vector<string>&   res ) {
  base::maybe<unique_ptr<rcl::table> const&> mtbl =
      v.get_if<unique_ptr<rcl::table>>();
  if( !mtbl ) return;
  rcl::table const& tbl = **mtbl;
  for( auto& [k, child_v] : tbl ) {
    string path = parent_path + "." + k;
    if( used_field_paths.contains( path ) )
      // If a field path is used by something declared as a
      // "field" in the config file, then skip it as well as any
      // child fields that it contains (if its value is a table)
      // since those child fields will be checked when deserial-
      // izing the object.
      continue;
    if( !used_object_paths.contains( path ) )
      res.push_back( path );
    get_all_unused_fields_impl( parent_path + "." + k, child_v,
                                res );
  }
}

vector<string> get_all_unused_fields( string const&     file,
                                      rcl::value const& v ) {
  vector<string> res;
  get_all_unused_fields_impl( file, v, res );
  return res;
}

void init_configs() {
  lg.info( "reading config files." );
  for( auto const& f : load_functions() ) f();
  for( auto [rcl_name, file] : config_files() ) {
    replace( file.begin(), file.end(), '_', '-' );
    base::expect<rcl::doc, std::string> doc =
        rcl::parse_file( file );
    CHECK( doc, "failed to load {}: {}", file, doc.error() );
    rcl_configs.emplace( rcl_name, std::move( *doc ) );
  }
  for( auto const& populate : populate_functions() ) populate();
  // This loop tests that there are no fields in the config files
  // that are not in the schema. The program can still run in
  // this case, but we emit a warning since it could be a sign of
  // a problem. Fields that are nested inside other fields (but
  // not nested inside rcl objects) will be tested during conver-
  // sion to objects and so they are not included here.
  for( auto const& [rcl_name, file] : config_files() ) {
    auto it = rcl_configs.find( rcl_name );
    CHECK( it != rcl_configs.end() );
    auto unused_fields =
        get_all_unused_fields( file, it->second.top_val() );
    for( auto const& f : unused_fields )
      lg.warn( "config field `{}' unused", f );
  }
  // Make sure this can load.
  (void)g_palette();
}

void cleanup_configs() {}

REGISTER_INIT_ROUTINE( configs );

} // namespace

} // namespace rn

// Revolution Now (config inl files)
#include "../config/all-rcl.inl"

namespace rn {

vector<Color> const& g_palette() {
  static vector<Color> colors = [] {
    vector<Color> res;
    string        file = "config/rcl/palette.rcl";

    base::expect<rcl::doc, std::string> doc =
        rcl::parse_file( file );
    CHECK( doc, "failed to load {}: {}", file, doc.error() );

    for( auto& [hue_key, hue_val] : doc->top_tbl() ) {
      if( hue_key == "grey" ) continue;
      UNWRAP_CHECK( hue_val_tbl,
                    hue_val.get_if<unique_ptr<rcl::table>>() );
      for( auto& [sat_key, sat_val] : *hue_val_tbl ) {
        UNWRAP_CHECK( sat_val_tbl,
                      sat_val.get_if<unique_ptr<rcl::table>>() );
        for( auto& [lum_key, lum_val] : *sat_val_tbl ) {
          UNWRAP_CHECK( parsed,
                        rcl::convert_to<Color>( lum_val ) );
          res.push_back( parsed );
        }
      }
    }
    hsl_bucketed_sort( res );
    return res;
  }();
  return colors;
}

} // namespace rn
