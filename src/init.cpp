/****************************************************************
**init.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-02-19.
*
* Description: Handles initialization of all subsystems.
*
*****************************************************************/
#include "init.hpp"

// Revolution Now
#include "errors.hpp"
#include "logging.hpp"
#include "ranges.hpp"

// base-util
#include "base-util/graph.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"

using namespace std;

namespace rn {

namespace {

bool g_init_finished{false};

auto& init_functions() {
  static absl::flat_hash_map<e_init_routine, InitFunction>
      s_init_functions;
  return s_init_functions;
}

auto& cleanup_functions() {
  static absl::flat_hash_map<e_init_routine, InitFunction>
      s_cleanup_functions;
  return s_cleanup_functions;
}

auto& init_routine_run_map() {
  static absl::flat_hash_map<e_init_routine, bool>
      s_init_routine_run;
  return s_init_routine_run;
}

// This records whether initialization has started. If an
// exception is thrown before this is `true` then no cleanup will
// be done at all (to avoid causing further errors).
bool g_init_has_started{false};

absl::flat_hash_map<e_init_routine, vector<e_init_routine>>
    g_init_deps{{e_init_routine::configs, {}},
                {e_init_routine::rng,
                 {
                     e_init_routine::configs //
                 }},
                {e_init_routine::sdl,
                 {
                     e_init_routine::configs //
                 }},
                {e_init_routine::fonts,
                 {
                     e_init_routine::configs, //
                     e_init_routine::sdl      //
                 }},
                {e_init_routine::app_window,
                 {
                     e_init_routine::configs, //
                     e_init_routine::sdl      //
                 }},
                {e_init_routine::screen,
                 {
                     e_init_routine::configs, //
                     e_init_routine::sdl      //
                 }},
                {e_init_routine::renderer,
                 {
                     e_init_routine::configs,    //
                     e_init_routine::app_window, //
                     e_init_routine::screen      //
                 }},
                {e_init_routine::sprites,
                 {
                     e_init_routine::configs, //
                     e_init_routine::sdl,     //
                     e_init_routine::renderer //
                 }},
                {e_init_routine::planes,
                 {
                     e_init_routine::configs, //
                     e_init_routine::sdl,     //
                     e_init_routine::screen,  //
                     e_init_routine::renderer //
                 }},
                {e_init_routine::sound,
                 {
                     e_init_routine::configs, //
                     e_init_routine::sdl      //
                 }},
                {e_init_routine::images,
                 {
                     e_init_routine::configs, //
                     e_init_routine::sdl      //
                 }},
                {e_init_routine::europe,
                 {
                     e_init_routine::configs,    //
                     e_init_routine::sdl,        //
                     e_init_routine::fonts,      //
                     e_init_routine::app_window, //
                     e_init_routine::screen,     //
                     e_init_routine::renderer,   //
                     e_init_routine::sprites,    //
                     e_init_routine::planes,     //
                     e_init_routine::images,     //
                     e_init_routine::menus       //
                 }},
                {e_init_routine::menus,
                 {
                     e_init_routine::configs,  //
                     e_init_routine::sdl,      //
                     e_init_routine::screen,   //
                     e_init_routine::renderer, //
                     e_init_routine::sprites,  //
                     e_init_routine::fonts     //
                 }},
                {e_init_routine::terrain,
                 {
                     e_init_routine::configs,  //
                     e_init_routine::renderer, // for block cache
                     e_init_routine::sprites,  // for block cache
                     e_init_routine::sdl       //
                 }},
                {e_init_routine::tunes,
                 {
                     e_init_routine::configs, //
                     e_init_routine::rng,     //
                 }},
                {e_init_routine::midiseq,
                 {
                     e_init_routine::configs, //
                 }},
                {e_init_routine::midiplayer,
                 {
                     e_init_routine::midiseq, //
                     e_init_routine::tunes,   //
                     e_init_routine::configs, //
                 }},
                {e_init_routine::oggplayer,
                 {
                     e_init_routine::sdl,     //
                     e_init_routine::sound,   //
                     e_init_routine::tunes,   //
                     e_init_routine::configs, //
                 }},
                {e_init_routine::conductor,
                 {
                     e_init_routine::tunes,      //
                     e_init_routine::midiplayer, //
                     e_init_routine::oggplayer,  //
                     // *** Should depend on all future music
                     // players added.
                     e_init_routine::configs, //
                 }}};

} // namespace

void register_init_routine( e_init_routine      routine,
                            InitFunction const& init_func,
                            InitFunction const& cleanup_func ) {
  CHECK( !init_functions().contains( routine ) );
  CHECK( !cleanup_functions().contains( routine ) );
  CHECK( !init_routine_run_map().contains( routine ) );
  init_functions()[routine]       = init_func;
  cleanup_functions()[routine]    = cleanup_func;
  init_routine_run_map()[routine] = false;
}

void run_all_init_routines( Opt<e_init_routine> only ) {
  // Logging must be initialized first, since we actually need it
  // in this function itself.
  init_logging( spdlog::level::debug );
  lg.debug( "initializing: logging" );

  // These should guarantee that the maps contain all enum
  // values, no more and no fewer.
  CHECK( e_init_routine::_values().size() == g_init_deps.size(),
         "The init routine dependency graph is missing some "
         "enum values" );
  CHECK( e_init_routine::_values().size() ==
             init_functions().size(),
         "not all e_init_routine values have registered "
         "init/cleanup functions" );
  CHECK( e_init_routine::_values().size() ==
             cleanup_functions().size(),
         "not all e_init_routine values have registered "
         "init/cleanup functions" );

  auto graph = util::make_graph( g_init_deps );
  CHECK( !graph.cyclic(),
         "there is a cycle in the dependency graph of "
         "initialization routine dependencies" );

  CHECK( !g_init_finished );

  // Now that we know there are no cycles, let's create a DAG.
  // This would actually throw an error if there is a cycle, but
  // we'd rather catch it here above.
  auto dag = util::DAG<e_init_routine>::make_dag( g_init_deps );

  // Should only be set after DAG creation succeeds.
  g_init_has_started = true;

  auto sorted = dag.sorted();

  // By default initialize all elements from the dag, unless the
  // caller has specified a routine on which to focus; in that
  // case, only initialize it and its dependencies.
  auto reachable = ( only.has_value() )
                       ? dag.accessible( only.value(),
                                         /*with_self=*/true )
                       : sorted;

  for( auto routine : sorted ) {
    if( rg::find( reachable, routine ) != reachable.end() ) {
      lg.debug( "initializing: {}", routine );
      init_functions()[routine]();
      init_routine_run_map()[routine] = true;
    }
  }

  g_init_finished = true;
}

void run_all_cleanup_routines() {
  if( !g_init_has_started ) {
    cerr << "skipping cleanup entirely as we did not even make "
            "it into subsystem initialization.\n";
    return;
  }
  auto dag = util::DAG<e_init_routine>::make_dag( g_init_deps );
  auto ordered = dag.sorted();

  // We must cleanup in the reverse order that we initialized.
  for( auto routine : ordered | rv::reverse ) {
    if( init_routine_run_map()[routine] ) {
      lg.debug( "cleaning: {}", routine );
      cleanup_functions()[routine]();
    }
  }
}

bool has_init_finished() { return g_init_finished; }

} // namespace rn
