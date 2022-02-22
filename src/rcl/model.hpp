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

// cdr
#include "cdr/converter.hpp" // TODO: remove
#include "cdr/repr.hpp"

// base
#include "base/expect.hpp"

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
// Table Keys
// ==========
// Table keys are normally just identifiers, but they can actu-
// ally be arbitrary strings (containing spaces, backslashes,
// double or single quotes, and any weird characters). This is
// done by quoting (in double quotes) and escaping. For example,
// if you want a table key of:
//
//   I can't stop "quoting" and backs\ashing.
//
// You would write the key as:
//
//   "I can't stop \"quoting\" and backs\\ashing."
//
// Quoted sections of keys that are adjacent to non-quoted iden-
// tifiers will be joined with them, like in shell languages.
// For example, if you write:
//
//   "this"is a.weird."string"
//
// It will be translated to:
//
//   thisis {
//     a {
//       weird {
//         string {
//          ...
//         }
//       }
//     }
//   }
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
//
//    into:
//
//      a {
//        b {
//          c: 1
//        }
//      }
//
//    This step also parses any quoted/escaped table keys.
//
namespace rcl {

/****************************************************************
** Post-processing Routine
*****************************************************************/
// The default options are setup to be those required when pro-
// ducing Rcl via parsing text.
struct ProcessingOptions {
  bool run_key_parse  = true;
  bool unflatten_keys = true;
};

/****************************************************************
** doc
*****************************************************************/
// This structure cannot be mutable after creation because it re-
// quires post processing in order to maintain invariants.
struct doc {
  // This will not only create the doc, it will also run a recur-
  // sive post-processing over the table.
  static base::expect<doc> create(
      cdr::table&& tbl, ProcessingOptions const& opts );

  cdr::table const& top_tbl() const {
    return val_.get<cdr::table>();
  }
  cdr::value const& top_val() const { return val_; }

 private:
  doc( cdr::table&& tbl ) : val_( std::move( tbl ) ) {}

  cdr::value val_;
};

void to_str( doc const& o, std::string& out, base::ADL_t );

/****************************************************************
** Helpers for parsing/formatting.
*****************************************************************/
// Any logic used by more than one of the parser, model post
// processor, or formatter, should be put in this common location
// so that it can be consistent across those three modules.

// An table key that does not start with a double quote must
// start with a character that satisfies this predicate.
bool is_leading_identifier_char( char c );

// A character in an unquoted segment of a table key must satisfy
// this predicate if it is not a space or dot.
bool is_identifier_char( char c );

// A string value (i.e., not a table key) that is unquoted must
// contain only characters that satisfy this predicate.
//
// Note: this is not relevant for table keys, which have more
// strict rules for quoting.
bool is_forbidden_unquoted_str_char( char c );

// Same as above but for the first character of such a string.
bool is_forbidden_leading_unquoted_str_char( char c );

// This will escape any double quotes or backslashes in the
// string and put quotes around the result if necessary (i.e.,
// only if there are any non-identifier characters in the string
// or if the string starts with a non-identifier-start charac-
// ter). In other words, it will not put quotes around the result
// unless it has to.
std::string escape_and_quote_table_key( std::string const& k );

std::string escape_and_quote_string_val( std::string const& k );

} // namespace rcl
