/****************************************************************
**rds-util.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-11.
*
* Description: Utilities for the RDS compiler.
*
*****************************************************************/
#pragma once

// base
#include "base/function-ref.hpp"

// rdsc
#include "expr.hpp"

// {fmt}
#include "fmt/format.h"

// c++ standard library
#include <cstdlib>
#include <iostream>
#include <string_view>

using namespace std;

namespace rds {

template<typename... Args>
void error_no_exit_msg( string_view fmt, Args&&... args ) {
  cerr << "\033[31merror:\033[00m ";
  cerr << fmt::format( fmt::runtime( fmt ),
                       forward<Args>( args )... );
  cerr << "\n";
}

template<typename... Args>
void error_no_exit( string_view filename, int line, int col,
                    string_view fmt, Args&&... args ) {
  cerr << filename << ":" << line << ":" << col << ": ";
  error_no_exit_msg( fmt, forward<Args>( args )... );
}

template<typename... Args>
void error( string_view filename, int line, int col,
            string_view fmt, Args&&... args ) {
  error_no_exit( filename, line, col, fmt,
                 forward<Args>( args )... );
  exit( 1 );
}

template<typename... Args>
void error_msg( string_view fmt, Args&&... args ) {
  error_no_exit_msg( fmt, forward<Args>( args )... );
  exit( 1 );
}

// const version.
template<typename T>
void perform_on_item_type(
    expr::Rds const&                     rds,
    base::function_ref<void( T const& )> func ) {
  for( expr::Item const& item : rds.items ) {
    for( expr::Construct const& construct : item.constructs ) {
      std::visit( mp::overload{ [&]( T const& o ) { func( o ); },
                                []( auto const& ) {} },
                  construct );
    }
  }
}

// non-const version.
template<typename T>
void perform_on_item_type(
    expr::Rds& rds, base::function_ref<void( T& )> func ) {
  for( expr::Item& item : rds.items ) {
    for( expr::Construct& construct : item.constructs ) {
      std::visit( mp::overload{ [&]( T& o ) { func( o ); },
                                []( auto& ) {} },
                  construct );
    }
  }
}

} // namespace rds
