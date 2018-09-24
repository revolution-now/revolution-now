/****************************************************************
* macros.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: Preprocessor macros.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#include <cstdlib>
#include <iostream>

#define TO_STR1NG(x) #x
#define TO_STRING(x) TO_STR1NG(x)

#define STRING_JO1N(arg1, arg2) arg1 ## arg2
#define STRING_JOIN(arg1, arg2) STRING_JO1N(arg1, arg2)

#define DIE( msg ) { rn::die( __FILE__, __LINE__, msg ); }

// Use like this:
//
//   WARNING() << "some warning message " << xyz;
//
// It will append a newline for you.
#define WARNING()                                         \
  std::cerr <<  "WARNING:" << __FILE__ << ":" << __LINE__ \
            << ": "

#define LOG( a ) \
  std::cerr << "LOG:" << __FILE__ << ":" << __LINE__ \
            << ": " << a << "\n"

#define ASSERT( a ) \
  { if( !(a) ) DIE( TO_STRING( a ) " is false but should not be" ) }

// This takes care to only evaluate (b) once, since it may be
// e.g. a function call. This function will evaluate (b) which is
// expected to result in a std::optional. It will assign that
// std::optional by value to a local anonymous variable and will
// then inspect it to see if it has a value. If not, error is
// thrown. If it does have a value then another local reference
// variable will be created to reference the value inside the op-
// tional, so there should not be any unnecessary copies.
#define ASSIGN_ASSERT_OPT( a, b )                       \
  auto STRING_JOIN( __x, __LINE__ ) = b;                \
  if( !(STRING_JOIN( __x, __LINE__)) )                  \
    DIE( TO_STRING( b ) " is false but should not be" ) \
  auto& a = *STRING_JOIN( __x, __LINE__ )

// This takes care to only evaluate (b) once, since it may be
// e.g. a function call. This function will evaluate (b) which is
// expected to result in a value that can be tested for true'-
// ness, and where a "true" value is interpreted as success. Oth-
// erwise an error is thrown.
#define ASSIGN_ASSERT( a, b ) \
  auto a = b;                 \
  if( !(a) )                  \
    { DIE( TO_STRING( b ) " is false but should not be" ) }

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
