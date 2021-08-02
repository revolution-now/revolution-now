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
#include "base/fmt.hpp"
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

struct string_val {
  string_val( std::string_view s ) : val( s ) {}
  std::string val;
};

using value =
    base::variant<int, string_val, std::unique_ptr<table>>;

struct key_val {
  key_val( std::string_view k_, value v_ )
    : k( std::move( k_ ) ), v( std::move( v_ ) ) {}
  std::string k;
  value       v;
};

struct table {
  table() = default;
  table( std::vector<key_val>&& m )
    : members( std::move( m ) ) {}

  table( table const& ) = delete;
  table& operator=( table const& ) = delete;

  table( table&& ) = default;
  table& operator=( table&& ) = default;

  std::vector<key_val> members{};

  std::string pretty_print( std::string_view indent = "" ) const;
};

struct doc {
  doc( table t ) : tbl( std::move( t ) ) {}
  table tbl;
};

} // namespace cl

DEFINE_FORMAT( cl::table, "{}", o.pretty_print() );
DEFINE_FORMAT( cl::string_val, "{}", o.val );
DEFINE_FORMAT( cl::doc, "{}", o.tbl.pretty_print() );
