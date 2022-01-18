/****************************************************************
**model.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-17.
*
* Description: Generic data model for holding any serializable
*              data in the game.
*
*****************************************************************/
#include "model.hpp"

using namespace std;

namespace model {

/****************************************************************
** table
*****************************************************************/
table::table() : o_( base::heap_value<table_impl>() ) {}

size_t table::size() const { return o_->size(); }

int table::ssize() const { return int( o_->size() ); }

value& table::operator[]( string const& key ) {
  return o_.get()[key];
}

base::maybe<value const&> table::operator[](
    string const& key ) const {
  if( !o_.get().contains( key ) ) return base::nothing;
  return o_.get().find( key )->second;
}

/****************************************************************
** list
*****************************************************************/
list::list() : o_( base::heap_value<list_impl>() ) {}

size_t list::size() const { return o_->size(); }

int list::ssize() const { return int( o_->size() ); }

value& list::operator[]( size_t idx ) {
  DCHECK( idx < o_.get().size() );
  return o_.get()[idx];
}

value const& list::operator[]( size_t idx ) const {
  DCHECK( idx < o_.get().size() );
  return o_.get()[idx];
}

} // namespace model
