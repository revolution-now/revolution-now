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
#include "base/function-ref.hpp"
#include "base/maybe.hpp"

// C++ standard library
#include <stdexcept>

namespace rn {

/****************************************************************
**Death
*****************************************************************/
// An exception to throw when you just want to exit. Mainly just
// for use during development.
struct exception_exit : public std::exception {};

// This should be be used to register a callback to be called to
// cleanup the engine just before aborting the process, since
// otherwise the usual cleanup routines won't get run. This can
// lead to bad things such as midi synths not being stopped.
void register_cleanup_callback_on_abort(
    base::maybe<base::function_ref<void() const>> fn );

/****************************************************************
** Inject some things from base.
*****************************************************************/
using ::base::generic_err;
using ::base::GenericError;

} // namespace rn
