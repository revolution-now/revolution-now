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

struct value;

struct key_val {
  std::string            k;
  std::unique_ptr<value> v;
};

struct table {
  table() = default;

  table( table const& ) = delete;
  table& operator=( table const& ) = delete;

  table( table&& ) = default;
  table& operator=( table&& ) = default;

  std::vector<key_val> members{};

  std::string pretty_print( std::string_view indent = "" ) const;
};

using value_base = base::variant<int, std::string, table>;

struct value : public value_base {
  using base_type = value_base;
  using base_type::base_type;

  value( value&& ) = default;
  value& operator=( value&& ) = default;

  value( base_type&& b ) : base_type( std::move( b ) ) {}

  base_type const& as_base() const {
    return static_cast<base_type const&>( *this );
  }

  std::string pretty_print( std::string_view indent = "" ) const;
};

struct doc {
  table tbl;
};

} // namespace cl

DEFINE_FORMAT( cl::value, "{}", o.pretty_print() );
DEFINE_FORMAT( cl::table, "{}", o.pretty_print() );
DEFINE_FORMAT( cl::doc, "{}", o.tbl.pretty_print() );
