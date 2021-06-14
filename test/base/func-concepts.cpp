/****************************************************************
**func-concepts.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-14.
*
* Description: Unit tests for the src/base/func-concepts.*
*              module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/func-concepts.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace base {
namespace {

using namespace std;

/****************************************************************
** Function props.
*****************************************************************/
namespace func {

struct A;

struct NonCallable {};
struct Callable {
  void operator()();
};
struct StaticCallable {
  static void fun();
};

struct Stateful1 {
  void operator()();
};
struct Stateful2 {
  void operator()();
  int  x;
};

void takes_params( int, char ) {}

void  normal() {}
auto  lambda              = []() {};
auto* pointer_to_normal   = normal;
auto* pointer_to_lambda   = &lambda;
auto& reference_to_normal = normal;
auto& reference_to_lambda = lambda;

auto c_ext_lambda         = []( A* ) -> int { return 0; };
auto c_ext_lambda_capture = [x = 1.0]( A* ) -> int {
  (void)x;
  return 0;
};

} // namespace func

// Use the above functions so that the compiler does not emit
// warnings about them not being used.
TEST_CASE( "[func-concepts] use funcs" ) {
  func::takes_params( 5, 'c' );
  func::normal();
  func::lambda();
  func::pointer_to_normal();
  ( *func::pointer_to_lambda )();
  func::reference_to_normal();
  func::reference_to_lambda();
  func::c_ext_lambda( nullptr );
  func::c_ext_lambda_capture( nullptr );
}

/****************************************************************
** Sanity checks (not the actual tests).
*****************************************************************/
static_assert( sizeof( func::c_ext_lambda ) == 1 );
static_assert( sizeof( func::c_ext_lambda_capture ) ==
               sizeof( 1.0 ) );
static_assert( sizeof( func::c_ext_lambda ) <
               sizeof( func::c_ext_lambda_capture ) );
static_assert( is_convertible_v<decltype( func::c_ext_lambda ),
                                int ( * )( func::A* )> );
static_assert(
    !is_convertible_v<decltype( func::c_ext_lambda_capture ),
                      int ( * )( func::A* )> );

/****************************************************************
** NonOverloadedCallable
*****************************************************************/
static_assert(
    NonOverloadedCallable<decltype( func::c_ext_lambda )> );
static_assert( NonOverloadedCallable<
               decltype( func::c_ext_lambda_capture )> );

static_assert( NonOverloadedCallable<func::Stateful1> );
static_assert( NonOverloadedCallable<func::Stateful2> );

static_assert(
    NonOverloadedCallable<decltype( func::takes_params )> );

static_assert( NonOverloadedCallable<int()> );
static_assert( NonOverloadedCallable<
               decltype( func::StaticCallable::fun )> );
static_assert( NonOverloadedCallable<
               decltype( &func::StaticCallable::fun )> );
static_assert( NonOverloadedCallable<decltype( func::normal )> );
static_assert(
    NonOverloadedCallable<decltype( func::pointer_to_normal )> );
static_assert( NonOverloadedCallable<
               decltype( func::reference_to_normal )> );

static_assert( NonOverloadedCallable<decltype( func::lambda )> );
static_assert( NonOverloadedCallable<
               decltype( func::reference_to_lambda )> );
static_assert( !NonOverloadedCallable<
               decltype( func::pointer_to_lambda )> );

static_assert( NonOverloadedCallable<func::Callable> );
static_assert( NonOverloadedCallable<func::Callable&> );
static_assert( !NonOverloadedCallable<func::Callable*> );

static_assert( !NonOverloadedCallable<func::NonCallable> );
static_assert( !NonOverloadedCallable<func::NonCallable&> );
static_assert( !NonOverloadedCallable<func::NonCallable*> );

/****************************************************************
** NonOverloaded{Stateless,Stateful}Function.
*****************************************************************/
static_assert( NonOverloadedStatelessCallable<
               decltype( func::c_ext_lambda )> );
static_assert( !NonOverloadedStatefulCallable<
               decltype( func::c_ext_lambda )> );
static_assert( !NonOverloadedStatelessCallable<
               decltype( func::c_ext_lambda_capture )> );
static_assert( NonOverloadedStatefulCallable<
               decltype( func::c_ext_lambda_capture )> );

static_assert( NonOverloadedStatefulCallable<int() const> );
static_assert( NonOverloadedStatefulCallable<int() &> );

static_assert( !NonOverloadedStatelessCallable<
               decltype( func::Stateful1{} )> );
static_assert( !NonOverloadedStatelessCallable<
               decltype( func::Stateful2{} )> );
static_assert( NonOverloadedStatefulCallable<
               decltype( func::Stateful1{} )> );
static_assert( NonOverloadedStatefulCallable<
               decltype( func::Stateful2{} )> );

static_assert( NonOverloadedStatelessCallable<
               decltype( func::StaticCallable::fun )> );
static_assert( !NonOverloadedStatefulCallable<
               decltype( func::StaticCallable::fun )> );

} // namespace
} // namespace base
