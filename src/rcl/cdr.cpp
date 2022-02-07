/****************************************************************
**cdr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-07.
*
* Description: Conversion from Rcl model to Cdr model.
*
*****************************************************************/
#include "cdr.hpp"

// cdr
#include "cdr/converter.hpp"

using namespace std;

namespace rcl {

cdr::value to_canonical( cdr::converter&   conv,
                         rcl::value const& o,
                         cdr::tag_t<rcl::value> ) {
  struct visitor {
    visitor( cdr::converter& conv ) : conv_{ conv } {}

    cdr::value operator()( null_t ) const { return cdr::null; }
    cdr::value operator()( bool o ) const { return o; }
    cdr::value operator()( int o ) const {
      return static_cast<cdr::integer_type>( o );
    }
    cdr::value operator()( double o ) const { return o; }
    cdr::value operator()( string const& o ) const { return o; }

    cdr::value operator()( unique_ptr<table> const& o ) const {
      DCHECK( o != nullptr );
      cdr::table tbl;
      for( auto const& [k, v] : *o ) tbl[k] = conv_.to( v );
      return cdr::value( std::move( tbl ) );
    }

    cdr::value operator()( unique_ptr<list> const& o ) const {
      DCHECK( o != nullptr );
      cdr::list lst;
      for( value const& elem : *o )
        lst.push_back( conv_.to( elem ) );
      return cdr::value( std::move( lst ) );
    }

    cdr::converter& conv_;
  };
  return std::visit( visitor{ conv }, o );
}

cdr::result<rcl::value> from_canonical(
    cdr::converter& conv, cdr::value const& v,
    cdr::tag_t<rcl::value> ) {
  struct visitor {
    visitor( cdr::converter& conv ) : conv_{ conv } {}

    value operator()( cdr::null_t ) const { return null; }
    value operator()( bool o ) const { return o; }
    value operator()( cdr::integer_type o ) const {
      return static_cast<int>( o );
    }
    value operator()( double o ) const { return o; }
    value operator()( string const& o ) const { return o; }

    value operator()( cdr::table const& o ) const {
      vector<pair<string, value>> values;
      for( auto const& [k, v] : o ) {
        UNWRAP_CHECK( rcl_v, conv_.from<value>( v ) );
        values.push_back(
            pair<string, value>{ k, std::move( rcl_v ) } );
      }
      return value( make_unique<table>( std::move( values ) ) );
    }

    value operator()( cdr::list const& o ) const {
      vector<value> values;
      for( cdr::value const& elem : o ) {
        UNWRAP_CHECK( rcl_v, conv_.from<value>( elem ) );
        values.push_back( std::move( rcl_v ) );
      }
      return value( make_unique<list>( std::move( values ) ) );
    }

    cdr::converter& conv_;
  };
  return std::visit( visitor{ conv }, v.as_base() );
}

} // namespace rcl
