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
#include "base/variant.hpp"

// C++ standard library
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
// Note on map types: we are using a std::map here because it,
// unlike std::unordered_map, seems to allow declaration and de-
// fault construction with an incomplete value type (though not
// sure if this is guaranteed by the standard). If we're not al-
// lowed to use an incomplete type as the value type (as would be
// the case with std::unordered_map) then that really complicates
// the implementation below since we have to add an additional
// layer of indirection. Performance-wise, std::map should not be
// that much worse than std::unordered_map for this use case
// (theoretically; was not measured), since these CDR data struc-
// tures are not created to linger and be queried many times (in
// which case unordered_map might have the advantage); instead,
// they are constructed once, iterated over, and converted to
// something else. Moreover, the per-node heap allocations done
// by std::map would also be done by unordered map, since it is a
// node-based container.
struct table {
  using Map = std::map<std::string, value>;

  table() = default;

  table( Map const& m );

  table( Map&& m );

  // Beware this one entails copying since elements in initial-
  // izer lists can't be moved from. Apparently std::pair is ok
  // with having value be an incomplete type (?).
  table( std::initializer_list<Map::value_type> il );

  // Will create the value if it doesn't exist.
  value& operator[]( std::string const& key );

  base::maybe<value const&> operator[](
      std::string const& key ) const;

  size_t size() const;
  long   ssize() const;

  friend bool operator==( table const& lhs, table const& rhs );

 private:
  Map o_;
};

/****************************************************************
** list
*****************************************************************/
struct list {
  list() = default;

  list( std::vector<value> const& v );

  list( std::vector<value>&& v );

  // Beware this one entails copying since elements in initial-
  // izer lists can't be moved from.
  list( std::initializer_list<value> il );

  // This is UB if the index is out of bounds.
  value& operator[]( size_t idx );

  // This is UB if the index is out of bounds.
  value const& operator[]( size_t idx ) const;

  size_t size() const;
  long   ssize() const;

  bool operator==( list const& ) const = default;

 private:
  // vector is one of the containers that is specified by the
  // standard to support declaration with an incomplete type,
  // which is useful here to break the cyclic dependency of value
  // -> list -> value.
  std::vector<value> o_;
};

/****************************************************************
** value
*****************************************************************/
// I believe the order of these matters since it might affect
// conversions and how the alternative is selected upon assign-
// ment.
// clang-format off
using value_base = base::variant<
  // null should be first so that a default-constructed value ob-
  // ject will default to it.
  null_t,
  double,
  int,
  bool,
  std::string,
  table,
  list
>;
// clang-format on

struct value : public value_base {
  // It seems that we automatically inherit all of the methods of
  // the base (which we want) except for the special members, so
  // we need to explicitly pull those in.
  using value_base::value_base;

  bool operator==( value const& ) const = default;

  value( std::vector<value> const& v ) : value( list( v ) ) {}

  value( std::vector<value>&& v )
    : value( list( std::move( v ) ) ) {}

  value( table::Map const& m ) : value( table( m ) ) {}

  value( table::Map&& m ) : value( table( std::move( m ) ) ) {}
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