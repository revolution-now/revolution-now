/****************************************************************
**function-ref.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-04.
*
* Description: Alias for the function-ref used in this code base.
*
*****************************************************************/
#pragma once

// Abseil
#include "absl/functional/function_ref.h"

namespace base {

template<typename T>
using function_ref = absl::FunctionRef<T>;

} // namespace base

namespace rn {

template<typename T>
using function_ref = absl::FunctionRef<T>;

} // namespace rn