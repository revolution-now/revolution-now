/****************************************************************
**cc-specific.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-20.
*
* Description: Compiler-specific things.
*
*****************************************************************/
#include "cc-specific.hpp"

// C++ standard library
#include <memory>

// Compiler-specific includes.
#ifdef __GNUG__
#  include <cxxabi.h>
#endif

using namespace std;

namespace base {

/****************************************************************
** Public API
*****************************************************************/
// The below is a C++ name demangler that works when using
// certain compilers, see:
//   https://stackoverflow.com/questions/281818/
//       unmangling-the-result-of-stdtype-infoname
#ifdef __GNUG__
string demangle( char const* name ) {
  int status{};

  unique_ptr<char, void ( * )( void* )> res{
      abi::__cxa_demangle( name, nullptr, nullptr, &status ),
      std::free };
  return ( status == 0 ) ? res.get() : name;
}
#else
// Does nothing if not g++/clang
string demangle( const char* name ) { return name; }
#endif

} // namespace base
