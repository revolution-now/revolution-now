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

// Abseil
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/container/node_hash_map.h"

// c++ standard library
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

/****************************************************************
** Time
*****************************************************************/
using Clock_t    = ::std::chrono::system_clock;
using Time_t     = decltype( Clock_t::now() );
using Duration_t = ::std::chrono::nanoseconds;
using Seconds    = ::std::chrono::seconds;

namespace chrono_literals = ::std::literals::chrono_literals;

using Str = ::std::string;

/****************************************************************
** Vocabulary Types
*****************************************************************/
template<typename T>
using Ref = ::std::reference_wrapper<T>;

template<typename T>
using CRef = ::std::reference_wrapper<T const>;

template<typename T>
using Vec = ::std::vector<T>;

template<typename T>
using Opt = ::std::optional<T>;

template<typename T>
using OptRef = Opt<::std::reference_wrapper<T>>;

template<typename T>
using OptCRef = OptRef<T const>;

template<typename T>
using ObserverPtr = ::nonstd::observer_ptr<T>;

template<typename T>
using ObserverCPtr = ObserverPtr<T const>;

template<typename T>
using UPtr = ::std::unique_ptr<T>;

template<typename T>
using UCPtr = ::std::unique_ptr<T const>;

template<typename K, typename V>
using FlatMap = ::absl::flat_hash_map<K, V>;

template<typename T>
using FlatSet = ::absl::flat_hash_set<T>;

template<typename K, typename V>
using NodeMap = ::absl::node_hash_map<K, V>;

template<typename F, typename S>
using Pair = std::pair<F, S>;

/****************************************************************
** Flatbuffers
*****************************************************************/
namespace flatbuffers {
class FlatBufferBuilder;
template<typename T>
struct Offset;
} // namespace flatbuffers

// These are probably not safe since the API probably does guar-
// antee that we can do this.
using FBBuilder = flatbuffers::FlatBufferBuilder;
template<typename T>
using FBOffset = flatbuffers::Offset<T>;

/****************************************************************
** Ranges
*****************************************************************/
namespace ranges {
inline namespace v3 {
namespace view {}
} // namespace v3
} // namespace ranges

namespace rv = ::ranges::view;
namespace rg = ::ranges;

/****************************************************************
** Source Location
*****************************************************************/
// source_location (coming in C++20)
#if __has_include( <experimental/source_location>)
#  include <experimental/source_location>
using SourceLoc = std::experimental::source_location;
#else
// Dummy to allow compilation to proceed; can be deleted after
// all compilers supported support C++20.
struct SourceLoc {
  int         line() const { return 0; }
  int         column() const { return 0; }
  char const* file_name() const { return "unknown"; }
  char const* function_name() const { return "unknown"; }

  static SourceLoc current() { return SourceLoc{}; }
};
NOTHROW_MOVE( SourceLoc );
#endif

namespace rn {} // namespace rn
