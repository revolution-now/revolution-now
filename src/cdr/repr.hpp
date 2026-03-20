/****************************************************************
**repr.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-17.
*
* Description: Canonical data representation for holding any
*              serializable data in the game.
*
*****************************************************************/
#pragma once

// base
#include "base/maybe.hpp"
#include "base/to-str.hpp"
#include "base/variant.hpp"

// C++ standard library
#include <map>
#include <string>
#include <vector>

namespace cdr {

/****************************************************************
** Canonical Data Representation
*****************************************************************/
// This is a basic JSON-like data model for all data types in the
// game that must be either serializable or convertible to/from
// other formats (other than C++). Such data types must be con-
// vertible to and from this data representation format. It sup-
// ports:
//
//   - null
//   - floating point
//   - integer
//   - boolean
//   - string
//   - list
//   - table
//
// The table keys are always strings. Any map-like data with a
// key type other than string would be represented by e.g. a list
// of tables, where the tables has a key/value pair.
//
// Table keys are **unordered**.
//
// Must be deterministic: any C++ type that is converted to a Cdr
// list must result in a deterministic ordering (any ordering is
// fine, so long as it is not dependent on any factors other than
// the data contained in the type). For example, when converting
// an unordered_map or unordered_set to Cdr lists, the elements
// are sorted first before putting them into the list, that way
// we don't end up with a resulting order that depends on e.g.
// the particular hashing algorithm used by the container.

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct value;

/****************************************************************
** Aliases.
*****************************************************************/
using integer_type = int64_t;
using float_type   = double;

// Use std::map instead of std::unordered_map so that we get a
// stable ordering of map elements, which is useful generally fo
// getting deterministic results when converting, serializing,
// and testing these types. Performance-wise, std::map should not
// be a problem (or at least any worse than std::unordered_map)
// for this use case, because 1) unordered_map does a heap allo-
// cation per node just like std::map, and 2) these cdr tables
// are really meant to be short-lived and to be iterated over (a
// use case where std::map excels) as opposed to being long-lived
// and queried many times for specific keys (a use case where
// unordered_map would excel). Hence, std::map might be more per-
// formant here than unordered_map, but no profiling was done.
template<typename K, typename V>
using map_type = std::map<K, V>;

/****************************************************************
** null_t
*****************************************************************/
struct null_t {
  bool operator==( null_t const& ) const = default;

  friend void to_str( null_t const& o, std::string& out,
                      base::tag<null_t> );
};

inline constexpr null_t null;

/****************************************************************
** table
*****************************************************************/
struct table {
  using value_type = std::pair<std::string const, value>;

  table();
  ~table();

  table( table const& );
  table( table&& );
  table& operator=( table const& );
  table& operator=( table&& );

  table( map_type<std::string, value> const& m );
  table( map_type<std::string, value>&& m );

  // Beware this one entails copying since elements in initial-
  // izer lists can't be moved from.
  table( std::initializer_list<value_type> il );

  // Will create the value if it doesn't exist, and it will have
  // an initial value of null.
  value& operator[]( std::string const& key );

  base::maybe<value const&> operator[](
      std::string const& key ) const;

  template<typename K, typename V>
  auto emplace( K&& k, V&& v ) {
    return o_.emplace( std::forward<K>( k ),
                       std::forward<V>( v ) );
  }

  void insert( value_type const& o );
  void insert( value_type&& o );

  [[nodiscard]] bool contains( std::string const& key ) const;

  [[nodiscard]] size_t size() const;
  [[nodiscard]] long ssize() const;
  [[nodiscard]] bool empty() const;

  [[nodiscard]] bool operator==( table const& rhs ) const;

  map_type<std::string, value>::const_iterator begin() const;
  map_type<std::string, value>::const_iterator end() const;

  map_type<std::string, value>::iterator begin();
  map_type<std::string, value>::iterator end();

  friend void to_str( table const& o, std::string& out,
                      ::base::tag<table> );

 private:
  map_type<std::string, value> o_;
};

/****************************************************************
** list
*****************************************************************/
// Originally we had `list` inheriting from vector so that we
// didn't have to forward all of its member functions, but that
// caused trouble because it seems that, even when inheriting
// privately, the concept-checking mechanism will check for
// satistfaction of a concept using the base class (at least in
// some cases) that that was causing issues with the to_str con-
// cepts in that it led to recursive template errors.
struct list {
  using value_type = value;

  list();
  ~list();

  list( list const& );
  list( list&& );
  list& operator=( list const& );
  list& operator=( list&& );

  // Initially it was not allowed to put an incomplete type in an
  // initializer list, but indications are that that was recti-
  // fied in c++17.
  list( std::initializer_list<value> il );

  list( std::vector<value> const& v );

  list( std::vector<value>&& v );

  value& operator[]( size_t idx );
  value const& operator[]( size_t idx ) const;

  std::vector<value>::iterator begin();
  std::vector<value>::iterator end();

  std::vector<value>::const_iterator begin() const;
  std::vector<value>::const_iterator end() const;

  void resize( size_t n );

  template<typename... T>
  auto emplace_back( T&&... args ) {
    return o_.emplace_back( std::forward<T>( args )... );
  }

  void push_back( value const& v );

  void push_back( value&& v );

  void reserve( size_t elems );

  [[nodiscard]] bool empty() const;

  [[nodiscard]] bool operator==( list const& ) const;

  [[nodiscard]] long ssize() const;

  [[nodiscard]] size_t size() const;

  friend void to_str( list const& o, std::string& out,
                      base::tag<list> );

 private:
  std::vector<value> o_;
};

/****************************************************************
** value
*****************************************************************/
// The order of these matters since 1) affects conversions and
// how the alternative is selected upon assignment (I think), and
// 2) null should be first so that a default-constructed value
// object will default to it.
using value_base =
    base::variant<null_t, float_type, integer_type, bool,
                  std::string, table, list>;

struct value : public value_base {
  using base = value_base;
  using base::base;

  bool operator==( value const& ) const = default;

  // Will switch the value to a table if it is not already a
  // table. Then will create the key in the table if it doesn't
  // already exist.
  value& operator[]( std::string const& key );

  // Same as above but will not switch the value type or create
  // the key if it doesn't exist.
  ::base::maybe<value const&> operator[](
      std::string const& key ) const;

  // Same as above but for list indexing. NOTE: this will resize
  // the list if the idx is out of range.
  value& operator[]( size_t idx );

  ::base::maybe<value const&> operator[]( size_t idx ) const;

  value( std::vector<value> const& v ) : value( list( v ) ) {}
  value( std::vector<value>&& v )
    : value( list( std::move( v ) ) ) {}

  value( map_type<std::string, value> const& m )
    : value( table( m ) ) {}
  value( map_type<std::string, value>&& m )
    : value( table( std::move( m ) ) ) {}

  value_base& as_base() { return *this; }
  value_base const& as_base() const { return *this; }

  friend void to_str( value const& o, std::string& out,
                      ::base::tag<value> );
};

std::string_view type_name( value const& v );

/****************************************************************
** literals
*****************************************************************/
namespace literals {

inline value operator""_val( long double const d ) {
  return value{ float_type( d ) };
}

inline value operator""_val( unsigned long long const i ) {
  return value{ integer_type( i ) };
}

namespace detail {

struct key_proxy {
  key_proxy( char const* key, unsigned long len )
    : key_( key, key + len ) {}

  table::value_type operator=( value&& v ) {
    return { std::move( key_ ), std::move( v ) };
  }

  table::value_type operator=( value const& v ) {
    return { std::move( key_ ), v };
  }

  std::string key_;
};

} // namespace detail

// This allows using syntax like table{ "one"_key=123 } syntax
// when manually constructing tables from initializer lists.
inline detail::key_proxy operator""_key( char const* key,
                                         unsigned long len ) {
  return detail::key_proxy( key, len );
}

} // namespace literals

} // namespace cdr