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
#include "fs.hpp"
#include "to-str.hpp"

// base-util
#include "base-util/string.hpp"

// C++ standard library
#include <chrono>
#include <deque>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

// FIXME: Putting these in base temporarily until we add an ADL
// tag.
namespace base {

void to_str( std::string_view o, std::string& out, ADL_t );
void to_str( std::string const& o, std::string& out, ADL_t );

template<Show T>
void to_str( std::vector<T> o, std::string& out, ADL_t ) {
  using namespace std::literals::string_literals;
  to_str( "["s, out, ADL );
  for( auto const& elem : o ) {
    to_str( elem, out, ADL );
    to_str( ","s, out, ADL );
  }
  to_str( "]"s, out, ADL );
}

void to_str( fs::path const& o, std::string& out, ADL_t );

void to_str( std::monostate const&, std::string& out, ADL_t );

template<typename... Ts>
void to_str( std::chrono::time_point<Ts...> const& o,
             std::string&                          out, ADL_t ) {
  out += "\"";
  out += util::to_string( o );
  out += "\"";
};

// {fmt} formatter for formatting unordered_maps whose contained
// types are formattable.
template<Show K, Show V>
void to_str( std::unordered_map<K, V> const& o, std::string& out,
             ADL_t ) {
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
template<Show K, Show V>
void to_str( std::map<K, V> const& o, std::string& out, ADL_t ) {
  out += "{";
  for( auto const& [k, v] : o )
    out += fmt::format( "{}={},", k, v );
  if( !o.empty() )
    // Remove trailing comma.
    out.pop_back();
  out += "}";
};

// {fmt} formatter for formatting unordered_sets whose contained
// {fmt} formatter for formatting unordered_sets whose contained
// types are formattable.
template<Show T>
void to_str( std::unordered_set<T> const& o, std::string& out,
             ADL_t ) {
  out += "[";
  for( auto const& elem : o ) out += fmt::format( "{},", elem );
  if( !o.empty() )
    // Remove trailing comma.
    out.resize( out.size() - 1 );
  out += "]";
};

// {fmt} formatter for formatting reference wrappers whose
// referenced type is formattable.
template<Show T>
void to_str( std::reference_wrapper<T> const& o,
             std::string&                     out, ADL_t ) {
  to_str( o.get(), out, ADL );
};

// {fmt} formatter for formatting pairs whose contained types are
// formattable.
template<Show T, Show U>
void to_str( std::pair<T, U> const& o, std::string& out,
             ADL_t ) {
  out += fmt::format( "({},{})", o.first, o.second );
};

// {fmt} formatter for formatting pairs whose contained types are
// formattable.
template<Show... Ts>
void to_str( std::tuple<Ts...> const& o, std::string& out,
             ADL_t ) {
  auto build = [&]<size_t... Idx>(
      std::index_sequence<Idx...> ) {
    ( ( out += fmt::format( "{}, ", std::get<Idx>( o ) ) ),
      ... );
    if( sizeof...( Idx ) > 0 ) out.resize( out.size() - 2 );
  };
  out += "(";
  build( std::index_sequence_for<Ts...>() ), out += ")";
};

// {fmt} formatter for formatting std::deque whose contained type
// is formattable.
template<Show T>
void to_str( std::deque<T> const& o, std::string& out, ADL_t ) {
  for( int i = 0; i < int( o.size() ); ++i ) {
    out += fmt::format( "{}", o[i] );
    if( i != int( o.size() - 1 ) ) out += ',';
  }
  out += ']';
};

// FIXME: move this somewhere else and improve it.
template<class Rep, class Period>
auto to_string_colons(
    std::chrono::duration<Rep, Period> const& duration ) {
  using namespace std::chrono;
  using namespace std::literals::chrono_literals;
  std::string res; // should use small-string optimization.
  auto        d = duration;

  bool has_hour_or_minute = false;
  if( d > 1h ) {
    auto hrs = duration_cast<hours>( d );
    res += fmt::format( "{:0>2}", hrs.count() );
    res += ':';
    d -= hrs;
    has_hour_or_minute = true;
  }
  if( d > 1min ) {
    auto mins = duration_cast<minutes>( d );
    res += fmt::format( "{:0>2}", mins.count() );
    res += ':';
    d -= mins;
    has_hour_or_minute = true;
  }
  auto secs = duration_cast<seconds>( d );
  res += fmt::format( "{:0>2}", secs.count() );
  if( !has_hour_or_minute ) res += 's';
  return res;
}

// {fmt} formatter for formatting duration.
template<class Rep, class Period>
void to_str( std::chrono::duration<Rep, Period> const& o,
             std::string& out, ADL_t ) {
  out += to_string_colons( o );
};

} // namespace base