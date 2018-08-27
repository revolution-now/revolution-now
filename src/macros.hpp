/****************************************************************
* macros.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: 
*
*****************************************************************/

#pragma once

/****************************************************************
* Macros
****************************************************************/
#pragma once

#include <cstdlib>

#define TO_STR1NG(x) #x
#define TO_STRING(x) TO_STR1NG(x)

#define STRING_JO1N(arg1, arg2) arg1 ## arg2
#define STRING_JOIN(arg1, arg2) STRING_JO1N(arg1, arg2)

#define DIE( msg ) { rn::die( __FILE__, __LINE__, msg ); }

#define ASSERT( a ) \
  { if( !(a) ) DIE( TO_STRING( a ) " is false but should not be" ) }

#define ASSERT_( a ) ASSERT( a, "" )
#define ERROR( a ) ASSERT( false, a )

// This can be used  to  execute  an  arbitrary  block of code at
// startup (when the binary is loaded). It  is  used  like  this:
// STARTUP() { util::log << "some code here"; }
#define STARTUP()                                              \
    struct STRING_JOIN( register_, __LINE__ ) {                \
        STRING_JOIN( register_, __LINE__ )();                  \
    }   STRING_JOIN( obj, __LINE__ );                          \
    STRING_JOIN( register_, __LINE__ )::                       \
        STRING_JOIN( register_, __LINE__ )()
