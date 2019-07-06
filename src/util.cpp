/****************************************************************
**util.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description:
*
*****************************************************************/
#include "util.hpp"

#include "errors.hpp"

// C++ standard library.
#include <cstdlib>
#include <exception>
#include <memory>
#include <stdexcept>
#include <utility>

using namespace std;

namespace rn {

namespace {

Opt<string> environment_variable( char const* name ) {
  Opt<string> res;
  auto const* value = getenv( name );
  if( value != nullptr ) res = value;
  return res;
}

} // namespace

int round_up_to_nearest_int_multiple( double d, int m ) {
  if( d < 0.0 )
    return -round_down_to_nearest_int_multiple( -d, m );
  int floor = int( d );
  if( floor % m != 0 ) floor += m;
  return floor / m;
}

int round_down_to_nearest_int_multiple( double d, int m ) {
  if( d < 0.0 )
    return -round_up_to_nearest_int_multiple( -d, m );
  int floor = int( d );
  return floor / m;
}

// The below is a C++ name demangler that works when using
// certain compilers, see:
//   https://stackoverflow.com/questions/281818/
//       unmangling-the-result-of-stdtype-infoname

#ifdef __GNUG__
#  include <cxxabi.h>

string demangle( char const* name ) {
  int status{};

  unique_ptr<char, void ( * )( void* )> res{
      abi::__cxa_demangle( name, NULL, NULL, &status ),
      std::free};
  return ( status == 0 ) ? res.get() : name;
}

#else
// Does nothing if not g++/clang
string demangle( const char* name ) { return name; }
#endif

Opt<fs::path> user_home_folder() {
  return environment_variable( "HOME" );
}

} // namespace rn
