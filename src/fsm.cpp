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

using BallTransitionMap = FSM_TRANSITIONS(
    BallState,
    BallEvent
    // clang-format off
   ,((none,     do_nothing    ),  /*->*/  none    )
   ,((none,     start_spinning),  /*->*/  spinning)
   ,((none,     start_rolling ),  /*->*/  rolling )
   ,((none,     start_bouncing),  /*->*/  bouncing)
   ,((rolling,  stop_rolling  ),  /*->*/  none    )
   ,((spinning, stop_spinning ),  /*->*/  none    )
   ,((bouncing, stop_bouncing ),  /*->*/  none    )
    // clang-format on
);

struct BallFsm : public fsm<BallFsm, BallState_t, BallEvent_t,
                            BallTransitionMap> {
  using Parent::transition;

  BallState_t transition( BallState::none const&,
                          BallEvent::start_rolling const& event,
                          BallState::rolling const& ) {
    return BallState_t{BallState::rolling{event.speed}};
  }

  BallState_t transition( BallState::none const&,
                          BallEvent::start_bouncing const& event,
                          BallState::bouncing const& ) {
    return BallState_t{BallState::bouncing{event.height}};
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
}

} // namespace rn
