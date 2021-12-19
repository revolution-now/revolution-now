/****************************************************************
**to-str-tags.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-19.
*
* Description: Light wrappers for altering to_str behavior.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "to-str.hpp"

// C++ standard library
#include <ranges>
#include <vector>

namespace base {

/****************************************************************
** Concepts
*****************************************************************/
template<typename R>
concept FormattableRange =
    std::ranges::range<R> && base::Show<typename R::value_type>;

template<typename R>
concept FormattableSizedRange =
    FormattableRange<R> && std::ranges::sized_range<R>;

/****************************************************************
** FmtJsonStyleList
*****************************************************************/
// Stringifier for vectors whose contained type is formattable,
// in a JSON-like notation: [3,4,8,3]. However note that it is
// not real json since e.g. strings will not have quotes around
// them.
namespace detail {
  void json_style_list_impl( std::vector<std::string> const& vec,
                             std::string& out );
}

template<FormattableRange R>
struct FmtJsonStyleList {
  R const& rng;

  friend void to_str( FmtJsonStyleList const& o,
                      std::string&            out, ADL_t ) {
    std::vector<std::string> items;
    if constexpr( std::ranges::sized_range<R> )
      items.reserve( o.rng.size() );
    for( auto const& item : o.rng )
      items.push_back( fmt::format( "{}", item ) );
    detail::json_style_list_impl( items, out );
  }
};

template<FormattableRange R>
FmtJsonStyleList( R const& ) -> FmtJsonStyleList<R>;

/****************************************************************
** FmtVerticalJsonList
*****************************************************************/
// Stringifier for for vectors whose contained type is format-
// table, in a vertical pretty-printed JSON-style list with com-
// mas. This is a bit expensive.
namespace detail {
void vertical_json_list_impl(
    std::vector<std::string> const& vec, std::string& out );
}

template<FormattableSizedRange R>
struct FmtVerticalJsonList {
  R const& rng;

  // FIXME: get rid of this.
  bool operator==( FmtVerticalJsonList const& rhs ) const {
    return rng == rhs.rng;
  }

  friend void to_str( FmtVerticalJsonList const& o,
                      std::string&               out, ADL_t ) {
    std::vector<std::string> items;
    if constexpr( std::ranges::sized_range<R> )
      items.reserve( o.rng.size() );
    for( auto const& item : o.rng )
      items.push_back( fmt::format( "{}", item ) );
    detail::vertical_json_list_impl( items, out );
  }
};

template<FormattableSizedRange R>
FmtVerticalJsonList( R const& ) -> FmtVerticalJsonList<R>;

/****************************************************************
** FmtVerticalMap
*****************************************************************/
// Stringifier for maps in a vertical pretty-printed style. This
// is a bit expensive.
namespace detail {
void vertical_map_impl(
    std::vector<std::pair<std::string, std::string>> const& vec,
    std::string& out );
}

template<template<typename K, typename V, typename...>
         typename M,
         typename K, typename V>
struct FmtVerticalMap {
  M<K, V> const& m;

  bool operator==( M<K, V> const& rhs ) const {
    return m == rhs.m;
  }

  friend void to_str( FmtVerticalMap const& o, std::string& out,
                      ADL_t ) {
    std::vector<std::pair<std::string, std::string>> items;
    items.reserve( o.m.size() );
    for( auto const& [k, v] : o.m )
      items.push_back(
          { fmt::to_string( k ), fmt::to_string( v ) } );
    detail::vertical_map_impl( items, out );
  }
};

template<template<typename K, typename V, typename...>
         typename M,
         typename K, typename V>
FmtVerticalMap( M<K, V> const& ) -> FmtVerticalMap<M, K, V>;

} // namespace base