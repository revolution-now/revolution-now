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
#include "base/heap-value.hpp"
#include "base/to-str.hpp"
#include "base/variant.hpp"

// C++ standard library
#include <concepts>
#include <map>
#include <string>
#include <vector>

namespace cdr {

// Forward decls.
struct table;
struct list;
struct value;

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
** null
*****************************************************************/
struct null_t {
  bool operator==( null_t const& ) const = default;

  friend void to_str( null_t const& o, std::string& out,
                      base::ADL_t );
};

inline constexpr null_t null;

/****************************************************************
** table
*****************************************************************/
// This is the map type used to represent tables. We make this a
// template but limit the type to `value`. We can't refer to
// map<string, value> at this point because `value` is an incom-
// plete type due to the cyclic dependency between `table` and
// `value`.
//
// This alias probably can't be used when constructing maps using
// braced initialization because it probably won't be able to de-
// tect the type of V. Instead just use the underlying std::map
// for that.
//
// Use std::map instead of std::unordered_map so that we get a
// stable ordering of map elements, which is useful generally fo
// getting deterministic results when converting, serializing,
// and testing these things. Performance-wise, std::map should
// not be a problem (or at least any worse than
// std::unordered_map) for this use case, because 1)
// unordered_map does a heap allocation per node just like
// std::map, and 2) these Cdr tables are really meant to be
// short-lived and to be iterated over (a use case where std::map
// excels) as opposed to being long-lived and queried many times
// for specific keys (a use case where unordered_map would ex-
// cel). Hence, std::map might be more performant here than
// unordered_map, but no profiling was done.
template<std::same_as<value> V>
using MapTo = std::map<std::string, V>;

// Writing this is a bit tricky because the table depends on
// `value`, but `value` is an incomplete type at this point, so
// we have to use some templates, wrappers, and indirection to
// make it work.
struct table {
  using value_type = std::pair<std::string const, value>;

  table() = default;

  // This is a template to break the cyclic dependency.
  template<std::same_as<value> V>
  table( MapTo<V> const& m ) : o_( m ) {}

  // This is a template to break the cyclic dependency.
  template<std::same_as<value> V>
  table( MapTo<V>&& m ) : o_( std::move( m ) ) {}

  // Beware this one entails copying since elements in initial-
  // izer lists can't be moved from.
  //
  // The parameter of the initializer list must be the value type
  // of MapTo<value>, but we can't say that explicitly because we
  // can't write MapTo<value> at this point because `value` is
  // incomplete.
  //
  // Apparently std::pair is ok with having value be an incom-
  // plete type at least in this context. Not sure if this is
  // guaranteed by the standard. Luckily this works because, if
  // we make this a template like we've done for the above con-
  // structors, then the compiler can't seem to infer the tem-
  // plate parameter.
  table(
      std::initializer_list<std::pair<std::string const, value>>
          il );

  // Will create the value if it doesn't exist, and it will have
  // an initial value of null.
  value& operator[]( std::string const& key );

  base::maybe<value const&> operator[](
      std::string const& key ) const;

  template<typename K, typename V>
  auto emplace( K&& k, V&& v );

  void insert( value_type const& o );
  void insert( value_type&& o );

  bool contains( std::string const& key ) const;

  size_t size() const;
  long   ssize() const;
  bool   empty() const;

  bool operator==( table const& rhs ) const;

  auto begin() const;
  auto end() const;

  auto begin();
  auto end();

  friend void to_str( table const& o, std::string& out,
                      ::base::ADL_t );

 private:
  // This will inherit from MapTo<value>. We do this in order to
  // defer the mention of MapTo<value> which we can't mention at
  // this point because `value` is an incomplete type.
  struct TableImpl;

  // Need to use heap_value to add one level of indirection since
  // TableImpl is an incomplete type, which in turn is the case
  // because of the cyclic dependency (table -> value -> table).
  // heap_value is kind of like unique_ptr except it allows copy-
  // ing, in which case it makes a deep copy using a new heap al-
  // location.
  base::heap_value<TableImpl> o_;
};

/****************************************************************
** list
*****************************************************************/
// In this class, which depends on `value`, we don't have to jump
// through all of the same hoops that we did in the `table` class
// because `list` is implemented in terms of std::vector which is
// one of the only types in the standard lib that allows instan-
// tiating with an incomplete type.
//
// Originally we had `list` inheriting from vector so that we
// didn't have to forward all of its member functions, but that
// caused trouble because it seems that, even when inheriting
// privately, the concept-checking mechanism will check for
// satistfaction of a concept using the base class (at least in
// some cases) that that was causing issues with the to_str con-
// cepts in that it led to recursive template errors.
struct list {
  using value_type = value;

  list() = default;

  list( std::initializer_list<value> il );

  list( std::vector<value> const& v );

  list( std::vector<value>&& v );

  value&       operator[]( size_t idx ) { return o_[idx]; }
  value const& operator[]( size_t idx ) const { return o_[idx]; }

  auto begin() { return o_.begin(); }
  auto end() { return o_.end(); }

  auto begin() const { return o_.begin(); }
  auto end() const { return o_.end(); }

  template<typename... T>
  auto emplace_back( T&&... args ) {
    return o_.emplace_back( std::forward<T>( args )... );
  }

  auto push_back( value const& v ) { return o_.push_back( v ); }

  auto push_back( value&& v ) {
    return o_.push_back( std::move( v ) );
  }

  auto reserve( size_t elems ) { return o_.reserve( elems ); }

  bool empty() const { return o_.empty(); }

  bool operator==( list const& ) const = default;

  long ssize() const;

  auto size() const { return o_.size(); }

 private:
  std::vector<value> o_;
};

void to_str( list const& o, std::string& out, ::base::ADL_t );

/****************************************************************
** value
*****************************************************************/
using integer_type = long;

// The order of these matters since 1) affects conversions and
// how the alternative is selected upon assignment (I think), and
// 2) null should be first so that a default-constructed value
// object will default to it.
using value_base = base::variant<null_t, double, integer_type,
                                 bool, std::string, table, list>;

struct value : public value_base {
  using base = value_base;
  using base::base;

  bool operator==( value const& ) const = default;

  value( std::vector<value> const& v ) : value( list( v ) ) {}
  value( std::vector<value>&& v )
    : value( list( std::move( v ) ) ) {}

  value( MapTo<value> const& m ) : value( table( m ) ) {}
  value( MapTo<value>&& m ) : value( table( std::move( m ) ) ) {}

  value_base&       as_base() { return *this; }
  value_base const& as_base() const { return *this; }
};

void to_str( value const& o, std::string& out, ::base::ADL_t );

std::string_view type_name( value const& v );

/****************************************************************
** table map implementation
*****************************************************************/
// This needs to be defined after `value` is no longer an incom-
// plete type type.
struct table::TableImpl : public MapTo<value> {
  using base = MapTo<value>;
  using base::base;

  TableImpl( MapTo<value> const& m ) : base( m ) {}
  TableImpl( MapTo<value>&& m ) : base( std::move( m ) ) {}

  bool operator==( TableImpl const& ) const = default;
};

inline auto table::begin() { return o_->begin(); }

inline auto table::end() { return o_->end(); }

inline auto table::begin() const { return o_->begin(); }

inline auto table::end() const { return o_->end(); }

template<typename K, typename V>
auto table::emplace( K&& k, V&& v ) {
  return o_->emplace( std::forward<K>( k ),
                      std::forward<V>( v ) );
}

inline void table::insert( value_type const& o ) {
  o_->insert( o );
}

inline void table::insert( value_type&& o ) {
  o_->insert( std::move( o ) );
}

/****************************************************************
** literals
*****************************************************************/
namespace literals {

inline value operator""_val( long double d ) {
  return value{ double( d ) };
}

inline value operator""_val( unsigned long long i ) {
  return value{ integer_type( i ) };
}

namespace detail {

struct key_proxy {
  key_proxy( char const* key, unsigned long len )
    : key_( key, key + len ) {}

  std::pair<std::string const, value> operator=( value&& v ) {
    return { std::move( key_ ), std::move( v ) };
  }

  std::pair<std::string const, value> operator=(
      value const& v ) {
    return { std::move( key_ ), v };
  }

  std::string key_;
};

} // namespace detail

// This allows using syntax like table{ "one"_key=123 } syntax
// when manually constructing tables from initializer lists.
inline detail::key_proxy operator""_key( char const*   key,
                                         unsigned long len ) {
  return detail::key_proxy( key, len );
}

} // namespace literals

} // namespace cdr