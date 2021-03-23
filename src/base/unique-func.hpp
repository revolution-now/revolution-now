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

// C++ standard library
#include <memory>
#include <type_traits>

namespace base {

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