/****************************************************************
**adt.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-18.
*
* Description: Algebraic Data Types.
*
*****************************************************************/
#include "adt.hpp"

// Revolution Now
#include "fb.hpp"
#include "logging.hpp"
#include "macros.hpp"
#include "serial.hpp"

// Flatbuffers
#include "fb/testing_generated.h"

// base-util
#include "base-util/string.hpp"

using namespace std;

namespace rn {

adt_s_rn( MyAdt,            //
          ( none ),         //
          ( some,           //
            ( string, s ),  //
            ( int, y ) ),   //
          ( more,           //
            ( double, d ) ) //
);
NOTHROW_MOVE( MyAdt_t );

std::string_view remove_rn_ns( std::string_view sv ) {
  constexpr std::string_view rn_ = "rn::";
  if( util::starts_with( sv, rn_ ) )
    sv.remove_prefix( rn_.size() );
  return sv;
}

void test_adt() {
  using namespace ::rn::serial;
  MyAdt::none n;
  MyAdt::some s{"hello", 7};
  // ---
  auto s_n = serialize_to_json( n );
  lg.info( "s_n:\n{}", s_n );
  auto s_s = serialize_to_json( s );
  lg.info( "s_s:\n{}", s_s );
  // ---
  MyAdt::none d_n;
  MyAdt::some d_s;
  CHECK_XP( deserialize_from_json( "testing", s_n, &d_n ) );
  CHECK( n == d_n, "{} != {}", n, d_n );
  CHECK_XP( deserialize_from_json( "testing", s_s, &d_s ) );
  CHECK( s == d_s, "{} != {}", s, d_s );
}

} // namespace rn
