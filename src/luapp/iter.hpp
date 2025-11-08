/****************************************************************
**iter.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-21.
*
* Description: Iterator over tables.
*
*****************************************************************/
#pragma once

// luapp
#include "any.hpp"
#include "rfunction.hpp"
#include "rtable.hpp"

// C++ standard library
#include <utility>

namespace lua {

/****************************************************************
** Lua-style table iteration.
*****************************************************************/
// This is what should be used by default to iterate because it
// will call the __pairs metamethod. You can use it like so:
//
//   for( auto const& [k, v] : lua::pairs( x ) )
//     ...
//
// If you omit the pairs() and just iterate over a table then you
// will get the raw table iteration further below, which can only
// iterate over keys that are explicitly there.
struct lua_iterator {
  using iterator_category = std::input_iterator_tag;
  using difference_type   = int;
  using value_type        = std::pair<any, any>;
  using pointer           = std::pair<any, any>*;
  using reference         = std::pair<any, any>&;

  lua_iterator( rfunction next, table tbl, any seed );

  lua_iterator() = default; // end iterator

  lua_iterator( lua_iterator const& ) = default;
  lua_iterator( lua_iterator&& )      = default;

  lua_iterator& operator=( lua_iterator const& ) = default;
  lua_iterator& operator=( lua_iterator&& )      = default;

  value_type const& operator*() const;

  value_type const* operator->() const;

  lua_iterator& operator++();

  lua_iterator operator++( int ) {
    auto res = *this;
    ++( *this );
    return res;
  }

  bool operator==( lua_iterator const& rhs ) const;

  bool operator!=( lua_iterator const& rhs ) const {
    return !( *this == rhs );
  }

 private:
  struct iter_data {
    rfunction next;
    table tbl;
    // NOTE: the third component of the iteration trio is in the
    // first component of the pair, namely the previous key.
    value_type value;
  };
  base::maybe<iter_data> data_;
};

struct pairs {
  pairs( any const a ) : a( a ) {}
  any const a;
};

lua_iterator begin( pairs p ) noexcept;
lua_iterator end( pairs p ) noexcept;

/****************************************************************
** Raw table iteration.
*****************************************************************/
// NOTE: Iteration order of keys when using lua_next is not spec-
// ified, even for numeric indices. To iterate through the in-
// dices of an array in order, use a numerical-for loop, or C
// equivalent (i.e. don't use this in that case).
//
// This is only for iterating among keys that are literally in
// the table. Normally though you should iterate using the
// lua-style iterator which will call pairs on the table and then
// iterate using Lua's function based iteration.
struct raw_table_iterator {
  using iterator_category = std::input_iterator_tag;
  using difference_type   = int;
  using value_type        = std::pair<any, any>;
  using pointer           = std::pair<any, any>*;
  using reference         = std::pair<any, any>&;

  raw_table_iterator( table t, bool is_end );

  raw_table_iterator( raw_table_iterator const& ) = default;
  raw_table_iterator( raw_table_iterator&& )      = default;

  raw_table_iterator& operator=( raw_table_iterator const& ) =
      default;
  raw_table_iterator& operator=( raw_table_iterator&& ) =
      default;

  value_type const& operator*() const;

  value_type const* operator->() const;

  raw_table_iterator& operator++();

  raw_table_iterator operator++( int ) {
    auto res = *this;
    ++( *this );
    return res;
  }

  bool operator==( raw_table_iterator const& rhs ) const;

  bool operator!=( raw_table_iterator const& rhs ) const {
    return !( *this == rhs );
  }

 private:
  table tbl_;
  base::maybe<std::pair<any, any>> curr_pair_;
};

raw_table_iterator begin( table tbl ) noexcept;
raw_table_iterator end( table tbl ) noexcept;

} // namespace lua
