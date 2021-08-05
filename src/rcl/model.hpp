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
#include "base/maybe.hpp"
#include "base/variant.hpp"

// C++ standard library
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/****************************************************************
** Document Model
*****************************************************************/
namespace rcl {

struct rcl_lang {};

struct table;
struct list;

struct string_val {
  string_val() = default;
  string_val( std::string_view s ) : val( s ) {}
  string_val( std::string s ) : val( std::move( s ) ) {}
  std::string val;
};

struct number {
  number() = default;
  number( int n ) : val( n ) {}
  number( double d ) : val( d ) {}
  std::variant<int, double> val;
};

struct boolean {
  boolean() = default;
  boolean( bool b_ ) : b( b_ ) {}
  bool b;
};

// The order of these in the variant matters, as parsing will be
// attempted in order, and the first one that succeeds will be
// chosen.
using value =
    base::variant<number, string_val, boolean,
                  std::unique_ptr<table>, std::unique_ptr<list>>;

struct key_val {
  key_val() = default;
  key_val( std::string k_, value v_ )
    : k( std::move( k_ ) ), v( std::move( v_ ) ) {}
  std::string k;
  value       v;
};

struct table {
  table() = default;
  table( std::vector<key_val>&& m )
    : members( std::move( m ) ) {}

  std::vector<key_val> members{};

  base::maybe<value&>       operator[]( std::string_view key );
  base::maybe<value const&> operator[](
      std::string_view key ) const;

  std::string pretty_print( std::string_view indent = "" ) const;
};

struct list {
  list() = default;
  list( std::vector<value>&& l ) : members( std::move( l ) ) {}
  std::vector<value> members;

  std::string pretty_print( std::string_view indent = "" ) const;
};

struct rawdoc {
  rawdoc() = default;
  rawdoc( table&& tbl_ ) : tbl( std::move( tbl_ ) ) {}
  table tbl;
};

struct doc {
  static base::expect<doc, std::string> create( rawdoc tbl );

  table tbl;

private:
  doc( table tbl_ ) : tbl( std::move( tbl_ ) ) {}
};

} // namespace rcl

DEFINE_FORMAT( rcl::table, "{}", o.pretty_print() );
DEFINE_FORMAT( rcl::boolean, "{}", o.b );
DEFINE_FORMAT( rcl::number, "{}", o.val );
DEFINE_FORMAT( rcl::list, "{}", o.pretty_print() );
DEFINE_FORMAT( rcl::string_val, "{}", o.val );
DEFINE_FORMAT( rcl::doc, "{}", o.tbl.pretty_print() );
