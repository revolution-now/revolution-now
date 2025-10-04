/****************************************************************
**ext-std.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-03.
*
* Description: Traversable implementation for std library types.
*
*****************************************************************/
#pragma once

// traverse
#include "ext.hpp"

// base
#include "base/fs.hpp"

// C++ standard library
#include <algorithm>
#include <concepts>
#include <deque>
#include <map>
#include <queue>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace trv {

/****************************************************************
** string
*****************************************************************/
template<typename Fn>
void traverse( std::string const&, Fn&, tag_t<std::string> ) {}

/****************************************************************
** string_view
*****************************************************************/
template<typename Fn>
void traverse( std::string_view const&, Fn&,
               tag_t<std::string_view> ) {}

/****************************************************************
** std::filesystem::path
*****************************************************************/
template<typename Fn>
void traverse( fs::path const&, Fn&, tag_t<fs::path> ) {}

/****************************************************************
** std::chrono::seconds
*****************************************************************/
template<typename Fn>
void traverse( std::chrono::seconds const&, Fn&,
               tag_t<std::chrono::seconds> ) {}

/****************************************************************
** std::chrono::milliseconds
*****************************************************************/
template<typename Fn>
void traverse( std::chrono::milliseconds const&, Fn&,
               tag_t<std::chrono::milliseconds> ) {}

/****************************************************************
** std::chrono::microseconds
*****************************************************************/
template<typename Fn>
void traverse( std::chrono::microseconds const&, Fn&,
               tag_t<std::chrono::microseconds> ) {}

/****************************************************************
** std::pair
*****************************************************************/
template<typename Fst, typename Snd, typename Fn>
void traverse( std::pair<Fst, Snd> const& o, Fn& fn,
               tag_t<std::pair<Fst, Snd>> ) {
  using namespace std::literals;
  fn( o.first, "first"sv );
  fn( o.second, "second"sv );
}

/****************************************************************
** std::ranges::range
*****************************************************************/
// This will be used to implement traverse for any range.
template<std::ranges::range R, typename Fn>
void traverse( R const& o, Fn& fn, tag_t<R> ) {
  for( int i = 0; auto const& elem : o ) fn( elem, i++ );
}

/****************************************************************
** std::vector
*****************************************************************/
// traverse will use the std::ranges::range overload.

/****************************************************************
** std::queue
*****************************************************************/
// Can't read from this one.
template<typename T, typename Fn>
void traverse( std::queue<T> const& o, Fn& fn,
               tag_t<std::queue<T>> ) = delete;

/****************************************************************
** std::deque
*****************************************************************/
// traverse will use the std::ranges::range overload.

/****************************************************************
** std::array
*****************************************************************/
// traverse will use the std::ranges::range overload.

/****************************************************************
** unordered_map
*****************************************************************/
// Order not specified here.
template<typename K, typename V, typename Fn>
void traverse( std::unordered_map<K, V> const& o, Fn& fn,
               tag_t<std::unordered_map<K, V>> ) {
  for( auto& [k, v] : o ) fn( v, k );
}

/****************************************************************
** unordered_set
*****************************************************************/
// Order not specified here.
template<typename T, typename Fn>
void traverse( std::unordered_set<T> const& o, Fn& fn,
               tag_t<std::unordered_set<T>> ) {
  for( auto& elem : o ) fn( elem, none );
}

/****************************************************************
** map
*****************************************************************/
template<typename K, typename V, typename Fn>
void traverse( std::map<K, V> const& o, Fn& fn,
               tag_t<std::map<K, V>> ) {
  for( auto& [k, v] : o ) fn( v, k );
}

/****************************************************************
** set
*****************************************************************/
template<typename T, typename Fn>
void traverse( std::set<T> const& o, Fn& fn,
               tag_t<std::set<T>> ) {
  for( auto& elem : o ) fn( elem, none );
}

/****************************************************************
** std::unique_ptr
*****************************************************************/
template<typename T, typename Fn>
void traverse( std::unique_ptr<T> const& o, Fn& fn,
               tag_t<std::unique_ptr<T>> ) {
  if( o ) fn( *o, none );
}

/****************************************************************
** std::shared_ptr
*****************************************************************/
template<typename T, typename Fn>
void traverse( std::shared_ptr<T> const& o, Fn& fn,
               tag_t<std::shared_ptr<T>> ) {
  if( o ) fn( *o, none );
}

/****************************************************************
** std::variant
*****************************************************************/
// Hold off on handling a simple variant for which no reflection
// information is known, since then we would not be able to
// supply anything meaningful for the key. This is disabled for
// cdr as well for similar reasons. Maybe we'll come back to this
// in the future. There will be a more constrained overload in
// the reflection library that will provide these overloads in
// the case that the variant alternatives all consist of unique
// reflected structs, and hopefully that one will be selected in
// the relevant cases since it is more constrained.
template<typename... Ts, typename Fn>
void traverse( std::variant<Ts...> const& o,
               tag_t<std::variant<Ts...>> ) = delete;

} // namespace trv