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

/****************************************************************
** lua_iterator
*****************************************************************/
lua_iterator::lua_iterator( rfunction const next,
                            table const tbl, any const seed ) {
  advance( next, tbl, seed );
}

lua_iterator::value_type const& lua_iterator::operator*() const {
  CHECK( data_.has_value(),
         "attempt to dereference an invalid iterator." );
  return data_->value;
}

lua_iterator::value_type const* lua_iterator::operator->()
    const {
  CHECK( data_.has_value(),
         "attempt to dereference an invalid iterator." );
  return &data_->value;
}

bool lua_iterator::operator==( lua_iterator const& rhs ) const {
  if( data_.has_value() != rhs.data_.has_value() ) return false;
  if( !data_.has_value() ) return true;
  return data_->value.first == rhs.data_->value.first;
}

lua_iterator& lua_iterator::operator++() {
  CHECK( data_.has_value(),
         "attempt to iterate beyond the end of a table." );
  auto const& [next, tbl, value] = *data_;
  advance( next, tbl, value.first );
  return *this;
}

void lua_iterator::advance( rfunction const next,
                            table const tbl,
                            any const prev_key ) {
  cthread const L = tbl.this_cthread();
  c_api C( L );
  SCOPE_CHECK_STACK_UNCHANGED;
  push( L, next );
  push( L, tbl );
  push( L, prev_key );
  C.call( 2, 2 );
  // Order matters here.
  any const val( L, C.ref_registry() );
  any const key( L, C.ref_registry() );
  if( key == nil )
    data_.reset();
  else
    data_ = { .next = next, .tbl = tbl, .value = { key, val } };
}

lua_iterator begin( pairs const p ) noexcept {
  cthread const L = p.a.this_cthread();
  c_api C( L );
  SCOPE_CHECK_STACK_UNCHANGED;
  if( C.getglobal( "pairs" ) == type::nil )
    C.error( "pairs method not found among globals" );
  push( L, p.a );
  C.call( 1, 3 );
  // Order matters here.
  any const seed( L, C.ref_registry() );
  table const t( L, C.ref_registry() );
  rfunction const fn( L, C.ref_registry() );
  return lua_iterator( fn, t, seed );
}

lua_iterator end( pairs const ) noexcept {
  return lua_iterator();
}

/****************************************************************
** raw_table_iterator
*****************************************************************/
raw_table_iterator::raw_table_iterator( table t, bool is_end )
  : tbl_( t ) {
  if( is_end ) return;
  // This is a begin iterator, so we need to advance to the first
  // item, if any.
  cthread L = tbl_.this_cthread();
  c_api C( L );
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

raw_table_iterator::value_type const*
raw_table_iterator::operator->() const {
  CHECK( curr_pair_.has_value(),
         "attempt to dereference an invalid table iterator." );
  return &*curr_pair_;
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
  c_api C( L );
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

raw_table_iterator begin( table tbl ) noexcept {
  return raw_table_iterator( tbl, /*is_end=*/false );
}

raw_table_iterator end( table tbl ) noexcept {
  return raw_table_iterator( tbl, /*is_end=*/true );
}

} // namespace lua
