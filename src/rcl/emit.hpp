/****************************************************************
**emit.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-08.
*
* Description: Rcl language emitters.
*
*****************************************************************/
#pragma once

// rcl
#include "model.hpp"

// base
#include "base/maybe.hpp"

// C++ standard library
#include <string>

namespace rcl {

/****************************************************************
** Standard rcl dialect.
*****************************************************************/
struct EmitOptions {
  bool flatten_keys = true;
};

std::string emit( doc const&         document,
                  EmitOptions const& options = {} );

/****************************************************************
** JSON rcl dialect.
*****************************************************************/
struct JsonEmitOptions {
  base::maybe<std::string> key_order_tag;
};

// Since rcl is a superset of JSON, we can emit JSON as well.
std::string emit_json( doc const&      document,
                       JsonEmitOptions options = {} );

} // namespace rcl
