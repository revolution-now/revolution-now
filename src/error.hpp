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

/****************************************************************
**Death
*****************************************************************/
namespace rn {

// An exception to throw when you just want to exit. Mainly just
// for use during development.
struct exception_exit : public std::exception {};

} // namespace rn

/****************************************************************
** Inject some things from base.
*****************************************************************/
namespace rn {

using ::base::generic_err;
using ::base::GenericError;

} // namespace rn
