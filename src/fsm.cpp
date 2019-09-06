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
#include "fmt-helper.hpp"
#include "logging.hpp"

// C++ standard library
#include <string>
#include <variant>

using namespace std;

namespace rn {

namespace {

adt_rn_( ColorState,    //
         ( red ),       //
         ( light_red ), //
         ( dark_red )   //
);

adt_rn_( ColorEvent, //
         ( light ),  //
         ( dark )    //
);

fsm_transitions(
    // clang-format off
    Color
    ,((red,       light), ->, light_red)
    ,((red,       dark ), ->, dark_red)
    ,((light_red, dark ), ->, red)
    ,((dark_red,  light), ->, red)
    // clang-format on
);

fsm_class( Color ) {
  fsm_init( ColorState::red{} );

  fsm_transition( Color, dark_red, light, ->, red ) {
    (void)event;
    lg.debug( "going from `dark_red` to `red` via `light`." );
    return {};
  }
};

} // namespace

void test_fsm() {
  ColorFsm color;
  lg.info( "color state: {}", color.state() );

  color.send_event( ColorEvent::light{} );
  color.process_events();
  lg.info( "color state: {}", color.state() );

  color.send_event( ColorEvent::dark{} );
  color.process_events();
  lg.info( "color state: {}", color.state() );

  color.send_event( ColorEvent::dark{} );
  color.send_event( ColorEvent::light{} );
  color.process_events();
  lg.info( "color state: {}", color.state() );
}

} // namespace rn
