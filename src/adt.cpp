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
  MyAdt::more m{5.5};
  MyAdt_t     v = s;
  // ---
  auto s_n = serialize_to_json( n );
  lg.info( "s_n:\n{}", s_n );
  auto s_s = serialize_to_json( s );
  lg.info( "s_s:\n{}", s_s );
  auto s_m = serialize_to_json( m );
  lg.info( "s_m:\n{}", s_m );
  FBBuilder fbb;
  auto      v_offset = serialize<::fb::MyAdt_t>(
      fbb, v, ::rn::serial::rn_adl_tag{} );
  fbb.Finish( v_offset );
  auto blob = BinaryBlob::from_builder( std::move( fbb ) );
  auto s_v  = blob.template to_json<::fb::MyAdt_t>();
  lg.info( "s_v:\n{}", s_v );
  // ---
  MyAdt::none d_n;
  MyAdt::some d_s;
  MyAdt::more d_m;
  MyAdt_t     d_v;
  CHECK_XP( deserialize_from_json( "testing", s_n, &d_n ) );
  CHECK( n == d_n, "{} != {}", n, d_n );
  CHECK_XP( deserialize_from_json( "testing", s_s, &d_s ) );
  CHECK( s == d_s, "{} != {}", s, d_s );
  CHECK_XP( deserialize_from_json( "testing", s_m, &d_m ) );
  CHECK( m == d_m, "{} != {}", m, d_m );
  auto* root = flatbuffers::GetRoot<::fb::MyAdt_t>( blob.get() );
  CHECK_XP(
      deserialize( root, &d_v, ::rn::serial::rn_adl_tag{} ) );
  CHECK( v == d_v, "{} != {}", v, d_v );
  lg.info( "d_v:\n{}", d_v );
}

} // namespace rn
