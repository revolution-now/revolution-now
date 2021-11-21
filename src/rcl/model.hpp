/****************************************************************
**model.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: Document model for rcl (config language) files.
*
*****************************************************************/
#pragma once

// base
#include "base/expect.hpp"
#include "base/fmt.hpp"
#include "base/valid.hpp"
#include "base/variant.hpp"

// C++ standard library
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

/****************************************************************
** Rcl Document Model
*****************************************************************/
// Syntax
// ======
//
// Rcl is very similar to UCL (see github.com/vstakhov/libucl)
// though lacks some of fancier features such as macros or lit-
// eral suffixes (k, ms, etc.).
//
// Sample config:
//
//   # Comment.
//
//   foo {
//     an_int:   5
//     a_double: 5.55
//     a_bool:   true
//   }
//
//   foo.bar.baz: 6  # nest tables using dot notation.
//   foo.baz = 6     # Can use '=' or ':' to assign values.
//
//   message: This is an unquoted string.
//   url: "http://domain.com/url" # some characters need quoting.
//
//   enabled:  "true" # this is a string!
//   disabled:  true  # this is a bool!
//
//   one {           # can omit '=' or ':' for tables.
//     two.three = [
//        1,
//        "hello",   # list elements can vary in type.
//        {
//          one.two.three = 3
//          one.two {
//            hello=1       # commas are optional
//            world="two"
//          }
//        },                # commas allowed on last element.
//     ]
//   }
//
//   # Table keys, despite being strings, are ordered according
//   # to the order they appear in the config file.  When
//   # iterating through keys, the 'before' key will always
//   # come before the 'after' key:
//   before: 9
//   after:  8
//
// The "top level" of the config is always a table. The above
// will be parsed, and then post processed.
//
// Post Processing
// ===============
//
// Once a config file written in the above syntax has been
// parsed, it will be post processed. There are three phases in
// the post processing:
//
// 1. Unflattening: This is applied recursively, and
//    transforms an input of the form:
//
//      a.b.c = 1
//      a.b.d = 2
//      a.c   = 3
//
//    into:
//
//      a {
//        b {
//          c: 1
//        }
//      }
//
//      a {
//        b {
//          d: 2
//        }
//      }
//
//      a {
//        c: 3
//      }
//
//    Note that if there were duplicate keys, those will be pre-
//    served. They are deduplicated in the next phase.
//
// 2. Deduplication of Table Keys.  The unflattening
//    operation, which is applied recursively, transforms
//    an input of the form:
//
//      a {
//        b {
//          c: 1
//        }
//      }
//
//      a {
//        b {
//          d: 2
//        }
//      }
//
//      a {
//        c: 3
//      }
//
//    into:
//
//      a {
//        b {
//          c: 1
//          d: 2
//        }
//        c: 3
//      }
//
//    In other words, it merges duplicate keys whose values are
//    tables. Any duplicate keys where at least one instance has
//    a non-table value will cause an error. Order is preserved
//    during this operation, so that the order that a key is en-
//    countered during iteration will be determined by where it
//    first appeared in the config, even if it was subsequently
//    merged with duplicate keys that occurred later in the con-
//    fig.
//
// 3. Adding table keys (which are now deduplicated) into the
//    unordered_map for fast access.
//
namespace rcl {

struct table;
struct list;

struct null_t {
  bool operator==( null_t const& ) const = default;
};
inline constexpr null_t null;

/****************************************************************
** value
*****************************************************************/
enum class type {
  null,
  boolean,
  integral,
  floating,
  string,
  table,
  list
};

// clang-format off
using value = base::variant<
  null_t,
  double,
  int,
  bool,
  std::string,
  std::unique_ptr<table>,
  std::unique_ptr<list>
>;
// clang-format on

struct type_visitor {
  type operator()( null_t ) const { return type::null; }
  type operator()( bool ) const { return type::boolean; }
  type operator()( int ) const { return type::integral; }
  type operator()( double ) const { return type::floating; }
  type operator()( std::string const& ) const {
    return type::string;
  }
  type operator()( std::unique_ptr<table> const& ) const {
    return type::table;
  }
  type operator()( std::unique_ptr<list> const& ) const {
    return type::list;
  }
};

type             type_of( value const& v );
std::string_view name_of( type t );

/****************************************************************
** table
*****************************************************************/
// This structure cannot be mutable after creation because it re-
// quires post processing in order to maintain invariants.
struct table {
  using value_type = std::pair<std::string, value>;

  // These constructors are only for the convenience of the
  // parsers; they will not maintain class invariants, so one
  // should not use a table object after construction from these
  // constructors. Only tables that are returned by the docu-
  // ment's factory method should be used, as those will have
  // their invariants satisfied due to the post processing that
  // will be applied to them after construction.
  table() = default;
  table( std::vector<value_type>&& kvs )
    : members_( std::move( kvs ) ) {}

  bool has_key( std::string_view key ) const;

  // Key must exist or check-fail!
  value const& operator[]( std::string_view key ) const;

  int size() const { return members_.size(); }

  // Keys in table, although strings, are ordered. No range
  // checks, element must exist or check-fail! Note that if you
  // just need the keys in order, you can iterate over them using
  // begin/end since they will be delivered in that order.
  value_type const& operator[]( int n ) const;

  std::string pretty_print( std::string_view indent = "" ) const;

  // Consumes this table.
  table unflatten() &&;

  // Consumes this table.
  base::expect<table, std::string> dedupe() &&;

  // This should be done after all other post processing has fin-
  // ished; it will create the mapping in the map_ member (which
  // is possible after keys are deduplicated) for fast access
  // subsequently. We don't do this in the constructor because it
  // would be wasteful to do it before post-processing, since it
  // would then have to be done again.
  void map_members() &;

  auto begin() const { return members_.begin(); }
  auto end() const { return members_.end(); }

 private:
  void unflatten_impl( std::string_view dotted, value&& v );

  friend base::valid_or<std::string> merge_values(
      std::string_view key, value& v_target, value&& v_source );

  // Store this way because 1) when parsing, there can be dupli-
  // cate keys (though they will be merged during post process-
  // ing), and 2) keys are ordered, despite being strings.
  std::vector<value_type> members_;

  // Note: the key/value of this map refers to values inside of
  // members_. That said, this structure is still safe to std::-
  // move because it is not self-referential, because the vector
  // elements are on the heap. Also for this reason, the members_
  // should NOT be modified after post-processing finishes.
  std::unordered_map<std::string_view, value*> map_;
};

/****************************************************************
** list
*****************************************************************/
// This structure cannot be mutable after creation because it re-
// quires post processing in order to maintain invariants.
struct list {
  // These constructors are only for the convenience of the
  // parsers; they will not maintain class invariants, so one
  // should not use a list object after construction from these
  // constructors. Only list that are returned by the document's
  // factory method should be used, as those will have their in-
  // variants satisfied due to the post processing that will be
  // applied to them after construction.
  list() = default;
  list( std::vector<value>&& vs )
    : members_( std::move( vs ) ) {}

  int size() const { return members_.size(); }

  // Will check-fail on invalid index!
  value const& operator[]( int n ) const;

  std::string pretty_print( std::string_view indent = "" ) const;

  // Consumes this list.
  list unflatten() &&;

  // Will apply deduplication processing to any elements that are
  // tables. Returns a new list with the same elements except
  // that any table elements will have their keys deduplicated.
  // This is only called as part of post processing after pars-
  // ing, you do not need to ever call this as a user of this
  // class.
  base::expect<list, std::string> dedupe_tables() &&;

  // Perform the member-mapping post-processing step on any ele-
  // ments that are tables.
  void map_members() &;

  auto begin() const { return members_.begin(); }
  auto end() const { return members_.end(); }

 private:
  std::vector<value> members_;
};

/****************************************************************
** Post-processing Routine
*****************************************************************/
// This should only be run on the top-level table, since it will
// be applied recursively. In practice, this is only really
// useful for unit tests, since otherwise the top-level table is
// placed into a doc, and the doc will run this post-processing.
base::expect<table, std::string> run_postprocessing(
    table&& tbl );

base::expect<list, std::string> run_postprocessing( list&& lst );

/****************************************************************
** doc
*****************************************************************/
// This structure cannot be mutable after creation because it re-
// quires post processing in order to maintain invariants.
struct doc {
  // This will not only create the doc, it will also run a recur-
  // sive post-processing over the table.
  static base::expect<doc, std::string> create( table&& tbl );

  std::string pretty_print( std::string_view indent = "" ) const;

  table const& top_tbl() const { return *tbl_; }
  value const& top_val() const { return val_; }

 private:
  doc( table&& tbl )
    : val_( std::make_unique<table>( std::move( tbl ) ) ) {
    tbl_ = val_.get_if<std::unique_ptr<table>>()->get();
  }

  value        val_;
  table const* tbl_ = nullptr;
};

/****************************************************************
** Helpers for Testing
*****************************************************************/
template<typename... Vs>
list make_list( Vs&&... vs ) {
  std::vector<value> v;
  ( v.push_back( std::forward<Vs>( vs ) ), ... );
  return list( std::move( v ) );
}

template<typename... Vs>
value make_list_val( Vs&&... vs ) {
  return value{
      std::make_unique<list>( make_list( FWD( vs )... ) ) };
}

template<typename... Kvs>
table make_table( Kvs&&... kvs ) {
  using KV = table::value_type;
  std::vector<KV> v;
  ( v.push_back( std::forward<Kvs>( kvs ) ), ... );
  return table( std::move( v ) );
}

template<typename... Kvs>
value make_table_val( Kvs&&... kvs ) {
  return value{
      std::make_unique<table>( make_table( FWD( kvs )... ) ) };
}

template<typename... Kvs>
base::expect<doc, std::string> make_doc( Kvs&&... kvs ) {
  using KV = table::value_type;
  std::vector<KV> v;
  ( v.push_back( std::forward<Kvs>( kvs ) ), ... );
  return doc::create( table( std::move( v ) ) );
}

} // namespace rcl

DEFINE_FORMAT( rcl::doc, "{}", o.pretty_print() );
DEFINE_FORMAT( rcl::list, "{}", o.pretty_print() );
DEFINE_FORMAT( rcl::table, "{}", o.pretty_print() );
