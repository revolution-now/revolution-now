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

#include "config.hpp"

// C++ standard library
#include <string>
#include <typeinfo>

namespace base {

// These will demangle a type or symbol (e.g. one returned from
// type_id(<type>).name() if the compiler supports it, otherwise
// will return the mangled version. MUST be NULL-terminated.
std::string demangle( char const* name );

// You need to include <typeinfo> in your module to call this.
template<typename T>
std::string const& demangled_typename() {
  static std::string const demangled = [] {
    return demangle( typeid( T ).name() );
  }();
  return demangled;
}

template<typename... Types>
std::string const& type_list_to_names() {
  static std::string const names = [] {
    std::string res;
    if constexpr( sizeof...( Types ) > 0 ) {
      std::string const joiner = ",";
      res = ( ( demangled_typename<Types>() + joiner ) + ... );
      if( res.size() > 0 )
        res.resize( res.size() - joiner.size() );
    }
    return res;
  }();
  return names;
}

} // namespace base
