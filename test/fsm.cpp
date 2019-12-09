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

using namespace std;

namespace rn {
namespace {

/****************************************************************
** Color
*****************************************************************/
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

// clang-format off
fsm_transitions( Color
 ,(    (red,          light ),  ->   ,light_red
),(    (red,          dark  ),  ->   ,dark_red
),(    (light_red,    dark  ),  ->   ,red
),(    (dark_red,     light ),  ->   ,red
),(    (blue,         light ),  ->   ,light_blue
),(    (blue,         dark  ),  ->   ,dark_blue
),(    (light_blue,   dark  ),  ->   ,blue
),(    (dark_blue,    light ),  ->   ,blue
),(    (yellow,       light ),  ->   ,light_yellow
),(    (yellow,       dark  ),  ->   ,dark_yellow
),(    (light_yellow, dark  ),  ->   ,yellow
),(    (dark_yellow,  light ),  ->   ,yellow
),(    (red,          rotate),  ->   ,blue
),(    (blue,         rotate),  ->   ,yellow
),(    (yellow,       rotate),  ->   ,red
));
// clang-format on

fsm_class( Color ) { //
  fsm_init( ColorState::red{} );

  fsm_transition_( Color, blue, rotate, ->, yellow ) {
    return {};
  }
};

FSM_DEFINE_FORMAT_RN_( Color );

TEST_CASE( "[fsm] test color" ) {
  ColorFsm color;
  REQUIRE( color.state() == ColorState_t{ ColorState::red{} } );
  REQUIRE( color.holds<ColorState::red>() );
  REQUIRE( !color.holds<ColorState::yellow>() );

  color.send_event( ColorEvent::light{} );
  color.process_events();
  REQUIRE( color.state() ==
           ColorState_t{ ColorState::light_red{} } );
  REQUIRE( color.holds<ColorState::light_red>() );
  REQUIRE( !color.holds<ColorState::yellow>() );

  // Test push/pop.
  color.push( ColorState::blue{} );
  REQUIRE( color.holds<ColorState::light_red>() );
  color.process_events();
  REQUIRE( color.holds<ColorState::blue>() );
  color.push( ColorState::yellow{} );
  color.process_events();
  REQUIRE( color.holds<ColorState::yellow>() );
  color.pop();
  color.process_events();
  REQUIRE( color.holds<ColorState::blue>() );
  color.pop();
  color.process_events();
  REQUIRE( color.holds<ColorState::light_red>() );

  color.send_event( ColorEvent::dark{} );
  color.process_events();
  REQUIRE( color.state() == ColorState_t{ ColorState::red{} } );
  REQUIRE( color.holds<ColorState::red>() );
  REQUIRE( !color.holds<ColorState::light_red>() );

  color.send_event( ColorEvent::rotate{} );
  color.send_event( ColorEvent::rotate{} );
  color.send_event( ColorEvent::light{} );
  color.process_events();
  REQUIRE( color.state() ==
           ColorState_t{ ColorState::light_yellow{} } );
  REQUIRE( color.holds<ColorState::light_yellow>() );
  REQUIRE( !color.holds<ColorState::light_red>() );

  REQUIRE( fmt::format( "{}", color ) ==
           "ColorFsm{state=ColorState::light_yellow}" );

  SECTION( "throws" ) {
    color.send_event( ColorEvent::light{} );
    REQUIRE_THROWS_AS_RN( color.process_events() );
  }

  color.pop();
  REQUIRE_THROWS_AS_RN( color.process_events() );
}

/****************************************************************
** Templated Color
*****************************************************************/
adt_T_rn_( template( T, U ), //
           TColorState,      //
           ( red ),          //
           ( light_red,      //
             ( U, n ) ),     //
           ( dark_red ),     //
           ( blue ),         //
           ( light_blue ),   //
           ( dark_blue ),    //
           ( yellow ),       //
           ( light_yellow ), //
           ( dark_yellow )   //
);

adt_T_rn_( template( T, U ), //
           TColorEvent,      //
           ( light,          //
             ( U, n ) ),     //
           ( dark ),         //
           ( rotate )        //
);

// clang-format off
fsm_transitions_T( template( T, U ), TColor
 ,(    (red,          light ),  ->   ,light_red
),(    (red,          dark  ),  ->   ,dark_red
),(    (light_red,    dark  ),  ->   ,red
),(    (dark_red,     light ),  ->   ,red
),(    (blue,         light ),  ->   ,light_blue
),(    (blue,         dark  ),  ->   ,dark_blue
),(    (light_blue,   dark  ),  ->   ,blue
),(    (dark_blue,    light ),  ->   ,blue
),(    (yellow,       light ),  ->   ,light_yellow
),(    (yellow,       dark  ),  ->   ,dark_yellow
),(    (light_yellow, dark  ),  ->   ,yellow
),(    (dark_yellow,  light ),  ->   ,yellow
),(    (red,          rotate),  ->   ,blue
),(    (blue,         rotate),  ->   ,yellow
),(    (yellow,       rotate),  ->   ,red
));
// clang-format on

fsm_class_T( template( T, U ), TColor ) {
  fsm_init_T( template( T, U ), TColor,
              ( TColorState::red<T, U>{} ) );

  fsm_transition_T( template( T, U ), //
                    TColor, red, light, ->, light_red ) {
    return { event.n };
  }
};

FSM_DEFINE_FORMAT_T_RN_( template( T, U ), TColor );

TEST_CASE( "[fsm] test templated color" ) {
  TColorFsm<int, double> color;
  REQUIRE( color.state() ==
           TColorState_t<int, double>{
               TColorState::red<int, double>{} } );
  REQUIRE( color.holds<TColorState::red<int, double>>() );
  REQUIRE( !color.holds<TColorState::yellow<int, double>>() );

  color.send_event( TColorEvent::light<int, double>{} );
  color.process_events();
  REQUIRE( color.state() ==
           TColorState_t<int, double>{
               TColorState::light_red<int, double>{} } );
  REQUIRE( color.holds<TColorState::light_red<int, double>>() );
  REQUIRE( !color.holds<TColorState::yellow<int, double>>() );

  color.send_event( TColorEvent::dark<int, double>{} );
  color.process_events();
  REQUIRE( color.state() ==
           TColorState_t<int, double>{
               TColorState::red<int, double>{} } );
  REQUIRE( color.holds<TColorState::red<int, double>>() );
  REQUIRE( !color.holds<TColorState::light_red<int, double>>() );

  color.send_event( TColorEvent::rotate<int, double>{} );
  color.send_event( TColorEvent::rotate<int, double>{} );
  color.send_event( TColorEvent::light<int, double>{} );
  color.process_events();
  REQUIRE( color.state() ==
           TColorState_t<int, double>{
               TColorState::light_yellow<int, double>{} } );
  REQUIRE(
      color.holds<TColorState::light_yellow<int, double>>() );
  REQUIRE( !color.holds<TColorState::light_red<int, double>>() );

  REQUIRE( fmt::format( "{}", color ) ==
           "TColorFsm{state=TColorState::light_yellow<int,"
           "double>}" );

  SECTION( "throws" ) {
    color.send_event( TColorEvent::light<int, double>{} );
    REQUIRE_THROWS_AS_RN( color.process_events() );
  }
}

/****************************************************************
** Ball
*****************************************************************/
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

// clang-format off
fsm_transitions( Ball
 ,(    (none,     do_nothing    ),  ->   ,none
),(    (none,     start_spinning),  ->   ,spinning
),(    (none,     start_rolling ),  ->   ,rolling
),(    (none,     start_bouncing),  ->   ,bouncing
),(    (rolling,  stop_rolling  ),  ->   ,none
),(    (spinning, stop_spinning ),  ->   ,none
),(    (bouncing, stop_bouncing ),  ->   ,none
));
// clang-format on

fsm_class( Ball ) {
  fsm_init( BallState::none{} );

  fsm_transition( Ball, none, start_rolling, ->, rolling ) {
    (void)cur;
    return { event.speed };
  }

  fsm_transition( Ball, none, start_bouncing, ->, bouncing ) {
    (void)cur;
    return { event.height };
  }
};

FSM_DEFINE_FORMAT_RN_( Ball );

TEST_CASE( "[fsm] test ball" ) {
  BallFsm ball;
  REQUIRE( ball.state() == BallState_t{ BallState::none{} } );

  ball.send_event( BallEvent::start_rolling{ 5 } );
  ball.process_events();
  REQUIRE( ball.state() ==
           BallState_t{ BallState::rolling{ 5 } } );

  ball.send_event( BallEvent::stop_rolling{} );
  ball.process_events();
  REQUIRE( ball.state() == BallState_t{ BallState::none{} } );

  ball.send_event( BallEvent::start_spinning{} );
  ball.process_events();
  REQUIRE( ball.state() ==
           BallState_t{ BallState::spinning{} } );

  ball.send_event( BallEvent::stop_spinning{} );
  ball.process_events();
  REQUIRE( ball.state() == BallState_t{ BallState::none{} } );

  ball.send_event( BallEvent::start_bouncing{ 4 } );
  ball.process_events();
  REQUIRE( ball.state() ==
           BallState_t{ BallState::bouncing{ 4 } } );

  REQUIRE( fmt::format( "{}", ball ) ==
           "BallFsm{state=BallState::bouncing{height=4}}" );

  ball.send_event( BallEvent::stop_bouncing{} );
  ball.process_events();
  REQUIRE( ball.state() == BallState_t{ BallState::none{} } );

  ball.send_event( BallEvent::do_nothing{} );
  ball.process_events();
  REQUIRE( ball.state() == BallState_t{ BallState::none{} } );

  SECTION( "throws 1" ) {
    ball.send_event( BallEvent::stop_bouncing{} );
    REQUIRE_THROWS_AS_RN( ball.process_events() );
  }

  SECTION( "throws 2" ) {
    ball.send_event( BallEvent::start_bouncing{ 4 } );
    ball.send_event( BallEvent::stop_rolling{} );
    REQUIRE_THROWS_AS_RN( ball.process_events() );
  }
}

TEST_CASE( "[fsm] test pointer stability on push/pop" ) {
  BallFsm ball( BallState::rolling{ 5 } );
  REQUIRE( ball.state() ==
           BallState_t{ BallState::rolling{ 5 } } );

  REQUIRE( ball.pushed_states().size() == 1 );
  auto* ptr1 = &( *ball.pushed_states().begin() );
  REQUIRE( ptr1 != nullptr );

  for( int i = 0; i < 1000; ++i )
    ball.push( BallState::rolling{ 7 } );
  ball.process_events();
  REQUIRE( ball.state() ==
           BallState_t{ BallState::rolling{ 7 } } );

  // Popping these 1000 elements
  for( int i = 0; i < 1000; ++i ) ball.pop();
  ball.process_events();
  REQUIRE( ball.state() ==
           BallState_t{ BallState::rolling{ 5 } } );

  REQUIRE( ball.pushed_states().size() == 1 );
  auto* ptr2 = &( *ball.pushed_states().begin() );
  REQUIRE( ptr2 != nullptr );

  // Now finally the real test: the address of the original state
  // in memory should not have moved.
  REQUIRE( ptr1 == ptr2 );
}

} // namespace
} // namespace rn
