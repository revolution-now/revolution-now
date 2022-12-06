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

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace cdr {

/****************************************************************
** null_t
*****************************************************************/
void to_str( null_t const&, std::string& out, base::ADL_t ) {
  out += "null";
}

/****************************************************************
** table
*****************************************************************/
size_t table::size() const { return o_->size(); }

long table::ssize() const { return long( o_->size() ); }

bool table::empty() const { return o_->empty(); }

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

void to_str( table const& o, std::string& out, base::ADL_t ) {
  out += '{';
  bool remove_comma = false;

  vector<pair<string, value>> pairs( o.begin(), o.end() );
  sort( pairs.begin(), pairs.end(),
        []( auto const& l, auto const& r ) {
          return l.first < r.first;
        } );

  for( auto const& [k, v] : pairs ) {
    to_str( k, out, base::ADL_t{} );
    out += '=';
    to_str( v, out, base::ADL_t{} );
    out += ',';
    remove_comma = true;
  }
  if( remove_comma ) out.resize( out.size() - 1 );
  out += '}';
}

/****************************************************************
** list
*****************************************************************/
list::list( initializer_list<value> il ) : o_( il ) {}

list::list( vector<value> const& v ) : o_( v ) {}

list::list( vector<value>&& v ) : o_( std::move( v ) ) {}

value& list::operator[]( size_t idx ) { return o_[idx]; }

value const& list::operator[]( size_t idx ) const {
  return o_[idx];
}

long list::ssize() const { return long( size() ); }

void list::reserve( size_t elems ) { o_.reserve( elems ); }

size_t list::size() const { return o_.size(); }

void list::push_back( value&& v ) {
  o_.push_back( std::move( v ) );
}

void list::push_back( value const& v ) { o_.push_back( v ); }

void to_str( list const& o, std::string& out, base::ADL_t ) {
  out += '[';
  bool remove_comma = false;
  for( auto const& elem : o ) {
    to_str( elem, out, base::ADL_t{} );
    out += ',';
    remove_comma = true;
  }
  if( remove_comma ) out.resize( out.size() - 1 );
  out += ']';
}

/****************************************************************
** value
*****************************************************************/
string_view type_name( value const& v ) {
  struct visitor {
    string_view operator()( null_t ) const { return "null"; }
    string_view operator()( double ) const { return "floating"; }
    string_view operator()( integer_type ) const {
      return "integer";
    }
    string_view operator()( bool ) const { return "boolean"; }
    string_view operator()( string ) const { return "string"; }
    string_view operator()( table ) const { return "table"; }
    string_view operator()( list ) const { return "list"; }
  };
  return base::visit( visitor{}, v.as_base() );
}

void to_str( value const& o, std::string& out, base::ADL_t ) {
  base::visit(
      [&]( auto const& alt ) {
        to_str( alt, out, base::ADL_t{} );
      },
      o.as_base() );
}

} // namespace cdr
