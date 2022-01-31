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
#include "converter.hpp"
#include "ext.hpp"

// base
#include "base/fs.hpp"

// C++ standard library
#include <concepts>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

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
value to_canonical( fs::path const& o, tag_t<fs::path> );

result<fs::path> from_canonical( value const& v,
                                 tag_t<fs::path> );

/****************************************************************
** std::chrono::seconds
*****************************************************************/
value to_canonical( std::chrono::seconds const& o,
                    tag_t<std::chrono::seconds> );

result<std::chrono::seconds> from_canonical(
    value const& v, tag_t<std::chrono::seconds> );

/****************************************************************
** std::pair
*****************************************************************/
template<ToCanonical Fst, ToCanonical Snd>
value to_canonical( std::pair<Fst, Snd> const& o,
                    tag_t<std::pair<Fst, Snd>> ) {
  return table{ { "key", o.first }, { "val", o.second } };
}

template<FromCanonical Fst, FromCanonical Snd>
result<std::pair<Fst, Snd>> from_canonical(
    value const& v, tag_t<std::pair<Fst, Snd>> ) {
  converter conv( "std::pair" );
  auto      maybe_tbl = v.get_if<table>();
  if( !maybe_tbl.has_value() )
    return conv.err(
        "producing a std::pair requires type table, instead "
        "found type {}.",
        type_name( v ) );
  table const& tbl = *maybe_tbl;
  if( !tbl.contains( "key" ) || !tbl.contains( "val" ) )
    return conv.err(
        "table must have both a 'key' and 'val' field for "
        "conversion to std::pair." );
  UNWRAP_RETURN( fst, conv.from<Fst>( *tbl["key"] ) );
  UNWRAP_RETURN( snd, conv.from<Snd>( *tbl["val"] ) );
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
    res.push_back( to_canonical( elem ) );
  return res;
}

/****************************************************************
** std::vector
*****************************************************************/
// to_canonical will use the std::ranges::range overload.

template<FromCanonical T>
result<std::vector<T>> from_canonical( value const& v,
                                       tag_t<std::vector<T>> ) {
  converter conv( "std::vector" );
  auto      maybe_lst = v.get_if<list>();
  if( !maybe_lst.has_value() )
    return conv.err(
        "producing a std::vector requires type list, instead "
        "found type {}.",
        type_name( v ) );
  list const&    lst = *maybe_lst;
  std::vector<T> res;
  res.reserve( lst.size() );
  for( value const& elem : lst ) {
    UNWRAP_RETURN( val, conv.from<T>( elem ) );
    res.push_back( std::move( val ) );
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
  converter conv( "std::array" );
  auto      maybe_lst = v.get_if<list>();
  if( !maybe_lst.has_value() )
    return conv.err(
        "producing a std::array requires type list, instead "
        "found type {}.",
        type_name( v ) );
  list const& lst = *maybe_lst;
  if( lst.size() != N )
    return conv.err(
        "expected list of size {} for producing std::array of "
        "that same size, instead found size {}.",
        N, lst.size() );
  std::array<T, N> res;
  size_t           idx = 0;
  for( value const& elem : lst ) {
    UNWRAP_RETURN( val, conv.from<T>( elem ) );
    res[idx++] = std::move( val );
  }
  return res;
}

/****************************************************************
** unordered_map
*****************************************************************/
// This type is implemented a bit differently than the others,
// because an unordered_map where the keys are string-like is
// treated specially in that it can be converted directly to/from
// a Cdr table (whose keys must always be strings), whereas
// unordered_maps with any other key type must be converted to a
// list of pairs.
// clang-format off
template<typename K, typename V>
requires ToCanonical<
    typename std::unordered_map<K, V>::value_type>
value to_canonical( std::unordered_map<K, V> const& o,
                    tag_t<std::unordered_map<K, V>> ) {
  // clang-format on
  if constexpr( std::is_constructible_v<std::string, K> ) {
    table res;
    res.reserve( o.size() );
    for( auto const& [k, v] : o )
      res[std::string( k )] = to_canonical( v );
    return res;
  } else {
    list res;
    res.reserve( o.size() );
    for( auto const& elem : o )
      res.push_back( to_canonical( elem ) );
    return res;
  }
}

// Put this here to fix a weird clang-format issue.
inline constexpr int fix_clang_format = 0;

namespace detail {

template<typename K, typename V>
result<std::unordered_map<K, V>>
unordered_map_from_canonical_list( list const& lst ) {
  converter                conv( "std::unordered_map" );
  std::unordered_map<K, V> res;
  res.reserve( lst.size() );
  using value_type =
      typename std::unordered_map<K, V>::value_type;
  for( value const& elem : lst ) {
    UNWRAP_RETURN( val, conv.from<value_type>( elem ) );
    if( res.contains( val.first ) ) {
      if constexpr( base::Show<K> )
        return conv.err( "map contains duplicate key {}.",
                         val.first );
      else
        return conv.err( "map contains duplicate key." );
    }
    res.insert( std::move( val ) );
  }
  return res;
}

// `K` must be a type convertible from a Cdr string, since the
// keys in a Cdr table are always keys.
template<typename K, typename V>
result<std::unordered_map<K, V>>
unordered_map_from_canonical_table( table const& tbl ) {
  converter                conv( "std::unordered_map" );
  std::unordered_map<K, V> res;
  res.reserve( tbl.size() );
  // The keys are string types already.
  for( auto const& [k, v] : tbl ) {
    UNWRAP_RETURN( key, conv.from<K>( k ) );
    if( res.contains( key ) ) {
      if constexpr( base::Show<K> )
        return conv.err( "map contains duplicate key {}.", key );
      else
        return conv.err( "map contains duplicate key." );
    }
    UNWRAP_RETURN( val, conv.from<V>( v ) );
    res[std::move( key )] = std::move( val );
  }
  return res;
}

} // namespace detail

// clang-format off
template<typename K, typename V>
requires FromCanonical<
    typename std::unordered_map<K, V>::value_type>
result<std::unordered_map<K, V>> from_canonical(
    value const& v, tag_t<std::unordered_map<K, V>> ) {
  converter conv( "std::unordered_map" );
  // clang-format on
  auto maybe_lst = v.get_if<list>();
  if( maybe_lst.has_value() )
    return detail::unordered_map_from_canonical_list<K, V>(
        *maybe_lst );
  auto maybe_tbl = v.get_if<table>();
  if( maybe_tbl.has_value() )
    return detail::unordered_map_from_canonical_table<K, V>(
        *maybe_tbl );
  return conv.err(
      "producing a std::unordered_map requires either a list of "
      "pair objects or a table with string keys; instead found "
      "type {}.",
      type_name( v ) );
}

/****************************************************************
** unordered_set
*****************************************************************/
// to_canonical will use the std::ranges::range overload.

template<FromCanonical T>
result<std::unordered_set<T>> from_canonical(
    value const& v, tag_t<std::unordered_set<T>> ) {
  converter conv( "std::unordered_set" );
  auto      maybe_lst = v.get_if<list>();
  if( !maybe_lst.has_value() )
    return conv.err(
        "producing a std::unordered_set requires type list, "
        "instead found type {}.",
        type_name( v ) );
  list const&           lst = *maybe_lst;
  std::unordered_set<T> res;
  res.reserve( lst.size() );
  for( value const& elem : lst ) {
    UNWRAP_RETURN( val, conv.from<T>( elem ) );
    // We don't complain on duplicate elements here.
    res.insert( val );
  }
  return res;
}

/****************************************************************
** std::unique_ptr
*****************************************************************/
template<ToCanonical T>
value to_canonical( std::unique_ptr<T> const& o,
                    tag_t<std::unique_ptr<T>> ) {
  if( o == nullptr ) return null;
  return to_canonical( *o );
}

template<FromCanonical T>
result<std::unique_ptr<T>> from_canonical(
    value const& v, tag_t<std::unique_ptr<T>> ) {
  converter conv( "std::unique_ptr" );
  if( v == null ) return std::unique_ptr<T>{ nullptr };
  UNWRAP_RETURN( res, conv.from<T>( v ) );
  return std::make_unique<T>( std::move( res ) );
}

} // namespace cdr
