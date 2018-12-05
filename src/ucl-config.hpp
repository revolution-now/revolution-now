/****************************************************************
**ucl-config.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-11-29.
*
* Description: Handles config file data.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#include "errors.hpp"
#include "util.hpp"

#include "base-util/macros.hpp"
#include "base-util/misc.hpp"
#include "base-util/string.hpp"

#include <functional>
#include <string>
#include <utility>
#include <vector>

// This is just for debugging the registration of the the config
// data structure members.  Normally should not be enabled even
// for debug builds.
#if 0
#define CONFIG_LOG_STREAM std::cout
#else
#define CONFIG_LOG_STREAM util::cnull
#endif

#define CONFIG_FIELD( __type, __name )                      \
  static this_type* __name##_parent_ptr() {                 \
    return this_ptr();                                      \
  }                                                         \
  static ConfigPath __name##_parent_path() {                \
    return this_path();                                     \
  }                                                         \
  static int __name##_level() { return this_level() + 1; }  \
                                                            \
  __type const __name{};                                    \
                                                            \
  static void __populate_##__name() {                       \
    auto* dest = const_cast<__type*>(                       \
        &( __name##_parent_ptr()->__name ) );               \
    populate_config_field(                                  \
        dest, TO_STRING( __name ), __name##_level(),        \
        this_name(), __name##_parent_path(), this_file() ); \
  }                                                         \
  static inline bool const __register_##__name = [] {       \
    CONFIG_LOG_STREAM << "register: " << this_name()        \
                      << "." TO_STRING( __name ) "\n";      \
    config_registration_functions().push_back(              \
        {__name##_level(), __populate_##__name} );          \
    return true;                                            \
  }();

#define CONFIG_OBJECT( __name, __body )                         \
  static this_type* __name##_parent_ptr() {                     \
    return this_ptr();                                          \
  }                                                             \
  static ConfigPath __name##_parent_path() {                    \
    return this_path();                                         \
  }                                                             \
  static int __name##_parent_level() { return this_level(); }   \
                                                                \
  struct __name##_object;                                       \
  __name##_object* __##__name;                                  \
                                                                \
  struct __name##_object {                                      \
    using this_type = __name##_object;                          \
    static __name##_object* this_ptr() {                        \
      return __name##_parent_ptr()->__##__name;                 \
    }                                                           \
    static ConfigPath this_path() {                             \
      auto path = __name##_parent_path();                       \
      path.push_back( TO_STRING( __name ) );                    \
      return path;                                              \
    }                                                           \
    static int this_level() {                                   \
      return __name##_parent_level() + 1;                       \
    }                                                           \
                                                                \
    __body                                                      \
  };                                                            \
  __name##_object const __name{};                               \
                                                                \
  static void __populate_##__name() {                           \
    CONFIG_LOG_STREAM << __name##_object::this_level()          \
                      << " populate: " << this_name() << "."    \
                      << TO_STRING( __name ) << "\n";           \
    auto* parent = __name##_parent_ptr();                       \
    parent->__##__name =                                        \
        const_cast<__name##_object*>( &parent->__name );        \
  }                                                             \
  static inline int const __register_##__name = [] {            \
    CONFIG_LOG_STREAM << "register: " << this_name() << "."     \
                      << TO_STRING( __name ) "\n";              \
    config_registration_functions().push_back(                  \
        {__name##_object::this_level(), __populate_##__name} ); \
    return 0;                                                   \
  }();

#define CONFIG_FILE( __name, __body )                          \
  struct config_##__name##_object;                             \
  inline config_##__name##_object* __config_##__name{nullptr}; \
                                                               \
  struct config_##__name##_object {                            \
    using this_type = config_##__name##_object;                \
    static config_##__name##_object* this_ptr() {              \
      return __config_##__name;                                \
    }                                                          \
    static ConfigPath  this_path() { return {}; }              \
    static int         this_level() { return 0; }              \
    static std::string this_name() {                           \
      return TO_STRING( __name );                              \
    }                                                          \
    static std::string this_file() {                           \
      return config_file_for_name( this_name() );              \
    }                                                          \
                                                               \
    __body                                                     \
  };                                                           \
                                                               \
  inline config_##__name##_object const config_##__name{};     \
  struct startup_##__name {                                    \
    static void __load_##__name() {                            \
      __config_##__name =                                      \
          const_cast<config_##__name##_object*>(               \
              &config_##__name );                              \
      config_files().push_back(                                \
          {config_##__name##_object::this_name(),              \
           config_##__name##_object::this_file()} );           \
    }                                                          \
    static inline int const __register_##__name = [] {         \
      CONFIG_LOG_STREAM << "register load: "                   \
                        << TO_STRING( __name ) "\n";           \
      load_registration_functions().push_back(                 \
          __load_##__name );                                   \
      return 0;                                                \
    }();                                                       \
  };

// For compactness
#define FLD( a, b ) CONFIG_FIELD( a, b )
#define CFG( a, b ) CONFIG_FILE( a, b )
#define OBJ( a, b ) CONFIG_OBJECT( a, b )

namespace rn {

using RankedFunction =
    std::pair<int, std::function<void( void )>>;

std::vector<RankedFunction>& config_registration_functions();

std::vector<std::function<void( void )>>&
load_registration_functions();

using ConfigPath = std::vector<std::string>;

#define POPULATE_FIELD_DECL( __type )                   \
  void populate_config_field(                           \
      __type* dest, std::string const& name, int level, \
      std::string const& config_name,                   \
      ConfigPath const& parent_path, std::string const& file );

POPULATE_FIELD_DECL( bool )
POPULATE_FIELD_DECL( int )
POPULATE_FIELD_DECL( double )
POPULATE_FIELD_DECL( std::string )

// TODO: use fs::path here
using ConfigLoadPairs =
    std::vector<std::pair<std::string, std::string>>;

ConfigLoadPairs& config_files();

std::string config_file_for_name( std::string const& name );

void load_configs();

} // namespace rn
