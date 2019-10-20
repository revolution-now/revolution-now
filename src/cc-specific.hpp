/****************************************************************
**cc-specific.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-20.
*
* Description: Compiler-specific things.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// C++ standard library
#include <string>

namespace rn {

// These will demangle a type or symbol (e.g. one returned from
// type_id(<type>).name() if the compiler supports it, otherwise
// will return the mangled version. MUST be NULL-terminated.
std::string demangle( char const* name );

// You need to include <typeinfo> in your module to call this.
template<typename T>
std::string demangled_typename() {
  return demangle( typeid( T ).name() );
}

template<typename... Types>
std::string type_list_to_names() {
  std::string joiner = ",";
  auto res = ( ( demangled_typename<Types>() + joiner ) + ... );
  if( res.size() > 0 ) res.resize( res.size() - joiner.size() );
  return res;
}

/****************************************************************
** Testing
*****************************************************************/
void test_cc_specific();

} // namespace rn
