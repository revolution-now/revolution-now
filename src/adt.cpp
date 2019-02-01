/****************************************************************
**adt.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-18.
*
* Description: Abstract Data Types.
*
*****************************************************************/
//#include "adt.hpp"

// Revolution Now
#include "variant.hpp"

struct __marker_start {};

// ADT( Test,
//     ( case_a ),                         //
//     ( case_b ),                         //
//     ( case_c, ( int, x ), ( int, y ) ), //
//     ( case_d, ( double, f ) )           //
//)

struct __marker_end {};

namespace rn {

namespace {} // namespace

void harness() {
  // Test_t o;
  // Test_t o2;

  // if( o == o2 ) {}

  // auto matcher =
  //    scelta::match( []( Test::case_a ) {}, //
  //                   []( Test::case_b ) {}, //
  //                   []( Test::case_c c ) {
  //                     (void)c.x;
  //                     (void)c.y;
  //                   },
  //                   []( Test::case_d d ) { (void)d.f; } );
  // matcher( o );
}

} // namespace rn
