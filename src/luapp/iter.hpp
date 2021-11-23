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
#include "rtable.hpp"

// C++ standard library
#include <utility>

namespace lua {

/****************************************************************
** raw_table_iterator
*****************************************************************/
// NOTE: Iteration order of keys when using lua_next is not spec-
// ified, even for numeric indices. To iterate through the in-
// dices of an array in order, use a numerical-for loop, or C
// equivalent (i.e. don't use this in that case).
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
  table                            tbl_;
  base::maybe<std::pair<any, any>> curr_pair_;
};

raw_table_iterator begin( table tbl ) noexcept;
raw_table_iterator end( table tbl ) noexcept;

} // namespace lua
