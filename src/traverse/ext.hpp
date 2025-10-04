/****************************************************************
**ext.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-03.
*
* Description: Concepts for traversing data structures.
*
*****************************************************************/
#pragma once

// C++ standard library
#include <concepts>
#include <string_view>
#include <type_traits>

namespace trv {

/****************************************************************
** ADL / Conversion helper tag
*****************************************************************/
// The fact that it is templated helps to prevent unwanted over-
// load ambiguities.
template<typename T>
struct tag_t {};

template<typename T>
inline constexpr tag_t<T> tag{};

/****************************************************************
** none_t
*****************************************************************/
// Used to designate that there is no key for a particular tra-
// versal.
struct none_t {};
inline constexpr none_t none{};

/****************************************************************
** Helpers for below concepts.
*****************************************************************/
namespace detail {
inline static auto const some_traversing_fn =
    []( auto& /*value*/, auto const& /*key*/ ) {};
} // namespace detail

/****************************************************************
** Concept: Traversable
*****************************************************************/
// For types that support traversing into their constituent
// parts.
//
// Note that the traverse function itself is not intended to be
// recursive; it just calls the callback for each immediate mem-
// ber. If the user wants to recurse then the callback should
// call traverse again. This is more flexible because it doesn't
// force recursion on the user, and also the user can choose
// whether they want e.g. depth-first or breadth-first traversal.
template<typename T>
concept Traversable = requires( T& o ) {
  {
    traverse( o, detail::some_traversing_fn,
              tag<std::remove_const_t<T>> )
  } -> std::same_as<void>;
};

/****************************************************************
** API methods.
*****************************************************************/
template<Traversable T, typename Fn>
void traverse( T& o, Fn&& fn ) {
  // NOTE: don't forward the function because the below traverse
  // method is expected to take it as an l-value ref since it may
  // need to be called multiple times.
  traverse( o, fn, tag<std::remove_const_t<T>> );
}

/****************************************************************
** Scalar types.
*****************************************************************/
template<typename T, typename Fn>
requires std::is_scalar_v<T>
inline void traverse( T const, Fn&, tag_t<T> ) {}

} // namespace trv