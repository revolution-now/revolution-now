/****************************************************************
**rnl-util.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-11.
*
* Description: Utilities for the RNL compiler.
*
*****************************************************************/
#pragma once

// base
#include "base/function-ref.hpp"

// rnlc
#include "expr.hpp"

// {fmt}
#include "fmt/format.h"

// c++ standard library
#include <cstdlib>
#include <iostream>
#include <string_view>

using namespace std;

namespace rnl {

template<typename... Args>
void error_no_exit_msg( string_view fmt, Args&&... args ) {
  cerr << "\033[31merror:\033[00m ";
  cerr << fmt::format( fmt, forward<Args>( args )... );
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

void perform_on_sumtypes(
    expr::Rnl*                                 rnl,
    base::function_ref<void( expr::Sumtype* )> func );

void perform_on_sumtypes(
    expr::Rnl const&                                 rnl,
    base::function_ref<void( expr::Sumtype const& )> func );

} // namespace rnl
