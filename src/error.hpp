/****************************************************************
**errors.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-04.
*
* Description: Error handling utilities.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// base
#include "base/error.hpp"

// C++ standard library
#include <stdexcept>

namespace rn {

/****************************************************************
**Death
*****************************************************************/
// An exception to throw when you just want to exit. Mainly just
// for use during development.
struct exception_exit : public std::exception {};

/****************************************************************
** Inject some things from base.
*****************************************************************/
using ::base::generic_err;
using ::base::GenericError;

} // namespace rn
