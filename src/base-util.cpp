/****************************************************************
* base-util.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: 
*
*****************************************************************/

#include "base-util.hpp"

#include <iostream>
#include <utility>

namespace rn {

namespace {


  
} // namespace

void die( char const* file, int line, std::string_view msg ) {
  std::cerr << "error:" << file << ":" << line << ": " << msg << "\n";
  std::terminate();
}

} // namespace rn

