/****************************************************************
**fsm.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-25.
*
* Description: Finite State Machine.
*
*****************************************************************/
#include "fsm.hpp"

// Revolution Now
#include "adt.hpp"
#include "fmt-helper.hpp"
#include "logging.hpp"

// C++ standard library
#include <string>
#include <variant>

using namespace std;

namespace rn {

namespace {

adt_rn_( BallState,           //
         ( none ),            //
         ( bouncing,          //
           ( int, height ) ), //
         ( rolling,           //
           ( int, speed ) ),  //
         ( spinning )         //
);

adt_rn_( BallEvent,           //
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

FSM_TRANSITIONS(
    // clang-format off
    Ball
   ,((none,     do_nothing    ),  /*->*/  none    )
   ,((none,     start_spinning),  /*->*/  spinning)
   ,((none,     start_rolling ),  /*->*/  rolling )
   ,((none,     start_bouncing),  /*->*/  bouncing)
   ,((rolling,  stop_rolling  ),  /*->*/  none    )
   ,((spinning, stop_spinning ),  /*->*/  none    )
   ,((bouncing, stop_bouncing ),  /*->*/  none    )
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

adt_rn_( ColorState,       //
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

adt_rn_( ColorEvent, //
         ( light ),  //
         ( dark ),   //
         ( rotate )  //
);

FSM_TRANSITIONS(
    // clang-format off
    Color
   ,((red,          light),  /*->*/  light_red   )
   ,((red,          dark ),  /*->*/  dark_red    )
   ,((light_red,    dark ),  /*->*/  red         )
   ,((dark_red,     light),  /*->*/  red         )
   ,((blue,         light),  /*->*/  light_blue  )
   ,((blue,         dark ),  /*->*/  dark_blue   )
   ,((light_blue,   dark ),  /*->*/  blue        )
   ,((dark_blue,    light),  /*->*/  blue        )
   ,((yellow,       light),  /*->*/  light_yellow)
   ,((yellow,       dark ),  /*->*/  dark_yellow )
   ,((light_yellow, dark ),  /*->*/  yellow      )
   ,((dark_yellow,  light),  /*->*/  yellow      )
   ,((red,          rotate), /*->*/  blue        )
   ,((blue,         rotate), /*->*/  yellow      )
   ,((yellow,       rotate), /*->*/  red         )
    // clang-format on
);

fsm_class( Color ) { //
  fsm_init( ColorState::red{} );

  fsm_transition( Color, blue, rotate, ->, yellow ) {
    (void)event;
    return {};
  }
};

} // namespace

void test_fsm() {
  BallFsm ball;
  lg.info( "ball state: {}", ball.state() );

  ball.send_event( BallEvent::start_rolling{5} );
  ball.process_events();
  lg.info( "ball state: {}", ball.state() );

  ball.send_event( BallEvent::stop_rolling{} );
  ball.process_events();
  lg.info( "ball state: {}", ball.state() );

  ball.send_event( BallEvent::start_spinning{} );
  ball.process_events();
  lg.info( "ball state: {}", ball.state() );

  ball.send_event( BallEvent::stop_spinning{} );
  ball.process_events();
  lg.info( "ball state: {}", ball.state() );

  ball.send_event( BallEvent::start_bouncing{4} );
  ball.process_events();
  lg.info( "ball state: {}", ball.state() );

  ball.send_event( BallEvent::stop_bouncing{} );
  ball.process_events();
  lg.info( "ball state: {}", ball.state() );

  ball.send_event( BallEvent::do_nothing{} );
  ball.process_events();
  lg.info( "ball state: {}", ball.state() );

  // Colors.

  ColorFsm color;
  lg.info( "color state: {}", color.state() );

  color.send_event( ColorEvent::light{} );
  color.process_events();
  lg.info( "color state: {}", color.state() );

  color.send_event( ColorEvent::dark{} );
  color.process_events();
  lg.info( "color state: {}", color.state() );

  color.send_event( ColorEvent::rotate{} );
  color.send_event( ColorEvent::rotate{} );
  color.send_event( ColorEvent::light{} );
  color.process_events();
  lg.info( "color state: {}", color.state() );
}

} // namespace rn
