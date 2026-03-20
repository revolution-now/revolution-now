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

#include <algorithm>

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace cdr {

namespace {

using ::base::maybe;
using ::base::nothing;

}

/****************************************************************
** null_t
*****************************************************************/
void to_str( null_t const&, std::string& out,
             base::tag<null_t> ) {
  out += "null";
}

/****************************************************************
** table
*****************************************************************/
table::table()  = default;
table::~table() = default;

table::table( table const& ) = default;
table::table( table&& )      = default;

table& table::operator=( table const& ) = default;
table& table::operator=( table&& )      = default;

// This is a template to break the cyclic dependency.
table::table( map_type<std::string, value> const& m )
  : o_( m ) {}

// This is a template to break the cyclic dependency.
table::table( map_type<std::string, value>&& m )
  : o_( std::move( m ) ) {}

table::table( std::initializer_list<value_type> const il )
  : o_( il.begin(), il.end() ) {}

size_t table::size() const { return o_.size(); }

long table::ssize() const { return long( o_.size() ); }

bool table::empty() const { return o_.empty(); }

map_type<std::string, value>::const_iterator table::begin()
    const {
  return o_.begin();
}
map_type<std::string, value>::const_iterator table::end() const {
  return o_.end();
}

map_type<std::string, value>::iterator table::begin() {
  return o_.begin();
}
map_type<std::string, value>::iterator table::end() {
  return o_.end();
}

value& table::operator[]( string const& key ) { return o_[key]; }

maybe<value const&> table::operator[](
    string const& key ) const {
  auto it = o_.find( key );
  if( it == o_.end() ) return nothing;
  return it->second;
}

bool table::operator==( table const& rhs ) const {
  return o_ == rhs.o_;
}

bool table::contains( string const& key ) const {
  return ( *this )[key].has_value();
}

void table::insert( value_type const& o ) { o_.insert( o ); }
void table::insert( value_type&& o ) {
  o_.insert( std::move( o ) );
}

void to_str( table const& o, std::string& out,
             base::tag<table> ) {
  out += '{';
  bool remove_comma = false;

  vector<pair<string, value>> pairs( o.begin(), o.end() );
  std::sort( pairs.begin(), pairs.end(),
             []( auto const& l, auto const& r ) {
               return l.first < r.first;
             } );

  for( auto const& [k, v] : pairs ) {
    base::to_str( k, out );
    out += '=';
    base::to_str( v, out );
    out += ',';
    remove_comma = true;
  }
  if( remove_comma ) out.resize( out.size() - 1 );
  out += '}';
}

/****************************************************************
** list
*****************************************************************/
list::list()  = default;
list::~list() = default;

list::list( list const& ) = default;
list::list( list&& )      = default;

list& list::operator=( list const& ) = default;
list& list::operator=( list&& )      = default;

list::list( initializer_list<value> const il ) : o_( il ) {}

std::vector<value>::iterator list::begin() { return o_.begin(); }

std::vector<value>::iterator list::end() { return o_.end(); }

std::vector<value>::const_iterator list::begin() const {
  return o_.begin();
}

std::vector<value>::const_iterator list::end() const {
  return o_.end();
}

list::list( vector<value> const& v ) : o_( v ) {}

list::list( vector<value>&& v ) : o_( std::move( v ) ) {}

value& list::operator[]( size_t const idx ) {
  CHECK_LT( idx, o_.size() );
  return o_[idx];
}

value const& list::operator[]( size_t const idx ) const {
  CHECK_LT( idx, o_.size() );
  return o_[idx];
}

void list::resize( size_t const n ) { o_.resize( n ); }

long list::ssize() const { return long( size() ); }

void list::reserve( size_t const elems ) { o_.reserve( elems ); }

size_t list::size() const { return o_.size(); }

bool list::empty() const { return o_.empty(); }

bool list::operator==( list const& ) const = default;

void list::push_back( value&& v ) {
  o_.push_back( std::move( v ) );
}

void list::push_back( value const& v ) { o_.push_back( v ); }

void to_str( list const& o, std::string& out, base::tag<list> ) {
  out += '[';
  bool remove_comma = false;
  for( auto const& elem : o ) {
    to_str( elem, out );
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
    string_view operator()( float_type ) const {
      return "floating";
    }
    string_view operator()( integer_type ) const {
      return "integer";
    }
    string_view operator()( bool ) const { return "boolean"; }
    string_view operator()( string ) const { return "string"; }
    string_view operator()( table ) const { return "table"; }
    string_view operator()( list ) const { return "list"; }
  };
  return visit( visitor{}, v.as_base() );
}

void to_str( value const& o, std::string& out,
             base::tag<value> ) {
  visit( [&]( auto const& alt ) { base::to_str( alt, out ); },
         o.as_base() );
}

value& value::operator[]( std::string const& key ) {
  if( !this->holds<table>() ) return this->emplace<table>()[key];
  return this->as<table>()[key];
}

maybe<value const&> value::operator[](
    std::string const& key ) const {
  if( !this->holds<table>() ) return nothing;
  return this->as<table>()[key];
}

value& value::operator[]( size_t const idx ) {
  if( !this->holds<list>() ) this->emplace<list>();
  list& l = this->get<list>();
  if( idx >= l.size() ) l.resize( idx + 1 );
  return l[idx];
}

maybe<value const&> value::operator[]( size_t const idx ) const {
  UNWRAP_RETURN_T( list const& l, this->get_if<list>() );
  if( idx >= l.size() ) return nothing;
  return l[idx];
}

} // namespace cdr
