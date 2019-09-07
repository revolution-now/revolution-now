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
#include "logging.hpp"
#include "macros.hpp"

// base-util
#include "base-util/string.hpp"

adt_T( rn,               //
       template( T, U ), //
       State,            //
       ( none ),         //
       ( starting,       //
         ( T, x ),       //
         ( int, y ) ),   //
       ( ending,         //
         ( int, x ),     //
         ( U, y ) )      //
);

namespace rn {

std::string_view remove_rn_ns( std::string_view sv ) {
  constexpr std::string_view rn_ = "rn::";
  if( util::starts_with( sv, rn_ ) )
    sv.remove_prefix( rn_.size() );
  return sv;
}

namespace {} // namespace

void test_adt() {
  using TestType = State_t<int, double>;
  ASSERT_NOTHROW_MOVING( TestType );

  TestType state = State::starting<int, double>{1, 2};
  lg.info( "state: {}", state );
  state = State::none<int, double>{};
  lg.info( "state: {}", state );
  state = State::ending<int, double>{3, 4.2};
  lg.info( "state: {}", state );
}

} // namespace rn
