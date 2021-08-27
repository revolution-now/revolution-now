/****************************************************************
**ext-std.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-25.
*
* Description: Rcl extensions for standard library types.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "rcl/ext.hpp"

// base
#include "base/attributes.hpp"
#include "base/error.hpp"
#include "base/fs.hpp"
#include "base/maybe.hpp"

// C++ standard library
#include <chrono>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

// We don't put these in the std namespace because that is risky
// (could lead to future conflicts) and also it is not necessary,
// since the rcl namespace is one of the ones that will always be
// searched by ADL for these convert_to methods (because one of
// their parameters is the rcl::tag type).
namespace rcl {

// std::string
convert_err<std::string> convert_to( value const& v,
                                     tag<std::string> );

// std::string_view
convert_err<std::string_view> convert_to(
    value const& v ATTR_LIFETIMEBOUND, tag<std::string_view> );

// std::filesystem::path
convert_err<fs::path> convert_to( value const& v,
                                  tag<fs::path> );

// std::chrono::seconds
convert_err<std::chrono::seconds> convert_to(
    value const& v, tag<std::chrono::seconds> );

// std::pair
template<Convertible T, Convertible U>
convert_err<std::pair<T, U>> convert_to( value const& v,
                                         tag<std::pair<T, U>> ) {
  base::maybe<std::unique_ptr<table> const&> mtbl =
      v.get_if<std::unique_ptr<table>>();
  if( !mtbl )
    return error(
        fmt::format( "cannot produce a std::pair from type {}.",
                     name_of( type_of( v ) ) ) );
  DCHECK( *mtbl != nullptr );
  table const& tbl = **mtbl;
  if( !tbl.has_key( "key" ) || !tbl.has_key( "val" ) )
    return error(
        "table must have both a 'key' and 'val' field for "
        "conversion to std::pair." );
  UNWRAP_RETURN( key, convert_to<T>( tbl["key"] ) );
  UNWRAP_RETURN( val, convert_to<U>( tbl["val"] ) );
  return std::pair<T, U>{ key, val };
}

template<Convertible T>
convert_err<std::vector<T>> convert_to( value const& v,
                                        tag<std::vector<T>> ) {
  base::maybe<std::unique_ptr<list> const&> mlst =
      v.get_if<std::unique_ptr<list>>();
  if( !mlst )
    return error( fmt::format(
        "cannot produce a std::vector from type {}.",
        name_of( type_of( v ) ) ) );
  DCHECK( *mlst != nullptr );
  list const&    lst = **mlst;
  std::vector<T> res;
  res.reserve( lst.size() );
  for( value const& v : lst ) {
    UNWRAP_RETURN( elem, convert_to<T>( v ) );
    res.push_back( std::move( elem ) );
  }
  return res;
}

template<Convertible K, Convertible V>
convert_err<std::unordered_map<K, V>> convert_to(
    value const& v, tag<std::unordered_map<K, V>> ) {
  // We could just delegate to the converter for vector<-
  // pair<K,V>>, but doing it manually will produce better error
  // messages.
  base::maybe<std::unique_ptr<list> const&> mlst =
      v.get_if<std::unique_ptr<list>>();
  if( !mlst )
    return error( fmt::format(
        "cannot produce a std::unordered_map from type {}.",
        name_of( type_of( v ) ) ) );
  DCHECK( *mlst != nullptr );
  list const&              lst = **mlst;
  std::unordered_map<K, V> res;
  res.reserve( lst.size() );
  for( value const& v : lst ) {
    UNWRAP_RETURN( p, convert_to<std::pair<K, V>>( v ) );
    auto& [key, val] = p;
    if( res.contains( key ) )
      return error( fmt::format(
          "dictionary contains duplicate key `{}'.", key ) );
    res[std::move( key )] = std::move( val );
  }
  return res;
}

} // namespace rcl
