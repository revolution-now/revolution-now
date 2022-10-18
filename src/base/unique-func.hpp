/****************************************************************
**unique-func.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-03-20.
*
* Description: Stamps out two specializations of unique_func
*              using the preprocessor.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// C++ standard library
#include <memory>
#include <type_traits>

namespace base {

// TODO: consider replacing this with std::move_only_function
// when the C++23 standard library is implemented.

template<typename T>
class unique_func;

}

// This stamps out two specializations of unique_func, one for
// const callables and one for non-const callables.
#define UNIQUE_FUNC_CONST
#include "unique-func-class.hpp"
#undef UNIQUE_FUNC_CONST
#define UNIQUE_FUNC_CONST const
#include "unique-func-class.hpp"
#undef UNIQUE_FUNC_CONST