/****************************************************************
**config-files.hpp
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

#include "aliases.hpp"
#include "geo-types.hpp"
#include "mv-points.hpp"
#include "typed-int.hpp"

#include <optional>
#include <string>
#include <vector>

#define STRUCT( name, body ) \
  struct name {              \
    body                     \
  };

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
  extern config_##name##_t const config_##name;

#define OBJ( name, body )  \
  STRUCT( name##_t, body ) \
  name##_t name;

#define FLD( type, name ) type name;

#define LNK( from, to ) \
  decltype( ::rn::config_##to ) const* from;

namespace rn {

#include "../config/config-vars.schema"

void load_configs();

} // namespace rn
