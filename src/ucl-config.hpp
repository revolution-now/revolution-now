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

#include "ucl++.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

// TODO: get UCL out of the header interface

// This is just for debugging the registration of the the config
// data structure members.  Normally should not be enabled even
// for debug builds.
#if 0
#define CONFIG_LOG_STREAM std::cout
#else
#define CONFIG_LOG_STREAM util::cnull
#endif

#define CONFIG_FIELD( __type, __name )                         \
  static this_type* __name##_parent_ptr() {                    \
    return this_ptr();                                         \
  }                                                            \
  static ConfigPath __name##_parent_path() {                   \
    return this_path();                                        \
  }                                                            \
  static int __name##_level() { return this_level() + 1; }     \
                                                               \
  __type __name{};                                             \
                                                               \
  static void __populate_##__name() {                          \
    auto path = __name##_parent_path();                        \
    path.push_back( TO_STRING( __name ) );                     \
    auto obj = ucl_from_path( this_config(), path );           \
    CHECK_( obj.type() != ::UCL_NULL,                          \
            "UCL Config field `" << util::join( path, "." )    \
                                 << "` was not found in file " \
                                 << this_file() << "." );      \
    CHECK_( obj.type() == ucl_type_of<__type>,                 \
            "expected `"                                       \
                << this_name() << "."                          \
                << util::join( path, "." )                     \
                << "` to be of type " TO_STRING( __type ) );   \
    __name##_parent_ptr()->__name =                            \
        (obj.*ucl_getter_for_type<__type>)( __type{} );        \
    CONFIG_LOG_STREAM << __name##_level()                      \
                      << " populate: " << this_name() << "."   \
                      << util::join( path, "." ) << " = "      \
                      << util::to_string(                      \
                             __name##_parent_ptr()->__name )   \
                      << "\n";                                 \
  }                                                            \
  static inline bool const __register_##__name = [] {          \
    CONFIG_LOG_STREAM << "register: " << this_name()           \
                      << "." TO_STRING( __name ) "\n";         \
    config_registration_functions().push_back(                 \
        {__name##_level(), __populate_##__name} );             \
    return true;                                               \
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
  __name##_object __name;                                       \
                                                                \
  static void __populate_##__name() {                           \
    CONFIG_LOG_STREAM << __name##_object::this_level()          \
                      << " populate: " << this_name() << "."    \
                      << TO_STRING( __name ) << "\n";           \
    auto* parent       = __name##_parent_ptr();                 \
    parent->__##__name = &parent->__name;                       \
  }                                                             \
  static inline int const __register_##__name = [] {            \
    CONFIG_LOG_STREAM << "register: " << this_name() << "."     \
                      << TO_STRING( __name ) "\n";              \
    config_registration_functions().push_back(                  \
        {__name##_object::this_level(), __populate_##__name} ); \
    return 0;                                                   \
  }();

#define CONFIG_FILE( __name, __body )                          \
  inline ucl::Ucl ucl_config_##__name;                         \
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
    static ucl::Ucl& this_config() {                           \
      return ucl_config_##__name;                              \
    }                                                          \
    static std::string this_file() {                           \
      return "config/" + this_name() + ".ucl";                 \
    }                                                          \
                                                               \
    __body                                                     \
  };                                                           \
                                                               \
  inline config_##__name##_object config_##__name;             \
  struct startup_##__name {                                    \
    static void __load_##__name() {                            \
      __config_##__name = &config_##__name;                    \
      config_files().push_back(                                \
          {ucl_config_##__name,                                \
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

#define UCL_TYPE( input, ucl_enum, ucl_name )                 \
  template<>                                                  \
  struct ucl_type_of_t<input> {                               \
    static constexpr UclType value = ucl_enum;                \
  };                                                          \
  template<>                                                  \
  struct ucl_getter_for_type_t<input> {                       \
    using getter_t = decltype( &ucl::Ucl::ucl_name##_value ); \
    static constexpr getter_t getter =                        \
        &ucl::Ucl::ucl_name##_value;                          \
  }

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

using UclType = decltype( ::UCL_INT );

template<typename T>
struct ucl_getter_for_type_t;

template<typename T>
struct ucl_type_of_t;

template<typename T>
auto ucl_getter_for_type = ucl_getter_for_type_t<T>::getter;

template<typename T>
UclType ucl_type_of = ucl_type_of_t<T>::value;

/****************************************************************
 * Mapping From C++ Types to UCL Types
 *
 *         C++ type         UCL Enum              Ucl::???_value
 *         ------------------------------------------------------*/
UCL_TYPE( int, UCL_INT, int );
UCL_TYPE( bool, UCL_BOOLEAN, bool );
UCL_TYPE( double, UCL_FLOAT, number );
UCL_TYPE( std::string, UCL_STRING, string );
/****************************************************************/

using ConfigLoadPairs = std::vector<
    std::pair<std::reference_wrapper<ucl::Ucl>, std::string>>;

ConfigLoadPairs& config_files();

ucl::Ucl ucl_from_path( ucl::Ucl const&   ucl_config,
                        ConfigPath const& components );

void load_configs();

} // namespace rn
