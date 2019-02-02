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
#include "adt.hpp"

// Revolution Now
#include "fmt-helper.hpp"
#include "logging.hpp"
#include "variant.hpp"

struct __marker_start {};
ADT( state,
     ( steady ),             //
     ( stopped ),            //
     ( paused,               //
       ( double, percent ),  //
       ( std::string, msg ), //
       ( int, y ) ),         //
     ( starting,             //
       ( int, x ),           //
       ( int, y ) ),         //
     ( ending,               //
       ( double, f )         //
       )                     //
)

struct __marker_end {};

namespace rn {

namespace {} // namespace

void adt_test() {
  state_t o;
  state_t o2;

  if( o == o2 ) {}

  auto matcher = scelta::match( []( state::steady ) {},  //
                                []( state::stopped ) {}, //
                                []( state::paused p ) {  //
                                  (void)p.percent;
                                  (void)p.msg;
                                  (void)p.y;
                                },
                                []( state::starting s ) { //
                                  (void)s.x;
                                  (void)s.y;
                                },
                                []( state::ending e ) { //
                                  (void)e.f;
                                } );
  matcher( o );

  o = state::steady{};
  logger->info( "o: {}", o );
  o = state::paused{5.5, "hello", 8};
  logger->info( "o: {}", o );
  o = state::ending{2.0};
  logger->info( "o: {}", o );
}

} // namespace rn
