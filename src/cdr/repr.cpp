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
size_t table::size() const { return o_->size(); }

long table::ssize() const { return long( o_->size() ); }

value& table::operator[]( string const& key ) {
  return o_.get()[key];
}

base::maybe<value const&> table::operator[](
    string const& key ) const {
  auto& impl = o_.get();
  auto  it   = impl.find( key );
  if( it == impl.end() ) return base::nothing;
  return it->second;
}

table::table( initializer_list<pair<string const, value>> il )
  : o_( il.begin(), il.end() ) {}

bool table::operator==( table const& rhs ) const {
  return o_ == rhs.o_;
}

bool table::contains( string const& key ) const {
  return ( *this )[key].has_value();
}

/****************************************************************
** list
*****************************************************************/
list::list( vector<value> const& v ) : base( v ) {}

list::list( vector<value>&& v ) : base( std::move( v ) ) {}

long list::ssize() const { return long( size() ); }

/****************************************************************
** value
*****************************************************************/
string_view type_name( value const& v ) {
  struct visitor {
    string_view operator()( null_t ) const { return "null"; }
    string_view operator()( double ) const { return "floating"; }
    string_view operator()( int ) const { return "integer"; }
    string_view operator()( bool ) const { return "boolean"; }
    string_view operator()( string ) const { return "string"; }
    string_view operator()( table ) const { return "table"; }
    string_view operator()( list ) const { return "list"; }
  };
  return std::visit( visitor{}, v.as_base() );
}

} // namespace cdr
