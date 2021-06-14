/****************************************************************
**func-concepts.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-14.
*
* Description: C++20 concepts related to function types.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "meta.hpp"

// C++ standard library
#include <concepts>
#include <type_traits>

namespace base {

template<typename T>
concept Function = std::is_function_v<T>;

template<typename T>
concept FunctionPointer = //
    std::is_pointer_v<T> &&
    std::is_function_v<std::remove_pointer_t<T>>;

template<typename T>
concept FunctionReference = //
    std::is_reference_v<T> &&
    std::is_function_v<std::remove_reference_t<T>>;

// clang-format off
template<typename T>
concept NonOverloadedCallable =
    Function<T>          ||
    FunctionPointer<T>   ||
    FunctionReference<T> ||
    requires( T o ) {
      &std::remove_cvref_t<T>::operator();
    };
// clang-format on

template<typename T>
concept NonOverloadedStatelessCallable =
    NonOverloadedCallable<T> && //
    std::is_convertible_v<
        T, std::add_pointer_t<mp::callable_func_type_t<T>>>;

template<typename T>
concept NonOverloadedStatefulCallable =
    NonOverloadedCallable<T> && //
    !NonOverloadedStatelessCallable<T>;

} // namespace base
