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

#include "errors.hpp"
#include "util.hpp"

#include "base-util/macros.hpp"
#include "base-util/misc.hpp"
#include "base-util/string.hpp"

#include <functional>
#include <string>
#include <utility>
#include <vector>

// TODO: add const back in

#define CFG( __name, __body )       \
  struct config_##__name##_object { \
    __body                          \
  };                                \
  extern config_##__name##_object config_##__name;

#define OBJ( __name, __body ) \
  struct __name##_object {    \
    __body                    \
  };                          \
  __name##_object __name;

#define FLD( __type, __name ) __type __name;

namespace rn {

#include "../config/config-vars.schema"

void load_configs();

} // namespace rn
