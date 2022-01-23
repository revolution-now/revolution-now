/****************************************************************
**repr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-17.
*
* Description: Canonical data model for holding any serializable
*              data in the game.
*
*****************************************************************/
#include "repr.hpp"

using namespace std;

namespace cdr {

/****************************************************************
** table
*****************************************************************/
table::table( Map const& m ) : o_( m ) {}

table::table( Map&& m ) : o_( std::move( m ) ) {}

size_t table::size() const { return o_.size(); }

long table::ssize() const { return long( o_.size() ); }

value& table::operator[]( string const& key ) { return o_[key]; }

base::maybe<value const&> table::operator[](
    string const& key ) const {
  auto it = o_.find( key );
  if( it == o_.end() ) return base::nothing;
  return it->second;
}

table::table( initializer_list<table::Map::value_type> il )
  : o_( il.begin(), il.end() ) {}

bool operator==( table const& lhs, table const& rhs ) {
  return lhs.o_ == rhs.o_;
}

/****************************************************************
** list
*****************************************************************/
list::list( vector<value> const& v ) : o_( v ) {}

list::list( vector<value>&& v ) : o_( std::move( v ) ) {}

size_t list::size() const { return o_.size(); }

long list::ssize() const { return long( o_.size() ); }

value& list::operator[]( size_t idx ) {
  DCHECK( idx < o_.size() );
  return o_[idx];
}

value const& list::operator[]( size_t idx ) const {
  DCHECK( idx < o_.size() );
  return o_[idx];
}

list::list( initializer_list<value> il )
  : o_( vector<value>( il.begin(), il.end() ) ) {}

} // namespace cdr
