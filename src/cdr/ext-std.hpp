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
#include <deque>
#include <queue>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace cdr {

/****************************************************************
** string
*****************************************************************/
value to_canonical( converter& conv, std::string const& o,
                    tag_t<std::string> );

result<std::string> from_canonical( converter&   conv,
                                    value const& v,
                                    tag_t<std::string> );

/****************************************************************
** string_view
*****************************************************************/
value to_canonical( converter& conv, std::string_view const& o,
                    tag_t<std::string_view> );

// Don't implement this because it will probably be indicative of
// a dangling-string_view bug.
result<std::string_view> from_canonical(
    converter& conv, value const& v,
    tag_t<std::string_view> ) = delete;

/****************************************************************
** std::filesystem::path
*****************************************************************/
value to_canonical( converter& conv, fs::path const& o,
                    tag_t<fs::path> );

result<fs::path> from_canonical( converter& conv, value const& v,
                                 tag_t<fs::path> );

/****************************************************************
** std::chrono::seconds
*****************************************************************/
value to_canonical( converter&                  conv,
                    std::chrono::seconds const& o,
                    tag_t<std::chrono::seconds> );

result<std::chrono::seconds> from_canonical(
    converter& conv, value const& v,
    tag_t<std::chrono::seconds> );

/****************************************************************
** std::chrono::milliseconds
*****************************************************************/
value to_canonical( converter&                       conv,
                    std::chrono::milliseconds const& o,
                    tag_t<std::chrono::milliseconds> );

result<std::chrono::milliseconds> from_canonical(
    converter& conv, value const& v,
    tag_t<std::chrono::milliseconds> );

/****************************************************************
** std::pair
*****************************************************************/
template<ToCanonical Fst, ToCanonical Snd>
value to_canonical( converter&                 conv,
                    std::pair<Fst, Snd> const& o,
                    tag_t<std::pair<Fst, Snd>> ) {
  table tbl;
  conv.to_field( tbl, "key", o.first );
  conv.to_field( tbl, "val", o.second );
  return tbl;
}

template<FromCanonical Fst, FromCanonical Snd>
result<std::pair<Fst, Snd>> from_canonical(
    converter& conv, value const& v,
    tag_t<std::pair<Fst, Snd>> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<table>( v ) );
  std::unordered_set<std::string> used_keys;
  UNWRAP_RETURN( fst,
                 conv.from_field<Fst>( tbl, "key", used_keys ) );
  UNWRAP_RETURN( snd,
                 conv.from_field<Snd>( tbl, "val", used_keys ) );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
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
value to_canonical( converter& conv, R const& o, tag_t<R> ) {
  // clang-format on
  list res;
  if constexpr( std::ranges::sized_range<R> )
    res.reserve( o.size() );
  for( auto const& elem : o ) res.push_back( conv.to( elem ) );
  return res;
}

/****************************************************************
** std::vector
*****************************************************************/
// to_canonical will use the std::ranges::range overload.

template<FromCanonical T>
result<std::vector<T>> from_canonical( converter&   conv,
                                       value const& v,
                                       tag_t<std::vector<T>> ) {
  UNWRAP_RETURN( lst, conv.ensure_type<list>( v ) );
  std::vector<T> res;
  res.reserve( lst.size() );
  for( int idx = 0; idx < lst.ssize(); ++idx ) {
    UNWRAP_RETURN( val, conv.from_index<T>( lst, idx ) );
    res.push_back( std::move( val ) );
  }
  return res;
}

/****************************************************************
** std::queue
*****************************************************************/
template<ToCanonical T>
value to_canonical( converter& conv, std::queue<T> const& o,
                    tag_t<std::queue<T>> ) {
  std::vector<T> data;
  data.reserve( size_t( o.size() ) );
  auto m_copy = o;
  while( !m_copy.empty() ) {
    data.emplace_back( m_copy.front() );
    m_copy.pop();
  }
  return conv.to( data );
}

template<FromCanonical T>
result<std::queue<T>> from_canonical( converter&   conv,
                                      value const& v,
                                      tag_t<std::queue<T>> ) {
  UNWRAP_RETURN( data, conv.from<std::vector<T>>( v ) );
  std::queue<T> q;
  for( auto& e : data ) q.emplace( std::move( e ) );
  return result<std::queue<T>>( std::move( q ) );
}

/****************************************************************
** std::deque
*****************************************************************/
template<FromCanonical T>
result<std::deque<T>> from_canonical( converter&   conv,
                                      value const& v,
                                      tag_t<std::deque<T>> ) {
  UNWRAP_RETURN( data, conv.from<std::vector<T>>( v ) );
  std::deque<T> q;
  for( auto& e : data ) q.push_back( std::move( e ) );
  return result<std::deque<T>>( std::move( q ) );
}

/****************************************************************
** std::array
*****************************************************************/
// to_canonical will use the std::ranges::range overload.

template<FromCanonical T, size_t N>
result<std::array<T, N>> from_canonical(
    converter& conv, value const& v, tag_t<std::array<T, N>> ) {
  UNWRAP_RETURN( lst, conv.ensure_type<list>( v ) );
  HAS_VALUE_OR_RET( conv.ensure_list_size( lst, N ) );
  std::array<T, N> res;
  for( int idx = 0; idx < lst.ssize(); ++idx ) {
    UNWRAP_RETURN( val, conv.from_index<T>( lst, idx ) );
    res[idx] = std::move( val );
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
template<ToCanonical K, ToCanonical V>
value to_canonical( converter&                      conv,
                    std::unordered_map<K, V> const& o,
                    tag_t<std::unordered_map<K, V>> ) {
  // This will test once up front if the key type
  static bool const converts_to_string = [] {
    // The ToCanonical concept will guarantee that K is default
    // constructible. We then assume that if the default con-
    // structed value gets converted to a string then all values
    // of that type will too, which is always going to apply in
    // practice to types that make sense to use as keys. An ex-
    // ception to this would be e.g. a maybe<string> in that it
    // will convert to a string when it holds a value but not
    // otherwise, but that type is not suitable as a key anyway.
    return converter{}.to( K{} ).template holds<std::string>();
  }();
  // Note: here we don't use conv.to_field because map keys are
  // not really "fields" of a struct; e.g., we don't necessarily
  // want them to be omitted when they have default values, even
  // when that serialization mode is enabled.
  if( converts_to_string ) {
    table res;
    for( auto const& [k, v] : o ) {
      value key = conv.to( k );
      CHECK( key.holds<std::string>(),
             "key type {} converts to string, but not "
             "consistently, therefore it is not suitable to use "
             "as a type in a map.",
             type_name( key ) );
      res[std::move( key.as<std::string>() )] = conv.to( v );
    }
    return res;
  } else {
    // Since this is an unordered container and we are converting
    // it to a list, we must sort the keys first so that we get a
    // deterministic result.
    static_assert( std::totally_ordered<K> );
    using value_type =
        typename std::unordered_map<K, V>::value_type;
    std::vector<value_type const*> elem_ptrs;
    elem_ptrs.reserve( o.size() );
    for( value_type const& elem : o )
      elem_ptrs.push_back( &elem );
    std::sort( elem_ptrs.begin(), elem_ptrs.end(),
               []( value_type const* l, value_type const* r ) {
                 return l->first < r->first;
               } );
    list res;
    res.reserve( o.size() );
    for( value_type const* p : elem_ptrs )
      res.push_back( conv.to( *p ) );
    return res;
  }
}

namespace detail {

template<typename K, typename V>
result<std::unordered_map<K, V>>
unordered_map_from_canonical_list( converter&  conv,
                                   list const& lst ) {
  std::unordered_map<K, V> res;
  res.reserve( lst.size() );
  using value_type =
      typename std::unordered_map<K, V>::value_type;
  for( int idx = 0; idx < lst.ssize(); ++idx ) {
    UNWRAP_RETURN( val,
                   conv.from_index<value_type>( lst, idx ) );
    auto&       key = val.first;
    std::string key_str;
    if constexpr( base::Show<K> )
      key_str = base::to_str( key );
    else
      key_str = "<unformattable>";
    if( res.contains( key ) )
      return conv.err( "map contains duplicate key {}.",
                       key_str );
    res.insert( std::move( val ) );
  }
  return res;
}

// `K` must be a type convertible from a Cdr string, since the
// keys in a Cdr table are always strings.
template<typename K, typename V>
result<std::unordered_map<K, V>>
unordered_map_from_canonical_table( converter&   conv,
                                    table const& tbl ) {
  std::unordered_map<K, V> res;
  res.reserve( tbl.size() );
  // The keys are string types already.
  for( auto const& [k, v] : tbl ) {
    UNWRAP_RETURN( key, conv.from<K>( k ) );
    std::string key_str;
    if constexpr( base::Show<K> )
      key_str = base::to_str( key );
    else
      key_str = "<unformattable>";
    // There should never be a duplicate key even upon invalid
    // user input because `tbl` is backed by a real map.
    CHECK( !res.contains( key ) );
    UNWRAP_RETURN( val,
                   conv.from_field_no_tracking<V>( tbl, k ) );
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
    converter& conv, value const& v, tag_t<std::unordered_map<K, V>> ) {
  // clang-format on
  auto maybe_lst = v.get_if<list>();
  if( maybe_lst.has_value() )
    return detail::unordered_map_from_canonical_list<K, V>(
        conv, *maybe_lst );
  auto maybe_tbl = v.get_if<table>();
  if( maybe_tbl.has_value() )
    return detail::unordered_map_from_canonical_table<K, V>(
        conv, *maybe_tbl );
  return conv.err(
      "producing a std::unordered_map requires either a list of "
      "pair objects or a table with string keys; instead found "
      "type {}.",
      type_name( v ) );
}

/****************************************************************
** unordered_set
*****************************************************************/
template<ToCanonical T>
value to_canonical( converter&                   conv,
                    std::unordered_set<T> const& o,
                    tag_t<std::unordered_set<T>> ) {
  static_assert( std::totally_ordered<T> );
  // First we need to sort the elements.
  std::vector<T const*> elem_ptrs;
  elem_ptrs.reserve( o.size() );
  for( T const& elem : o ) elem_ptrs.push_back( &elem );
  std::sort( elem_ptrs.begin(), elem_ptrs.end(),
             []( T const* l, T const* r ) { return *l < *r; } );
  list res;
  for( T const* p : elem_ptrs ) res.push_back( conv.to( *p ) );
  return res;
}

template<FromCanonical T>
result<std::unordered_set<T>> from_canonical(
    converter& conv, value const& v,
    tag_t<std::unordered_set<T>> ) {
  UNWRAP_RETURN( lst, conv.ensure_type<list>( v ) );
  std::unordered_set<T> res;
  res.reserve( lst.size() );
  for( int idx = 0; idx < lst.ssize(); ++idx ) {
    UNWRAP_RETURN( val, conv.from_index<T>( lst, idx ) );
    // We don't complain on duplicate elements here.
    res.insert( val );
  }
  return res;
}

/****************************************************************
** std::unique_ptr
*****************************************************************/
template<ToCanonical T>
value to_canonical( converter& conv, std::unique_ptr<T> const& o,
                    tag_t<std::unique_ptr<T>> ) {
  if( o == nullptr ) return null;
  return conv.to( *o );
}

template<FromCanonical T>
result<std::unique_ptr<T>> from_canonical(
    converter& conv, value const& v,
    tag_t<std::unique_ptr<T>> ) {
  if( v == null ) return std::unique_ptr<T>{ nullptr };
  UNWRAP_RETURN( res, conv.from<T>( v ) );
  return std::make_unique<T>( std::move( res ) );
}

/****************************************************************
** std::variant
*****************************************************************/
// We don't want to attempt to convert a simple variant for which
// no other reflection information is known, since then we would
// be basically forced (if we want efficiency) to use an approach
// consisting of recording the active alternative index, which is
// not very robust serialization-wise or readability-wise. There
// will be a more constrained overload in the reflection library
// that will provide these overloads in the case that the variant
// alternatives all consist of unique reflected structs, and
// hopefully that one will be selected in the relevant cases
// since it is more constrained.
template<typename... Ts>
value to_canonical( converter&                 conv,
                    std::variant<Ts...> const& o,
                    tag_t<std::variant<Ts...>> ) = delete;

template<typename... Ts>
result<std::variant<Ts...>> from_canonical(
    converter& conv, value const& v,
    tag_t<std::variant<Ts...>> ) = delete;

} // namespace cdr
