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
#include "base/variant.hpp"

// C++ standard library
#include <concepts>
#include <string>
#include <unordered_map>
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
// The table keys must be strings and are **unordered**.

/****************************************************************
** null
*****************************************************************/
struct null_t {
  bool operator==( null_t const& ) const = default;
};

inline constexpr null_t null;

/****************************************************************
** table
*****************************************************************/
// This is the map type used to represent tables. We make this a
// template but limit the type to `value`. We can't refer to
// unordered_map<string, value> at this point because `value` is
// an incomplete type due to the cyclic dependency between
// `table` and `value`.
template<std::same_as<value> V>
using MapTo = std::unordered_map<std::string, V>;

// Writing this is a bit tricky because the table depends on
// `value`, but `value` is an incomplete type at this point, so
// we have to use some templates, wrappers, and indirection to
// make it work.
struct table {
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

  // Will create the value if it doesn't exist.
  value& operator[]( std::string const& key );

  base::maybe<value const&> operator[](
      std::string const& key ) const;

  size_t size() const;
  long   ssize() const;

  friend bool operator==( table const& lhs, table const& rhs );

 private:
  // This will inherit from MapTo<value>. We do this in order to
  // defer the mention of MapTo<value> which we can't mention at
  // this point because `value` is an incomplete type.
  struct TableImpl;

  // Need to use heap_value to add one level of indirection since
  // TableImpl is an incomplete type, which in turn is the case
  // because of the cyclic dependency (table -> value -> table).
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
struct list {
  list() = default;

  list( std::vector<value> const& v );

  list( std::vector<value>&& v );

  // Beware this one entails copying since elements in initial-
  // izer lists can't be moved from. Note that `value` is incom-
  // plete here, and indications are that the standard does not
  // explicitly allow initializer lists of incomplete types, but
  // in practice it seems fine, since that class basically just
  // holds a pointer to T.
  list( std::initializer_list<value> il );

  // This is UB if the index is out of bounds.
  value& operator[]( size_t idx );

  // This is UB if the index is out of bounds.
  value const& operator[]( size_t idx ) const;

  size_t size() const;
  long   ssize() const;

  bool operator==( list const& ) const = default;

 private:
  std::vector<value> o_;
};

/****************************************************************
** value
*****************************************************************/
// I believe the order of these matters since it might affect
// conversions and how the alternative is selected upon assign-
// ment. null should be first so that a default-constructed value
// object will default to it.
using value_base = base::variant< //
    null_t, double, int, bool, std::string, table, list>;

struct value : public value_base {
  using value_base::value_base;

  bool operator==( value const& ) const = default;

  value( std::vector<value> const& v ) : value( list( v ) ) {}
  value( std::vector<value>&& v )
    : value( list( std::move( v ) ) ) {}

  value( MapTo<value> const& m ) : value( table( m ) ) {}
  value( MapTo<value>&& m ) : value( table( std::move( m ) ) ) {}
};

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

/****************************************************************
** literals
*****************************************************************/
namespace literals {

inline value operator""_val( long double d ) {
  return value{ double( d ) };
}

inline value operator""_val( unsigned long long i ) {
  return value{ int( i ) };
}

} // namespace literals

} // namespace cdr