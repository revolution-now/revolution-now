/****************************************************************
**init.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-02-19.
*
* Description: Handles initialization of all subsystems.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "core-config.hpp"
#include "enum.hpp"

// base-util
#include "base-util/macros.hpp"

// C++ standard library
#include <functional>
#include <vector>

namespace rn {

// Ordering of these enum values does not matter.
enum class e_( init_routine,
               /************/
               configs,    //
               rng,        //
               sdl,        //
               fonts,      //
               app_window, //
               midi,       //
               screen,     //
               renderer,   //
               sprites,    //
               planes,     //
               sound,      //
               images,     //
               menus,      //
               terrain     //
);

using InitFunction = std::function<void( void )>;

void register_init_routine( e_init_routine      routine,
                            InitFunction const& init_func,
                            InitFunction const& cleanup_func );

#define REGISTER_INIT_ROUTINE( name, init_func, cleanup_func ) \
  STARTUP() {                                                  \
    register_init_routine( e_init_routine::name, init_func,    \
                           cleanup_func );                     \
  }

// Will run initialization routines in order of dependencies.
void run_all_init_routines();
// This will run the corresponding cleanup routine for each
// initialization routine that was successfully run, and will do
// so in the opposite order.
void run_all_cleanup_routines();

// Returns false during initialization.
bool has_init_finished();

#define SHOULD_BE_HERE_ONLY_DURING_INITIALIZATION      \
  CHECK( !has_init_finished(),                         \
         "This code path should only be taken during " \
         "initialization" )

} // namespace rn
