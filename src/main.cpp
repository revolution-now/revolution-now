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

/*
CONFIG_MODULE( rn ) {
  CONFIG_OBJECT( fruit ) {
    CONFIG_FIELD( apples, int )
    CONFIG_FIELD( oranges, int )
    CONFIG_FIELD( description, string )
  }
}

User code:

rn.fruit.apples;

*/

#define CONFIG_FIELD( field_name, type )

using RankedFunction = std::pair<int, function<void( void )>>;
vector<RankedFunction> config_registration_functions;

ucl::Ucl ucl_config;

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
struct ucl_getter_for_type;

template<typename T>
struct ucl_type_of;

#define UCL_TYPE( input, ucl_enum, ucl_name )                 \
  template<>                                                  \
  struct ucl_type_of<input> {                                 \
    static constexpr UclType value = ucl_enum;                \
  };                                                          \
  template<>                                                  \
  struct ucl_getter_for_type<input> {                         \
    using getter_t = decltype( &ucl::Ucl::ucl_name##_value ); \
    static constexpr getter_t getter =                        \
        &ucl::Ucl::ucl_name##_value;                          \
  }

UCL_TYPE( int, UCL_INT, int );
UCL_TYPE( std::string, UCL_STRING, string );
UCL_TYPE( bool, UCL_BOOLEAN, bool );
UCL_TYPE( double, UCL_FLOAT, number );

template<typename T>
auto ucl_getter_for_type_v = ucl_getter_for_type<T>::getter;

template<typename T>
UclType ucl_type_of_v = ucl_type_of<T>::value;

// template<typename T>
// T config_from_path( char const* path );

// template<>
// inline int config_from_path( char const* path ) {
//  ucl::Ucl obj = ucl_config;

//  auto try_direct = obj[path];
//  if( try_direct ) return try_direct.int_value();
//  vector<string> components =
//      absl::StrSplit( path, absl::ByChar( '.' ) );
//  for( auto const& s : components ) {
//    obj = obj[s];
//    ASSERT( obj, "key " << path << " not found in config" );
//  }
//  ASSERT( obj.type() == ::UCL_INT,
//          "key " << path << " expected to be of type int" );
//  return obj.int_value();
//}

struct config_object;
config_object* __config{nullptr};

struct config_object {
  using this_type = config_object;
  static config_object* this_ptr() { return __config; }
  static ConfigPath     this_path() { return {}; }
  static int            this_level() { return 0; }

  /** CONFIG_OBJECT( fruit ) *************************/
  static this_type* fruit_parent_ptr() { return this_ptr(); }
  static ConfigPath fruit_parent_path() { return this_path(); }
  static int        fruit_parent_level() { return this_level(); }
  static int fruit_level() { return fruit_parent_level() + 1; }

  struct fruit_object;
  fruit_object* __fruit;

  struct fruit_object {
    using this_type = fruit_object;
    static fruit_object* this_ptr() {
      return fruit_parent_ptr()->__fruit;
    }
    static ConfigPath this_path() {
      auto path = fruit_parent_path();
      path.push_back( "fruit" );
      return path;
    }
    static int this_level() { return fruit_level(); }

    /** CONFIG_FIELD( apples, int ) *************************/
    int               apples{};
    static this_type* apples_parent_ptr() { return this_ptr(); }
    static ConfigPath apples_parent_path() {
      return this_path();
    }
    static int apples_level() { return this_level() + 1; }

    static void __populate_apples() {
      using this_type            = int;
      char const* this_type_name = "int";
      auto        path           = apples_parent_path();
      path.push_back( "apples" );
      auto path_str = util::to_string( path );
      cout << "__populate_apples: " << path_str << "\n";
      auto obj = ucl_from_path( ucl_config, path );
      ASSERT( obj, path_str << "not found!" );
      ASSERT( obj.type() == ucl_type_of_v<this_type>,
              "key " << path_str << " expected to be of type "
                     << this_type_name );
      auto getter = ucl_getter_for_type_v<this_type>;

      apples_parent_ptr()->apples =
          ( obj.*getter )( this_type{} );
    }
    static inline bool const __register_apples = [] {
      cout << "__register_apples\n";
      config_registration_functions.push_back(
          {apples_level(), __populate_apples} );
      return true;
    }();
    /********************************************************/
  };
  fruit_object fruit;
  static void  __populate_fruit() {
    cout << "__populate_fruit\n";
    auto* p    = fruit_parent_ptr();
    p->__fruit = &p->fruit;
  }
  static inline int const __register_fruit = [] {
    cout << "__register_fruit\n";
    config_registration_functions.push_back(
        {fruit_level(), __populate_fruit} );
    return 0;
  }();
  /********************************************************/
};

config_object config;

void initialize_config() {
  __config = &config;
  string errors;
  ucl_config =
      ucl::Ucl::parse_from_file( "config/rn.ucl", errors );
  CHECK( ucl_config );
  cout << "[DIRECT] fruit.apples: "
       << ucl_config["fruit"]["apples"].int_value() << "\n";
  sort( config_registration_functions.begin(),
        config_registration_functions.end(),
        []( RankedFunction const& left,
            RankedFunction const& right ) {
          return left.first < right.first;
        } );
  for( auto const& [level, f] : config_registration_functions )
    f();
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

#define LOG_CONFIG( path ) \
  console->info( TO_STRING( path ) ": {}\n", path )

int main( int /*unused*/, char** /*unused*/ ) try {
  cout << "\n";
  initialize_config();
  // LOG_CONFIG( config.one );
  // LOG_CONFIG( config.two );
  LOG_CONFIG( config.fruit.apples );
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
