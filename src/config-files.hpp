/****************************************************************
**config-files.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-11-29.
*
* Description: Handles config file data.
*****************************************************************/
#pragma once

#include "core-config.hpp"

// gfx
#include "gfx/pixel.hpp"

// C++ standard library
#include <string>
#include <vector>

#define STRUCT( name, body ) \
  struct name {              \
    body                     \
  };                         \
  NOTHROW_MOVE( name );

// It is found that when a config object contains only fields
// and subobjects which can be constructed at compile time
// (constexpr constructable?) then the compiler may opt to put
// the structure into read-only memory since it is declared
// const.  However this then prevents us from populating it
// initially (leads to a segfault). We want it const, but we want
// to be able to const_cast it during initialization to give
// values to it.  So for thatreason we inject a std::string
// variable into the struct to prevent this.
#define CFG( name, body )                 \
  struct config_##name##_t {              \
    body std::string __prevent_read_only; \
  };                                      \
  NOTHROW_MOVE( config_##name##_t );      \
  extern config_##name##_t const config_##name;

#define OBJ( name, body )  \
  STRUCT( name##_t, body ) \
  name##_t name;

#define FLD( type, name ) type name;

#define LNK( from, to ) \
  decltype( ::rn::config_##to ) const* from;

namespace rn {

// This tells us if all of the configs have been fully loaded.
// This is useful for preventing bugs that would result from
// using the config data (global variables) before they are
// loaded from the contents of config files.
bool configs_loaded();

// The main global palette sorted using bucketed hue-sat-lum.
// Colors should normally be accessed using the config_palette
// global config structure, e.g. config_palette.red.sat0.lum1.
// This function should only be used during development e.g. to
// aid in color selection or troubleshooting and/or if one needs
// to iterate over all the colors in the palette. NOTE: this
// will skip over the grey scale colors.
std::vector<gfx::pixel> const& g_palette();

} // namespace rn
