/****************************************************************
**iter.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-21.
*
* Description: Iterator over tables.
*
*****************************************************************/
#include "iter.hpp"

// luapp
#include "c-api.hpp"

// base
#include "base/error.hpp"
#include "base/scope-exit.hpp"

using namespace std;

namespace lua {

raw_table_iterator begin( table tbl ) noexcept {
  return raw_table_iterator( tbl, /*is_end=*/false );
}

raw_table_iterator end( table tbl ) noexcept {
  return raw_table_iterator( tbl, /*is_end=*/true );
}

#define SCOPE_CHECK_STACK_UNCHANGED         \
  int starting_stack_size = C.stack_size(); \
  SCOPE_EXIT( CHECK_EQ( C.stack_size(), starting_stack_size ) )

/****************************************************************
** raw_table_iterator
*****************************************************************/
raw_table_iterator::raw_table_iterator( table t, bool is_end )
  : tbl_( t ) {
  if( is_end ) return;
  // This is a begin iterator, so we need to advance to the first
  // item, if any.
  cthread L = tbl_.this_cthread();
  c_api   C( L );
  SCOPE_CHECK_STACK_UNCHANGED;
  push( L, tbl_ );
  push( L, nil );
  if( !C.next( /*table index=*/-2 ) ) {
    C.pop();
    return;
  }
  // Order matters here.
  any val( L, C.ref_registry() );
  any key( L, C.ref_registry() );
  curr_pair_ = pair{ key, val };
  C.pop();
}

raw_table_iterator::value_type const&
raw_table_iterator::operator*() const {
  CHECK( curr_pair_.has_value(),
         "attempt to dereference an invalid table iterator." );
  return *curr_pair_;
}

bool raw_table_iterator::operator==(
    raw_table_iterator const& rhs ) const {
  if( curr_pair_.has_value() != rhs.curr_pair_.has_value() )
    return false;
  if( !curr_pair_.has_value() ) return true;
  return curr_pair_->first == rhs.curr_pair_->first;
}

raw_table_iterator& raw_table_iterator::operator++() {
  CHECK( curr_pair_.has_value(),
         "attempt to iterate beyond the end of a table." );
  cthread L = tbl_.this_cthread();
  c_api   C( L );
  SCOPE_CHECK_STACK_UNCHANGED;
  push( L, tbl_ );
  push( L, curr_pair_->first );
  if( !C.next( /*table index=*/-2 ) ) {
    curr_pair_.reset();
    C.pop();
    return *this;
  }
  // Order matters here.
  any val( L, C.ref_registry() );
  any key( L, C.ref_registry() );
  curr_pair_ = pair{ key, val };
  C.pop();
  return *this;
}

} // namespace lua
