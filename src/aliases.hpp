/****************************************************************
**aliases.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-28.
*
* Description: Some `using` declarations to create convenient
*              type aliases.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// c++ standard library
#include <memory>
#include <string>
#include <vector>

/****************************************************************
** Vocabulary Types
*****************************************************************/
using Str = ::std::string;

template<typename T>
using Vec = ::std::vector<T>;

template<typename T>
using UPtr = ::std::unique_ptr<T>;

template<typename T>
using UCPtr = ::std::unique_ptr<T const>;

/****************************************************************
** Ranges
*****************************************************************/
namespace ranges {
inline namespace v3 {
namespace views {}
} // namespace v3
} // namespace ranges

namespace rv = ::ranges::views;
namespace rg = ::ranges;
