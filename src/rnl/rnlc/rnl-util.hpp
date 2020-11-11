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

// {fmt}
#include "fmt/format.h"

// c++ standard library
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace rnl {

template<typename... Args>
void error( std::string_view fmt, Args... args ) {
  std::cerr << "\033[31merror\033[00m: ";
  std::cerr << fmt::format( fmt, args... );
  std::cerr << "\n";
  std::exit( 1 );
}

} // namespace rnl
