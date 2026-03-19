/****************************************************************
**range-helpers.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-12-11.
*
* Description: Helpers for specifying ranges in config data.
*
*****************************************************************/
#pragma once

// Rds
#include "range-helpers.rds.hpp"

// refl
#include "refl/ext.hpp"

namespace rn {

template<typename T>
base::valid_or<std::string> config::UniformDistT<T>::validate()
    const {
  REFL_VALIDATE( min <= max, "min must <= max." );
  return base::valid;
}

} // namespace rn
