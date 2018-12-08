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

#include "typed-int.hpp"

#include <optional>
#include <string>
#include <vector>

#define STRUCT( name, body ) \
  struct name {              \
    body                     \
  };

#define CFG( name, body )           \
  STRUCT( config_##name##_t, body ) \
  extern config_##name##_t const config_##name;

#define OBJ( name, body )  \
  STRUCT( name##_t, body ) \
  name##_t name;

#define FLD( type, name ) type name;

#define FLD_OPT( type, name ) std::optional<type> name;

#define LIST( type, name ) std::vector<type> name;

namespace rn {

#include "../config/config-vars.schema"

void load_configs();

} // namespace rn
