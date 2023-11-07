/****************************************************************
**emit.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-08.
*
* Description: Rcl language emitter.
*
*****************************************************************/
#pragma once

// rcl
#include "model.hpp"

// C++ standard library
#include <string>

namespace rcl {

struct EmitOptions {
  bool flatten_keys = true;
};

std::string emit( doc const&         document,
                  EmitOptions const& options = {} );

// Since rcl is a superset of JSON, we can emit JSON as well.
std::string emit_json( doc const& document );

} // namespace rcl
