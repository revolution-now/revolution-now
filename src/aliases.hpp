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

// obesrver-ptr
#include "observer-ptr/observer-ptr.hpp"

// c++ standard library
#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

using TimeType = decltype( std::chrono::system_clock::now() );

// Deprecated; remove, since the name ordering is not consistent.
using StrVec = std::vector<std::string>;
using SVVec  = std::vector<std::string_view>;

template<typename U, typename V>
using PairVec = std::vector<std::pair<U, V>>;

// Use these from here down

using Str = std::string;

template<typename T>
using Ref = std::reference_wrapper<T>;

template<typename T>
using CRef = std::reference_wrapper<T const>;

template<typename T>
using Vec = std::vector<T>;

template<typename T>
using Opt = std::optional<T>;

template<typename T>
using OptRef = Opt<std::reference_wrapper<T>>;

template<typename T>
using OptCRef = OptRef<T const>;

template<typename T>
using OptVec = Opt<Vec<T>>;

using OptStr = Opt<std::string>;

template<typename U, typename V>
using VecPair = Vec<std::pair<U, V>>;

template<typename T>
using ObserverPtr = ::nonstd::observer_ptr<T>;

template<typename T>
using ObserverCPtr = ObserverPtr<T const>;

namespace rn {} // namespace rn
