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
ADT( rn::input, state,
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

std::string_view remove_rn_ns( std::string_view sv ) {
  constexpr std::string_view rn_ = "rn::";
  if( sv.starts_with( rn_ ) ) sv.remove_prefix( rn_.size() );
  return sv;
}

namespace {} // namespace

void adt_test() {
  input::state_t o;
  input::state_t o2;

  if( o == o2 ) {}

  auto matcher =
      scelta::match( []( input::state::steady ) {},  //
                     []( input::state::stopped ) {}, //
                     []( input::state::paused p ) {  //
                       (void)p.percent;
                       (void)p.msg;
                       (void)p.y;
                     },
                     []( input::state::starting s ) { //
                       (void)s.x;
                       (void)s.y;
                     },
                     []( input::state::ending e ) { //
                       (void)e.f;
                     } );
  matcher( o );

  o = input::state::steady{};
  logger->info( "o: {}", o );
  o = input::state::paused{5.5, "hello", 8};
  logger->info( "o: {}", o );
  o = input::state::ending{2.0};
  logger->info( "o: {}", o );
}

} // namespace rn
