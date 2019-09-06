/****************************************************************
**fsm.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-05.
*
* Description: Unit tests for the fsm module.
*
*****************************************************************/
#include "testing.hpp"

// Revolution Now
#include "fsm.hpp"

// Must be last.
#include "catch-common.hpp"

namespace {

using namespace std;
using namespace rn;

/****************************************************************
** Color
*****************************************************************/
adt__( ColorState,       //
       ( red ),          //
       ( light_red ),    //
       ( dark_red ),     //
       ( blue ),         //
       ( light_blue ),   //
       ( dark_blue ),    //
       ( yellow ),       //
       ( light_yellow ), //
       ( dark_yellow )   //
);

adt__( ColorEvent, //
       ( light ),  //
       ( dark ),   //
       ( rotate )  //
);

fsm_transitions(
    // clang-format off
    Color
   ,((red,          light),  ->,  light_red   )
   ,((red,          dark ),  ->,  dark_red    )
   ,((light_red,    dark ),  ->,  red         )
   ,((dark_red,     light),  ->,  red         )
   ,((blue,         light),  ->,  light_blue  )
   ,((blue,         dark ),  ->,  dark_blue   )
   ,((light_blue,   dark ),  ->,  blue        )
   ,((dark_blue,    light),  ->,  blue        )
   ,((yellow,       light),  ->,  light_yellow)
   ,((yellow,       dark ),  ->,  dark_yellow )
   ,((light_yellow, dark ),  ->,  yellow      )
   ,((dark_yellow,  light),  ->,  yellow      )
   ,((red,          rotate), ->,  blue        )
   ,((blue,         rotate), ->,  yellow      )
   ,((yellow,       rotate), ->,  red         )
    // clang-format on
);

fsm_class( Color ) { //
  fsm_init( ColorState::red{} );

  fsm_transition( Color, blue, rotate, ->, yellow ) {
    (void)event;
    return {};
  }
};

TEST_CASE( "[fsm] test color" ) {
  ColorFsm color;
  REQUIRE( color.state().get() ==
           ColorState_t{ColorState::red{}} );

  color.send_event( ColorEvent::light{} );
  color.process_events();
  REQUIRE( color.state().get() ==
           ColorState_t{ColorState::light_red{}} );

  color.send_event( ColorEvent::dark{} );
  color.process_events();
  REQUIRE( color.state().get() ==
           ColorState_t{ColorState::red{}} );

  color.send_event( ColorEvent::rotate{} );
  color.send_event( ColorEvent::rotate{} );
  color.send_event( ColorEvent::light{} );
  color.process_events();
  REQUIRE( color.state().get() ==
           ColorState_t{ColorState::light_yellow{}} );

  SECTION( "throws" ) {
    color.send_event( ColorEvent::light{} );
    REQUIRE_THROWS_AS_RN( color.process_events() );
  }
}

/****************************************************************
** Ball
*****************************************************************/
adt__( BallState,           //
       ( none ),            //
       ( bouncing,          //
         ( int, height ) ), //
       ( rolling,           //
         ( int, speed ) ),  //
       ( spinning )         //
);

adt__( BallEvent,           //
       ( do_nothing ),      //
       ( start_bouncing,    //
         ( int, height ) ), //
       ( stop_bouncing ),   //
       ( start_rolling,     //
         ( int, speed ) ),  //
       ( stop_rolling ),    //
       ( start_spinning ),  //
       ( stop_spinning )    //
);

fsm_transitions(
    // clang-format off
    Ball
   ,((none,     do_nothing    ),  ->,  none    )
   ,((none,     start_spinning),  ->,  spinning)
   ,((none,     start_rolling ),  ->,  rolling )
   ,((none,     start_bouncing),  ->,  bouncing)
   ,((rolling,  stop_rolling  ),  ->,  none    )
   ,((spinning, stop_spinning ),  ->,  none    )
   ,((bouncing, stop_bouncing ),  ->,  none    )
    // clang-format on
);

fsm_class( Ball ) {
  fsm_init( BallState::none{} );

  fsm_transition( Ball, none, start_rolling, ->, rolling ) {
    return {event.speed};
  }

  fsm_transition( Ball, none, start_bouncing, ->, bouncing ) {
    return {event.height};
  }
};

TEST_CASE( "[fsm] test ball" ) {
  BallFsm ball;
  REQUIRE( ball.state().get() ==
           BallState_t{BallState::none{}} );

  ball.send_event( BallEvent::start_rolling{5} );
  ball.process_events();
  REQUIRE( ball.state().get() ==
           BallState_t{BallState::rolling{5}} );

  ball.send_event( BallEvent::stop_rolling{} );
  ball.process_events();
  REQUIRE( ball.state().get() ==
           BallState_t{BallState::none{}} );

  ball.send_event( BallEvent::start_spinning{} );
  ball.process_events();
  REQUIRE( ball.state().get() ==
           BallState_t{BallState::spinning{}} );

  ball.send_event( BallEvent::stop_spinning{} );
  ball.process_events();
  REQUIRE( ball.state().get() ==
           BallState_t{BallState::none{}} );

  ball.send_event( BallEvent::start_bouncing{4} );
  ball.process_events();
  REQUIRE( ball.state().get() ==
           BallState_t{BallState::bouncing{4}} );

  ball.send_event( BallEvent::stop_bouncing{} );
  ball.process_events();
  REQUIRE( ball.state().get() ==
           BallState_t{BallState::none{}} );

  ball.send_event( BallEvent::do_nothing{} );
  ball.process_events();
  REQUIRE( ball.state().get() ==
           BallState_t{BallState::none{}} );

  SECTION( "throws 1" ) {
    ball.send_event( BallEvent::stop_bouncing{} );
    REQUIRE_THROWS_AS_RN( ball.process_events() );
  }

  SECTION( "throws 2" ) {
    ball.send_event( BallEvent::start_bouncing{4} );
    ball.send_event( BallEvent::stop_rolling{} );
    REQUIRE_THROWS_AS_RN( ball.process_events() );
  }
}

} // namespace
