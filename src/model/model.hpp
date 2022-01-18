/****************************************************************
**model.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-17.
*
* Description: Generic data model for holding any serializable
*              data in the game.
*
*****************************************************************/
#pragma once

// base
#include "base/heap-value.hpp"
#include "base/refl.hpp"
#include "base/variant.hpp"

// C++ standard library
#include <concepts>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace model {

struct table;
struct list;
struct value;

/****************************************************************
** Data Model
*****************************************************************/
// This is a basic JSON-like data model for all serializable data
// in the game. Any data that must be serializable or convertible
// to/from other formats (other than C++) must be convertible to
// this data model format. It supports:
//
//   - null
//   - double
//   - int
//   - bool
//   - string
//   - list
//   - table
//
// The table keys must be strings.

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
struct table {
  template<typename V>
  table( std::unordered_map<std::string, V>&& m )
    : o_( std::move( m ) ) {}

  template<typename V>
  table( std::unordered_map<std::string, V> const& m )
    : o_( m ) {}

  table();

  value&                    operator[]( std::string const& key );
  base::maybe<value const&> operator[](
      std::string const& key ) const;

  size_t size() const;
  int    ssize() const;

  bool operator==( table const& ) const = default;

  struct table_impl;

 private:
  // Need to wrap in heap_value to avoid a recursive data struc-
  // ture, since this refers to `value` which in turn refers to
  // `table`.
  base::heap_value<table_impl> o_;
};

/****************************************************************
** list
*****************************************************************/
struct list {
  template<typename V>
  list( std::vector<V>&& v ) : o_( std::move( v ) ) {}

  template<typename V>
  list( std::vector<V> const& v ) : o_( v ) {}

  list();

  value&       operator[]( size_t idx );
  value const& operator[]( size_t idx ) const;

  size_t size() const;
  int    ssize() const;

  struct list_impl;

  bool operator==( list const& ) const = default;

 private:
  // Need to wrap in heap_value to avoid a recursive data struc-
  // ture, since this refers to `value` which in turn refers to
  // `list`.
  base::heap_value<list_impl> o_;
};

/****************************************************************
** value
*****************************************************************/
using value_base = base::variant<null_t, double, int, bool,
                                 std::string, table, list>;

struct value : public value_base {
  using value_base::value_base;

  bool operator==( value const& ) const = default;

  value( std::vector<value>&& v )
    : value( list( std::move( v ) ) ) {}

  value( std::vector<value> const& v ) : value( list( v ) ) {}

  value( std::unordered_map<std::string, value>&& m )
    : value( table( std::move( m ) ) ) {}

  value( std::unordered_map<std::string, value> const& m )
    : value( table( m ) ) {}
};

/****************************************************************
** table implementation.
*****************************************************************/
struct table::table_impl
  : public std::unordered_map<std::string, value> {
  using base = std::unordered_map<std::string, value>;

  template<typename... Args>
  table_impl( Args&&... args )
    : base( std::forward<Args>( args )... ) {}

  bool operator==( table_impl const& ) const = default;
};

/****************************************************************
** list implementation.
*****************************************************************/
struct list::list_impl : public std::vector<value> {
  using base = std::vector<value>;
  using base::base;

  template<typename... Args>
  list_impl( Args&&... args )
    : base( std::forward<Args>( args )... ) {}

  bool operator==( list_impl const& ) const = default;
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

} // namespace model