/****************************************************************
**to-str-ext-std.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-18.
*
* Description: to_str from std types.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "to-str.hpp"

// C++ standard library // FIXME: too many heavy includes below.
#include <array>
#include <deque>
#include <filesystem>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace std {

void to_str( string_view const& o, string& out,
             base::tag<string_view> );

void to_str( string const& o, string& out, base::tag<string> );

void to_str( nullptr_t const& o, std::string& out,
             base::tag<nullptr_t> );

template<base::Show T>
void to_str( vector<T> const& o, string& out,
             base::tag<vector<T>> ) {
  using namespace literals::string_literals;
  base::to_str( "[", out );
  for( auto const& elem : o ) {
    base::to_str( elem, out );
    out += ',';
  }
  if( !o.empty() )
    // Remove trailing comma.
    out.pop_back();
  base::to_str( "]", out );
}

template<base::Show T>
void to_str( queue<T> const& o, string& out,
             base::tag<queue<T>> ) {
  using namespace literals::string_literals;
  base::to_str( "<queue<T>:", out );
  base::to_str( o.size(), out );
  base::to_str( ">", out );
}

template<base::Show T>
void to_str( span<T> o, string& out, base::tag<span<T>> ) {
  using namespace literals::string_literals;
  base::to_str( "[", out );
  for( auto const& elem : o ) {
    base::to_str( elem, out );
    out += ',';
  }
  if( !o.empty() )
    // Remove trailing comma.
    out.pop_back();
  base::to_str( "]", out );
}

template<base::Show T, size_t Size>
void to_str( array<T, Size> o, string& out,
             base::tag<array<T, Size>> ) {
  base::to_str( span<T>( o ), out );
}

void to_str( filesystem::path const& o, string& out,
             base::tag<filesystem::path> );

void to_str( monostate const&, string& out,
             base::tag<monostate> );

// {fmt} formatter for formatting unordered_maps whose contained
// types are formattable.
template<base::Show K, base::Show V>
void to_str( unordered_map<K, V> const& o, string& out,
             base::tag<unordered_map<K, V>> ) {
  out += "{";
  for( auto const& [k, v] : o )
    out += fmt::format( "{}={},", k, v );
  if( !o.empty() )
    // Remove trailing comma.
    out.pop_back();
  out += "}";
};

// {fmt} formatter for formatting map whose contained types are
// formattable.
template<base::Show K, base::Show V>
void to_str( map<K, V> const& o, string& out,
             base::tag<map<K, V>> ) {
  out += "{";
  for( auto const& [k, v] : o )
    out += fmt::format( "{}={},", k, v );
  if( !o.empty() )
    // Remove trailing comma.
    out.pop_back();
  out += "}";
};

// {fmt} formatter for formatting unordered_sets whose contained
// types are formattable.
template<base::Show T>
void to_str( unordered_set<T> const& o, string& out,
             base::tag<unordered_set<T>> ) {
  out += "{";
  for( auto const& elem : o ) out += fmt::format( "{},", elem );
  if( !o.empty() )
    // Remove trailing comma.
    out.resize( out.size() - 1 );
  out += "}";
};

// {fmt} formatter for formatting sets whose contained types are
// formattable.
template<base::Show T>
void to_str( set<T> const& o, string& out, base::tag<set<T>> ) {
  out += "{";
  for( auto const& elem : o ) out += fmt::format( "{},", elem );
  if( !o.empty() )
    // Remove trailing comma.
    out.resize( out.size() - 1 );
  out += "}";
};

// {fmt} formatter for formatting reference wrappers whose
// referenced type is formattable.
template<base::Show T>
void to_str( reference_wrapper<T> const& o, string& out,
             base::tag<reference_wrapper<T>> ) {
  base::to_str( o.get(), out );
};

// {fmt} formatter for formatting pairs whose contained types are
// formattable.
template<base::Show T, base::Show U>
void to_str( pair<T, U> const& o, string& out,
             base::tag<pair<T, U>> ) {
  out += fmt::format( "({},{})", o.first, o.second );
};

template<base::Show T, T N>
void to_str( std::integral_constant<T, N> const o, string& out,
             base::tag<std::integral_constant<T, N>> ) {
  // Defer to T.
  to_str( o, out, base::tag<T>{} );
}

template<base::Show T>
void to_str( shared_ptr<T> const& o, string& out,
             base::tag<shared_ptr<T>> ) {
  if( o )
    base::to_str( *o, out );
  else
    base::to_str( "null", out );
}

// {fmt} formatter for formatting pairs whose contained types are
// formattable.
template<base::Show... Ts>
void to_str( tuple<Ts...> const& o, string& out,
             base::tag<tuple<Ts...>> ) {
  auto build = [&]<size_t... Idx>( index_sequence<Idx...> ) {
    ( ( out += fmt::format( "{}, ", get<Idx>( o ) ) ), ... );
    if( sizeof...( Idx ) > 0 ) out.resize( out.size() - 2 );
  };
  out += "(";
  build( index_sequence_for<Ts...>() ), out += ")";
};

// {fmt} formatter for formatting deque whose contained type
// is formattable.
template<base::Show T>
void to_str( deque<T> const& o, string& out,
             base::tag<deque<T>> ) {
  for( int i = 0; i < int( o.size() ); ++i ) {
    out += fmt::format( "{}", o[i] );
    if( i != int( o.size() - 1 ) ) out += ',';
  }
  out += ']';
};

void to_str( source_location const& o, string& out,
             base::tag<source_location> );

template<base::Show... Ts>
void to_str( variant<Ts...> const& o, string& out,
             base::tag<variant<Ts...>> ) {
  return visit( [&]( auto const& _ ) { base::to_str( _, out ); },
                o );
};

} // namespace std