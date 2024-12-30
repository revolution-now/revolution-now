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

#include "core-config.hpp"

// Revolution Now
#include "logger.hpp"
#include "maybe.hpp"

// Rds
#include "init.rds.hpp"

// base-util
#include "base-util/macros.hpp"

// C++ standard library
#include <functional>
#include <vector>

namespace rn {

struct IEngine;

using InitFunction = std::function<void( void )>;

void register_init_routine( e_init_routine routine,
                            InitFunction const& init_func,
                            InitFunction const& cleanup_func );

#define REGISTER_INIT_ROUTINE( name )                         \
  STARTUP() {                                                 \
    register_init_routine( e_init_routine::name, init_##name, \
                           cleanup_##name );                  \
  }

// Will run initialization routines in order of dependencies. If
// `level` is not nothing then just it and its dependencies will
// be run. Otherwise all routines will be run in order of depen-
// dencies.
void run_all_init_routines(
    maybe<e_log_level> level,
    std::initializer_list<e_init_routine> top_level = {} );

// This will run the corresponding cleanup routine for each
// initialization routine that was successfully run, and will do
// so in the opposite order.
void run_all_cleanup_routines();

void run_init_routine( IEngine& engine, e_init_routine routine );

} // namespace rn
