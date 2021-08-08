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
// The "top level" of the config is always a table.  The above
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

/****************************************************************
** value
*****************************************************************/
// clang-format off
using value = base::variant<
  double,
  int,
  bool,
  std::string,
  std::unique_ptr<table>,
  std::unique_ptr<list>
>;
// clang-format on

// If the two values are both tables then they will be merged, in
// the sense that all of the source table's keys (and values)
// will be moved into the target table. Any duplicate keys that
// result in the target table will be handled by recursively ap-
// plying the deduplication/merge logic.
//
// If one or both of the values are not tables then this function
// returns an error. We don't check-fail in that case (or try to
// enforce it with types by making this function accept table pa-
// rameters only) because that situation can result from an in-
// valid config file supplied by the user, and so we need to be
// able to receive that sort of invalid input and handle it by
// producing a proper error message for the user.
base::valid_or<std::string> merge_values( std::string_view key,
                                          value&  v_target,
                                          value&& v_source );

/****************************************************************
** table
*****************************************************************/
// This structure cannot be mutable after creation because it re-
// quires post processing in order to maintain invariants.
struct table {
  struct key_val {
    std::string k;
    value       v;
  };

  // The constructors will not do any post processing, and thus a
  // table object is not necessarily fit to be used after con-
  // struction. These constructors are used just to create inter-
  // mediate tables during parsing. A table should only be used
  // by way of a `doc` object, which will do post processing
  // during creation.
  table() = default;
  table( std::vector<key_val>&& kvs )
    : members_( std::move( kvs ) ) {}

  bool has_key( std::string_view key ) const;

  // Key must exist or check-fail!
  value const& operator[]( std::string_view key ) const;

  int size() const { return members_.size(); }

  // Keys in table, although strings, are ordered. No range
  // checks, element must exist or check-fail!
  value const& operator[]( int n ) const;

  std::string pretty_print( std::string_view indent = "" ) const;

  // Consumes the old table.
  static table unflatten( table&& old );

  // Consumes the old table.
  static base::expect<table, std::string> dedupe( table&& old );

  auto begin() const { return members_.begin(); }
  auto end() const { return members_.begin(); }

private:
  void unflatten_impl( std::string_view dotted, value&& v );

  friend base::valid_or<std::string> merge_values(
      std::string_view key, value& v_target, value&& v_source );

  // This should be done after all other post processing has fin-
  // ished; it will create the mapping in the map_ member (which
  // is possible after keys are deduplicated) for fast access
  // subsequently. We don't do this in the constructor because it
  // would be wasteful to do it before post-processing, since it
  // would then have to be done again.
  friend void map_members( table* tbl );

  // Store this way because 1) when parsing, there can be dupli-
  // cate keys (though they will be merged during post process-
  // ing), and 2) keys are ordered, despite being strings.
  std::vector<key_val> members_;
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
  // The constructors will not do any post processing, and thus a
  // list object is not necessarily fit to be used after con-
  // struction. These constructors are used just to create inter-
  // mediate lists during parsing. A list should only be used by
  // way of a `doc` object, which will do post processing during
  // creation.
  list() = default;
  list( std::vector<value>&& vs )
    : members_( std::move( vs ) ) {}

  int          size() const { return members_.size(); }
  value const& operator[]( int n ) const;

  std::string pretty_print( std::string_view indent = "" ) const;

  // Consumes the old list. Returns a new list with the same ele-
  // ments except that any table elements will be unflattened.
  // This is only called as part of post processing after pars-
  // ing, you do not need to ever call this as a user of this
  // class.
  static list unflatten( list&& old );

  // Will apply deduplication processing to any elements that are
  // tables. Returns a new list with the same elements except
  // that any table elements will have their keys deduplicated.
  // This is only called as part of post processing after pars-
  // ing, you do not need to ever call this as a user of this
  // class.
  static base::expect<list, std::string> dedupe_tables(
      list&& old );

  auto begin() const { return members_.begin(); }
  auto end() const { return members_.begin(); }

private:
  std::vector<value> members_;
};

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

  table const& top() const { return tbl_; }

private:
  doc( table&& tbl ) : tbl_( std::move( tbl ) ) {}

  table tbl_;
};

} // namespace rcl

DEFINE_FORMAT( rcl::doc, "{}", o.pretty_print() );
DEFINE_FORMAT( rcl::list, "{}", o.pretty_print() );
DEFINE_FORMAT( rcl::table, "{}", o.pretty_print() );
