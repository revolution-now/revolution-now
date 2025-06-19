/****************************************************************
**merge.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-18.
*
* Description: Algorithm for merging two cdr tables.
*
*****************************************************************/
#pragma once

// cdr
#include "repr.hpp"

namespace cdr {

/****************************************************************
** Right Join.
*****************************************************************/
// Does a "right join" on two tables or two lists. Keeps all
// values in the left table (list), but brings in any values from
// the right table (list) that are missing in the left table
// (list). Returns the number of fields that needed to be pulled
// in from the r table (list).
struct RightJoiner {
  void operator()( table& l, table const& r );
  void operator()( list& l, list const& r );
  void operator()( value& l, value const& r );
  void operator()( auto&, auto const& ) const;

  int l_missing = 0;
};

int right_join( auto& l, auto const& r ) {
  RightJoiner j;
  j( l, r );
  return j.l_missing;
}

} // namespace cdr
