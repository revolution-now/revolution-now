/****************************************************************
**pretty.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-17.
*
* Description: Pretty-printer for Lua data structures.
*
*****************************************************************/
#include "pretty.hpp"

// luapp
#include "any.hpp"
#include "iter.hpp"
#include "rtable.hpp"
#include "types.hpp"

// C++ standard library
#include <algorithm>
#include <ranges>
#include <sstream>

using namespace std;

namespace rg = std::ranges;

namespace lua {

namespace {

void pretty_print_impl( ostringstream& out, any const a,
                        string const indent = "" ) {
  switch( type_of( a ) ) {
    case type::nil: {
      out << "nil";
      break;
    }
    case type::boolean: {
      out << base::to_str( a );
      break;
    }
    case type::lightuserdata: {
      out << base::to_str( a );
      break;
    }
    case type::number: {
      out << base::to_str( a );
      break;
    }
    case type::string: {
      out << quoted( base::to_str( a ) );
      break;
    }
    case type::table: {
      table const tbl = a.as<table>();
      out << "{";
      out << "\n";
      string const new_indent = indent + "  ";
      vector<pair<any, any>> ordered( begin( tbl ), end( tbl ) );
      rg::sort( ordered, []( auto const& l, auto const& r ) {
        return l.first.template as<string>() <
               r.first.template as<string>();
      } );
      for( auto const& [k, v] : ordered ) {
        string const k_str = [&] {
          string res;
          if( type_of( k ) != lua::type::string )
            res = "[" + base::to_str( k ) + "]";
          else {
            res = base::to_str( k );
            res = res.empty()         ? ""
                  : isdigit( res[0] ) ? ( '"' + res + '"' )
                                      : res;
          }
          return res;
        }();
        out << new_indent << k_str << " = ";
        pretty_print_impl( out, v, new_indent );
        out << ",\n";
      }
      out << indent << "}";
      break;
    }
    case type::function: {
      out << base::to_str( a );
      break;
    }
    case type::userdata: {
      out << base::to_str( a );
      break;
    }
    case type::thread: {
      out << base::to_str( a );
      break;
    }
  }
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
string pretty_print( any const& a ) {
  ostringstream out;
  pretty_print_impl( out, a );
  return out.str();
}

} // namespace lua
