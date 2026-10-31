/****************************************************************
**rds-helpers.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-08-08.
*
* Description: Helpers for specifying ranges in config data.
*
*****************************************************************/
#pragma once

// ss
#include "rds-helpers.rds.hpp"

// refl
#include "refl/ext.hpp"

// C++ standard library
#include <string>

namespace rn {

/****************************************************************
** UniformDistT
*****************************************************************/
template<typename T>
base::valid_or<std::string> UniformDistT<T>::validate() const {
  REFL_VALIDATE( min <= max, "min must <= max." );
  return base::valid;
}

/****************************************************************
** Probability
*****************************************************************/
[[nodiscard]] Probability operator*( Probability l,
                                     Probability r );

} // namespace rn
