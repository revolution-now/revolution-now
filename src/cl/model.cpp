/****************************************************************
**model.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: Document model for cl (config language) files.
*
*****************************************************************/
#include "model.hpp"

// C++ standard library
#include <sstream>

using namespace std;

namespace cl {

struct value_printer {
  string_view indent;

  string operator()( int n ) { return fmt::to_string( n ); }

  string operator()( string_val const& s ) { return s.val; }

  string operator()( std::unique_ptr<table> const& tbl ) {
    return tbl->pretty_print( indent );
  }
};

string table::pretty_print( string_view indent ) const {
  bool          is_top_level = ( indent.size() == 0 );
  ostringstream oss;
  if( indent.size() > 0 ) oss << fmt::format( "{{\n" );

  size_t n = members.size();
  for( auto& [k, v] : members ) {
    oss << fmt::format(
        "{}{}: {}\n", indent, k,
        std::visit( value_printer{ string( indent ) + "  " },
                    v ) );
    if( is_top_level && n-- > 1 ) oss << "\n";
  }

  if( !is_top_level ) {
    indent.remove_suffix( 2 );
    oss << fmt::format( "{}}}", indent );
  }

  return oss.str();
}

} // namespace cl
