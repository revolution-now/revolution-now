/****************************************************************
**auto-pad-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-22.
*
* Description: Unit tests for the auto-pad module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/auto-pad.hpp"

// Testing
// #include "test/mocks/render/itextometer.hpp"

// Revolution Now
#include "src/views.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;
using namespace rn::ui;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[auto-pad] autopad" ) {
  // Construct a grid of checkboxes:
  //
  //   ------------------------------
  //   |                            |
  //   |            off             |
  //   |                            |
  //   ------------------------------
  //   |                            |
  //   |          ref(on)           |
  //   |                            |
  //   ------------------------------
  //   |        |          |        |
  //   | Empty  | ref(on)  |   on   |
  //   |        |          |        |
  //   ------------------------------
  //
  auto v_arr = make_unique<VerticalArrayView>(
      VerticalArrayView::align::left );
  // This one is referenced, so must stay alive.
  auto checkbox = make_unique<CheckBoxView>( /*on=*/true );
  {
    auto h_arr = make_unique<HorizontalArrayView>(
        HorizontalArrayView::align::middle );
    h_arr->add_view( make_unique<EmptyView>() );
    h_arr->add_view( make_unique<RefView>( *checkbox ) );
    h_arr->add_view( make_unique<CheckBoxView>( /*on=*/true ) );

    v_arr->add_view( make_unique<CheckBoxView>( /*on=*/false ) );
    v_arr->add_view( make_unique<RefView>( *checkbox ) );
    v_arr->add_view( std::move( h_arr ) );
  }

  // Now, check that padding has been inserted only around the
  // checkbox views (not around the composite/ref views and not
  // around the empty view).

  auto const validate = []( unique_ptr<View> const& out,
                            int const phase ) {
    REQUIRE( out != nullptr );
    INFO( format( "phase: {}", phase ) );
    REQUIRE( out->cast_safe<VerticalArrayView>() );
    auto const v_arr = *out->cast_safe<VerticalArrayView>();
    REQUIRE( v_arr->count() == 3 );

    // First row.
    REQUIRE( v_arr->at( 0 ).view->cast_safe<PaddingView>() );
    REQUIRE( ( **v_arr->at( 0 ).view->cast_safe<PaddingView>() )
                 .single()
                 ->cast_safe<CheckBoxView>() );
    REQUIRE_FALSE(
        ( **( **v_arr->at( 0 ).view->cast_safe<PaddingView>() )
                .single()
                ->cast_safe<CheckBoxView>() )
            .on() );

    // Second row.
    REQUIRE( v_arr->at( 1 ).view->cast_safe<PaddingView>() );
    REQUIRE( ( **v_arr->at( 1 ).view->cast_safe<PaddingView>() )
                 .single()
                 ->cast_safe<RefView>() );
    REQUIRE(
        ( **( **v_arr->at( 1 ).view->cast_safe<PaddingView>() )
                .single()
                ->cast_safe<RefView>() )
            .referenced()
            .cast_safe<CheckBoxView>() );
    REQUIRE( ( **( **( **v_arr->at( 1 )
                             .view->cast_safe<PaddingView>() )
                         .single()
                         ->cast_safe<RefView>() )
                     .referenced()
                     .cast_safe<CheckBoxView>() )
                 .on() );

    // Third row.
    REQUIRE(
        v_arr->at( 2 ).view->cast_safe<HorizontalArrayView>() );
    auto& h_arr =
        **v_arr->at( 2 ).view->cast_safe<HorizontalArrayView>();

    // First column.
    REQUIRE( h_arr.at( 0 ).view->cast_safe<EmptyView>() );

    // Second column.
    REQUIRE( h_arr.at( 1 ).view->cast_safe<PaddingView>() );
    REQUIRE( ( **h_arr.at( 1 ).view->cast_safe<PaddingView>() )
                 .single()
                 ->cast_safe<RefView>() );
    REQUIRE(
        ( **( **h_arr.at( 1 ).view->cast_safe<PaddingView>() )
                .single()
                ->cast_safe<RefView>() )
            .referenced()
            .cast_safe<CheckBoxView>() );
    REQUIRE( ( **( **( **h_arr.at( 1 )
                             .view->cast_safe<PaddingView>() )
                         .single()
                         ->cast_safe<RefView>() )
                     .referenced()
                     .cast_safe<CheckBoxView>() )
                 .on() );

    // Third column.
    REQUIRE( h_arr.at( 2 ).view->cast_safe<PaddingView>() );
    REQUIRE( ( **h_arr.at( 2 ).view->cast_safe<PaddingView>() )
                 .single()
                 ->cast_safe<CheckBoxView>() );
    REQUIRE(
        ( **( **h_arr.at( 2 ).view->cast_safe<PaddingView>() )
                .single()
                ->cast_safe<CheckBoxView>() )
            .on() );
  };

  // Run/test.
  unique_ptr<View> out = std::move( v_arr );

  autopad( out );
  validate( out, 1 );

  // Now try to pad again and make sure no cahnges.

  autopad( out );
  validate( out, 2 );
}

} // namespace
} // namespace rn
