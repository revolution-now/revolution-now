/****************************************************************
**ext-std.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-27.
*
* Description: Cdr conversions for std types.
*
*****************************************************************/
#pragma once

// cdr
#include "ext.hpp"

// base
#include "base/fs.hpp"

// C++ standard library
#include <concepts>
#include <ranges>
#include <string>
#include <string_view>

namespace cdr {

/****************************************************************
** string
*****************************************************************/
value to_canonical( std::string const& o, tag_t<std::string> );

result<std::string> from_canonical( value const& v,
                                    tag_t<std::string> );

/****************************************************************
** string_view
*****************************************************************/
value to_canonical( std::string_view const& o,
                    tag_t<std::string_view> );

// Don't implement this because it will probably be indicative of
// a dangling-string_view bug.
result<std::string_view> from_canonical(
    value const& v, tag_t<std::string_view> ) = delete;

/****************************************************************
** std::filesystem::path
*****************************************************************/
// TODO

/****************************************************************
** std::chrono::seconds
*****************************************************************/
// TODO

/****************************************************************
** std::pair
*****************************************************************/
template<ToCanonical Fst, ToCanonical Snd>
value to_canonical( std::pair<Fst, Snd> const& o,
                    tag_t<std::pair<Fst, Snd>> ) {
  return table{ { "fst", o.first }, { "snd", o.second } };
}

template<FromCanonical Fst, FromCanonical Snd>
result<std::pair<Fst, Snd>> from_canonical(
    value const& v, tag_t<std::pair<Fst, Snd>> ) {
  static std::string kFrameName = "std::pair";
  auto               maybe_tbl  = v.get_if<table>();
  if( !maybe_tbl.has_value() )
    return error::build{ "std::pair" }(
        "producing a std::pair requires type table, instead "
        "found type {}.",
        type_name( v ) );
  table const& tbl = *maybe_tbl;
  if( !tbl.contains( "fst" ) || !tbl.contains( "snd" ) )
    return error::build{ "std::pair" }(
        "table must have both a 'fst' and 'snd' field for "
        "conversion to std::pair." );
  CDR_UNWRAP_RETURN( fst, from_canonical<Fst>( *tbl["fst"] ) );
  CDR_UNWRAP_RETURN( snd, from_canonical<Snd>( *tbl["snd"] ) );
  return std::pair<Fst, Snd>{ std::move( fst ),
                              std::move( snd ) };
}

/****************************************************************
** std::ranges::range
*****************************************************************/
// This will be used to implement to_canonical for any range.
// clang-format off
template<std::ranges::range R>
requires ToCanonical<typename R::value_type>
value to_canonical( R const& o, tag_t<R> ) {
  // clang-format on
  list res;
  if constexpr( std::ranges::sized_range<R> )
    res.reserve( o.size() );
  for( auto const& elem : o )
    res.push_back(
        to_canonical( elem, tag<typename R::value_type> ) );
  return res;
}

/****************************************************************
** std::vector
*****************************************************************/
// to_canonical will use the std::ranges::range overload.

template<FromCanonical T>
result<std::vector<T>> from_canonical( value const& v,
                                       tag_t<std::vector<T>> ) {
  static std::string kFrameName = "std::vector";
  auto               maybe_lst  = v.get_if<list>();
  if( !maybe_lst.has_value() )
    return error::build{ "std::vector" }(
        "producing a std::vector requires type list, instead "
        "found type {}.",
        type_name( v ) );
  list const&    lst = *maybe_lst;
  std::vector<T> res;
  res.reserve( lst.size() );
  for( value const& elem : lst ) {
    CDR_UNWRAP_RETURN( val, from_canonical<T>( elem ) );
    res.push_back( val );
  }
  return res;
}

/****************************************************************
** std::array
*****************************************************************/
// to_canonical will use the std::ranges::range overload.

template<FromCanonical T, size_t N>
result<std::array<T, N>> from_canonical(
    value const& v, tag_t<std::array<T, N>> ) {
  static std::string kFrameName = "std::array";
  auto               maybe_lst  = v.get_if<list>();
  if( !maybe_lst.has_value() )
    return error::build{ "std::array" }(
        "producing a std::array requires type list, instead "
        "found type {}.",
        type_name( v ) );
  list const& lst = *maybe_lst;
  if( lst.size() != N )
    return error::build{ "std::array" }(
        "expected list of size {} for producing std::array of "
        "that same size, instead found size {}.",
        N, lst.size() );
  std::array<T, N> res;
  size_t           idx = 0;
  for( value const& elem : lst ) {
    CDR_UNWRAP_RETURN( val, from_canonical<T>( elem ) );
    res[idx++] = val;
  }
  return res;
}

/****************************************************************
** unordered_map
*****************************************************************/
// to_canonical will use the std::ranges::range overload.

// clang-format off
template<typename K, typename V>
requires FromCanonical<
    typename std::unordered_map<K, V>::value_type>
result<std::unordered_map<K, V>> from_canonical(
    value const& v, tag_t<std::unordered_map<K, V>> ) {
  static std::string kFrameName = "std::unordered_map";
  // clang-format on
  auto maybe_lst = v.get_if<list>();
  if( !maybe_lst.has_value() )
    return error::build{ "std::unordered_map" }(
        "producing a std::unordered_map requires type list, "
        "instead found type {}.",
        type_name( v ) );
  list const&              lst = *maybe_lst;
  std::unordered_map<K, V> res;
  res.reserve( lst.size() );
  using value_type =
      typename std::unordered_map<K, V>::value_type;
  for( value const& elem : lst ) {
    CDR_UNWRAP_RETURN( val, from_canonical<value_type>( elem ) );
    if( res.contains( val.first ) ) {
      if constexpr( base::Show<K> )
        return error::build{ "std::unordered_map" }(
            "map contains duplicate key {}.", val.first );
      else
        return error::build{ "std::unordered_map" }(
            "map contains duplicate key." );
    }
    res.insert( val );
  }
  return res;
}

/****************************************************************
** unordered_set
*****************************************************************/
// TODO

} // namespace cdr
