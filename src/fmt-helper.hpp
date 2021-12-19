/****************************************************************
**fmt-helper.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-26.
*
* Description: Some helper utilities for using {fmt}.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// base
#include "base/fmt.hpp"
#include "base/to-str.hpp"

// base-util
#include "base-util/string.hpp"

// {fmt}
#include "fmt/format.h"

// C++ standard library
#include <concepts>
#include <ranges>
#include <string>
#include <type_traits>
#include <vector>

/****************************************************************
** Type Wrappers
*****************************************************************/
namespace rn {

template<typename R>
concept FormattableRange =
    std::ranges::range<R> && base::Show<typename R::value_type>;

template<typename R>
concept FormattableSizedRange =
    FormattableRange<R> && std::ranges::sized_range<R>;

template<FormattableRange R>
struct FmtJsonStyleList {
  R const& rng;
};

template<FormattableRange R>
FmtJsonStyleList( R const& ) -> FmtJsonStyleList<R>;

template<FormattableSizedRange R>
struct FmtVerticalJsonList {
  R const& rng;

  // FIXME: get rid of this.
  bool operator==( FmtVerticalJsonList const& rhs ) const {
    return rng == rhs.rng;
  }
};

template<FormattableSizedRange R>
FmtVerticalJsonList( R const& ) -> FmtVerticalJsonList<R>;

template<template<typename K, typename V, typename...>
         typename M,
         typename K, typename V>
struct FmtVerticalMap {
  M<K, V> const& m;

  bool operator==( M<K, V> const& rhs ) const {
    return m == rhs.m;
  }
};

template<template<typename K, typename V, typename...>
         typename M,
         typename K, typename V>
FmtVerticalMap( M<K, V> const& ) -> FmtVerticalMap<M, K, V>;

} // namespace rn

/****************************************************************
** Formatters for Type Wrappers
*****************************************************************/
namespace fmt {

// {fmt} formatter for vectors whose contained type is format-
// table, in a JSON-like notation: [3,4,8,3]. However note that
// it is not real json since e.g. strings will not have quotes
// around them.
template<typename R>
struct formatter<::rn::FmtJsonStyleList<R>>
  : formatter<std::string> {
  template<typename FormatContext>
  auto format( ::rn::FmtJsonStyleList<R> const& o,
               FormatContext&                   ctx ) {
    std::vector<std::string> items;
    if constexpr( std::ranges::sized_range<R> )
      items.reserve( o.rng.size() );
    for( auto const& item : o.rng )
      items.push_back( fmt::format( "{}", item ) );
    return formatter<std::string>::format(
        fmt::format( "[{}]", fmt::join( items, "," ) ), ctx );
  }
};

// {fmt} formatter for vectors whose contained type is format-
// table, in a vertical pretty-printed JSON-style list with com-
// mas.  This is a bit expensive.
template<typename R>
struct formatter<::rn::FmtVerticalJsonList<R>>
  : formatter<std::string> {
  template<typename FormatContext>
  auto format( ::rn::FmtVerticalJsonList<R> const& o,
               FormatContext&                      ctx ) {
    std::string res = "[";
    if( !o.rng.empty() ) {
      res += '\n';
      for( int i = 0; i < int( o.rng.size() ); ++i ) {
        std::string formatted = fmt::to_string( o.rng[i] );
        auto        lines     = util::split( formatted, '\n' );
        for( std::string_view line : lines )
          res += fmt::format( "  {}\n", line );
        if( i != int( o.rng.size() ) - 1 ) {
          res.resize( res.size() - 1 ); // remove newline.
          res += ',';
        }
        res += '\n';
      }
      res.resize( res.size() - 1 ); // remove newline.
    }
    res += "]";
    return formatter<std::string>::format( res, ctx );
  }
};

// {fmt} formatter for maps in a vertical pretty-printed style.
// This is a bit expensive.
template<template<typename K, typename V, typename...>
         typename M,
         typename K, typename V>
struct formatter<::rn::FmtVerticalMap<M, K, V>>
  : formatter<std::string> {
  template<typename FormatContext>
  auto format( ::rn::FmtVerticalMap<M, K, V> const& o,
               FormatContext&                       ctx ) {
    std::string res = "{";
    if( !o.m.empty() ) {
      res += '\n';
      int i = 0;
      for( auto const& [k, v] : o.m ) {
        std::string formatted_k = fmt::to_string( k );
        res += "  ";
        res += formatted_k;
        res += '=';
        std::string formatted_v = fmt::to_string( v );
        auto        lines = util::split( formatted_v, '\n' );
        for( int j = 0; j < int( lines.size() ); ++j ) {
          if( j != 0 ) res += "  ";
          res += lines[j];
          res += '\n';
        }
        if( i != int( o.m.size() ) - 1 ) {
          res.resize( res.size() - 1 ); // remove newline.
          res += ',';
          res += '\n';
        }
        ++i;
      }
    }
    res += "}";
    return formatter<std::string>::format( res, ctx );
  }
};

} // namespace fmt
