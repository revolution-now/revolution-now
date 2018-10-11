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

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

using OptStr = std::optional<std::string>;
using StrVec = std::vector<std::string>;
using SVVec  = std::vector<std::string_view>;

template<typename T>
using Ref = std::reference_wrapper<T>;

template<typename T>
using CRef = std::reference_wrapper<T const>;

template<typename T>
using OptRef  = std::optional<std::reference_wrapper<T>>;

template<typename T>
using OptCRef = std::optional<std::reference_wrapper<T const>>;

template<typename U, typename V>
using PairVec = std::vector<std::pair<U, V>>;

namespace rn {

} // namespace rn
