#include "fonts.hpp"
#include "global-constants.hpp"
#include "globals.hpp"
#include "macros.hpp"
#include "ownership.hpp"
#include "sdl-util.hpp"
#include "sound.hpp"
#include "tiles.hpp"
#include "turn.hpp"
#include "unit.hpp"
#include "util.hpp"
#include "viewport.hpp"
#include "window.hpp"
#include "world.hpp"

#include "base-util/string.hpp"

#include "absl/strings/str_split.h"
#include "fmt/format.h"

#include "ucl++.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

using namespace rn;
using namespace std;

// clang-format off
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
// clang-format on

auto err_logger = spdlog::stderr_color_mt( "stderr" );
auto console    = spdlog::stdout_color_mt( "console" );

void stdout_example() {
  // create color multi threaded logger
  console->info( "Welcome to spdlog!" );
  console->error( "Some error message with arg: {}", 1 );

  err_logger->error( "Some error message" );

  // Formatting examples
  console->warn( "Easy padding in numbers like {:08d}",
                 12 ); // NOLINT
  console->critical(
      "Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: "
      "{0:b}",
      42 ); // NOLINT
  console->info( "Support for floats {:03.2f}",
                 1.23456 ); // NOLINT
  console->info( "Positional args are {1} {0}..", "too",
                 "supported" );
  console->info( "{:<30}", "left aligned" );

  spdlog::get( "console" )
      ->info(
          "loggers can be retrieved from a global registry "
          "using the spdlog::get(logger_name)" );

  // Runtime log levels
  spdlog::set_level(
      spdlog::level::info ); // Set global log level to info
  console->debug( "This message should not be displayed!" );
  console->set_level(
      spdlog::level::trace ); // Set specific logger's log level
  console->debug( "This message should be displayed.." );

  // Customize msg format for all loggers
  spdlog::set_pattern(
      "[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v" );
  console->info( "This an info message with custom format" );

  // Compile time log levels
  // define SPDLOG_DEBUG_ON or SPDLOG_TRACE_ON
  SPDLOG_TRACE( console,
                "Enabled only #ifdef SPDLOG_TRACE_ON..{} ,{}", 1,
                3.23 );
  SPDLOG_DEBUG( console,
                "Enabled only #ifdef SPDLOG_DEBUG_ON.. {} ,{}",
                1, 3.23 );
}

struct cnull_t {};
template<typename T>
inline cnull_t& operator<<( cnull_t& cnull, T const& ) {
  return cnull;
}
cnull_t cnull;

#if 0
#define CONFIG_LOG_STREAM std::cout
#else
#define CONFIG_LOG_STREAM cnull
#endif

#define CONFIG_FIELD( __name, __type )                         \
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
    ASSERT( obj.type() != ::UCL_NULL,                          \
            "UCL Config field `" << util::join( path, "." )    \
                                 << "` was not found in file " \
                                 << this_file() << "." );      \
    ASSERT( obj.type() == ucl_type_of<__type>,                 \
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
    config_registration_functions.push_back(                   \
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
    CONFIG_LOG_STREAM << "register: " << this_name()            \
                      << TO_STRING( __name ) "\n";              \
    config_registration_functions.push_back(                    \
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
  STARTUP() {                                                  \
    __config_##__name = &config_##__name;                      \
    config_files.push_back(                                    \
        {ucl_config_##__name,                                  \
         config_##__name##_object::this_file()} );             \
  }

// For convenience
#define F_( a, b ) CONFIG_FIELD( a, b )
#define C_( a, p, b ) CONFIG_FILE( a, b )
#define O_( a, p, b ) CONFIG_OBJECT( a, b )

using RankedFunction = std::pair<int, function<void( void )>>;
vector<RankedFunction> config_registration_functions;

using ConfigPath = std::vector<std::string>;

ucl::Ucl ucl_from_path( ucl::Ucl const&   ucl_config,
                        ConfigPath const& components ) {
  ucl::Ucl obj = ucl_config;
  for( auto const& s : components ) {
    obj = obj[s];
    if( !obj ) break;
  }
  return obj; // return it whether true or false
}

using UclType = decltype( ::UCL_INT );

template<typename T>
struct ucl_getter_for_type_t;

template<typename T>
struct ucl_type_of_t;

template<typename T>
auto ucl_getter_for_type = ucl_getter_for_type_t<T>::getter;

template<typename T>
UclType ucl_type_of = ucl_type_of_t<T>::value;

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

vector<pair<std::reference_wrapper<ucl::Ucl>, string>>
    config_files;

// clang-format off

/***********************************************************
* Mapping From C++ Types to UCL Types
*
*         C++ type       UCL Enum for Type   Ucl::???_value()
*         -------------------------------------------------*/
UCL_TYPE( int,           UCL_INT,            int           );
UCL_TYPE( bool,          UCL_BOOLEAN,        bool          );
UCL_TYPE( double,        UCL_FLOAT,          number        );
UCL_TYPE( std::string,   UCL_STRING,         string        );

/***********************************************************
* Main config file
*
*    Field Name                    Type
*    ------------------------------------------------------*/
C_(  rn                          , object                  ,
F_(    one                       , int                     )
F_(    two                       , std::string             )
F_(    hello                     , int                     )
O_(    fruit                     , object                  ,
F_(      apples                  , int                     )
F_(      oranges                 , int                     )
F_(      description             , std::string             )
O_(      hello                   , object                  ,
F_(        world                 , int                     )
)))

/***********************************************************
* GUI Config File
*
*    Field Name                    Type
*    ------------------------------------------------------*/
C_(  window                      , object                  ,
F_(    game_title                , std::string             )
F_(    game_version              , double                  )
O_(    window_error              , object                  ,
F_(      title                   , std::string             )
F_(      show                    , bool                    )
F_(      x_size                  , int                     )
))

/***********************************************************/
// clang-format on

void load_configs() {
  for( auto [ucl_obj, file] : config_files ) {
    string errors;
    ucl_obj.get() = ucl::Ucl::parse_from_file( file, errors );
    ASSERT( ucl_config_rn,
            "failed to load " << file << ": " << errors );
  }
  sort( config_registration_functions.begin(),
        config_registration_functions.end(),
        []( RankedFunction const& left,
            RankedFunction const& right ) {
          return left.first < right.first;
        } );
  for( auto const& p : config_registration_functions )
    p.second();
}

void game() {
  init_game();
  load_sprites();
  load_tile_maps();

  // CHECK( play_music_file( "assets/music/bonny-morn.mp3" ) );

  //(void)create_unit_on_map( e_unit_type::free_colonist, Y(2),
  // X(3) );
  (void)create_unit_on_map( e_unit_type::free_colonist, Y( 2 ),
                            X( 4 ) );
  (void)create_unit_on_map( e_unit_type::caravel, Y( 2 ),
                            X( 2 ) );
  //(void)create_unit_on_map( e_unit_type::caravel, Y(2), X(1) );

  while( turn() != e_turn_result::quit ) {}

  font_test();
  gui::test_window();

  cleanup();
}

#define LOG_CONFIG( path )                 \
  console->info( TO_STRING( path ) ": {}", \
                 util::to_string( path ) )

int main( int /*unused*/, char** /*unused*/ ) try {
  load_configs();

  LOG_CONFIG( config_rn.fruit.apples );
  LOG_CONFIG( config_rn.fruit.oranges );
  LOG_CONFIG( config_rn.fruit.description );
  LOG_CONFIG( config_rn.fruit.hello.world );
  LOG_CONFIG( config_rn.hello );
  LOG_CONFIG( config_rn.one );
  LOG_CONFIG( config_rn.two );
  LOG_CONFIG( config_window.game_version );
  LOG_CONFIG( config_window.game_title );
  LOG_CONFIG( config_window.window_error.title );
  LOG_CONFIG( config_window.window_error.x_size );
  LOG_CONFIG( config_window.window_error.show );

  // fmt::print( "Hello, {}!\n", "world" );
  // auto s = fmt::format( "this {} a {}.\n", "is", "test" );
  // fmt::print( s );

  // stdout_example();
  // console->info( "fruit.oranges: {}",
  //               config<int>( "fruit.oranges" ) );

  // game();
  return 0;

} catch( exception const& e ) {
  cerr << e.what() << endl;
  cerr << "SDL_GetError: " << SDL_GetError() << endl;
  cleanup();
  return 1;
}
