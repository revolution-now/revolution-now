/****************************************************************
**model.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: Document model for cl (config language) files.
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
namespace cl {

struct cl_lang {};

struct table;
struct list;

struct string_val {
  string_val() = default;
  string_val( std::string s ) : val( std::move( s ) ) {}
  std::string val;
};

struct number {
  number() = default;
  number( int n ) : val( n ) {}
  number( double d ) : val( d ) {}
  std::variant<int, double> val;
};

using value =
    base::variant<number, string_val, std::unique_ptr<table>,
                  std::unique_ptr<list>>;

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

struct doc {
  static base::expect<doc, std::string> create( table tbl );

  table tbl;

private:
  doc( table tbl_ ) : tbl( std::move( tbl_ ) ) {}
};

} // namespace cl

DEFINE_FORMAT( cl::table, "{}", o.pretty_print() );
DEFINE_FORMAT( cl::list, "{}", o.pretty_print() );
DEFINE_FORMAT( cl::string_val, "{}", o.val );
DEFINE_FORMAT( cl::doc, "{}", o.tbl.pretty_print() );
