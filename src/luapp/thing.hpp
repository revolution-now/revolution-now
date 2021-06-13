/****************************************************************
**thing.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-09.
*
* Description: C++ containers for Lua values/objects.
*
*****************************************************************/
#pragma once

// luapp
#include "function.hpp"
#include "indexer.hpp"
#include "ref.hpp"
#include "string.hpp"
#include "table.hpp"
#include "thread.hpp"
#include "types.hpp"
#include "userdata.hpp"

// base
#include "base/error.hpp"
#include "base/fmt.hpp"
#include "base/variant.hpp"

// C++ standard library
#include <string>

namespace lua {

/****************************************************************
** thing
*****************************************************************/
// nil_t should be first so that it is selected as the default.
using thing_base =
    base::variant<nil_t, boolean, lightuserdata, integer,
                  floating, rstring, table, rfunction, userdata,
                  rthread>;

struct thing : public thing_base {
  using Base = thing_base;

  using Base::Base;
  using Base::operator=;

  bool operator==( thing const& rhs ) const noexcept;

  bool operator==( std::string_view rhs ) const noexcept;

  template<typename T>
  // clang-format off
  requires( !std::is_same_v<thing, T> &&
             std::is_constructible_v<thing, T const&> )
  bool operator==( T const& rhs ) const noexcept {
    // clang-format on
    return ( *this ) == thing( rhs );
  }

  std::string tostring() const noexcept;

  // Cannot index a thing.  Cast first to a table or userdata:
  //
  //   thing th = ...;
  //   th.as<table>()["key"] = ...;
  //
  template<typename U>
  auto operator[]( U&& idx ) noexcept = delete;

  template<typename IndexT, typename Predecessor>
  thing( indexer<IndexT, Predecessor> const& idxr ) noexcept {
    *this = idxr;
  }

  template<typename IndexT, typename Predecessor>
  thing& operator=(
      indexer<IndexT, Predecessor> const& idxr ) noexcept {
    cthread L = idxr.this_cthread();
    push( L, idxr );
    *this = pop( L );
    return *this;
  }

  // Follows Lua's rules, where every value is true except for
  // false and nil.
  operator bool() const noexcept;

  type type() const noexcept;

  static thing pop( cthread L ) noexcept;
};

void push( cthread L, thing const& th );

/****************************************************************
** to_str
*****************************************************************/
void to_str( thing const& th, std::string& out );

} // namespace lua

/****************************************************************
** fmt
*****************************************************************/
TOSTR_TO_FMT( lua::thing );
