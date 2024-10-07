/****************************************************************
**odr.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-06.
*
* Description: Helpers related to the concept of the ODR and/or
*              ODR-use.
*
*****************************************************************/
#pragma once

#include "config.hpp"

namespace base {

// This is used to force a method of a templated class to be
// ODR-used (and hence not discarded by the linker) when it is
// not otherwise called from outside the class. This is useful
// for methods on templated structs that need to run (e.g. regis-
// tration methods) even when the class is never instantiated.
// There are multiple ways to accomplish this:
//
// 1. static_assert on address equality:
//
//      static_assert( &registration == &registration );
//
//    which was taken from:
//
//      stackoverflow.com/questions/6420985/
//          how-to-force-a-static-member-to-be-initialized
//
// 2. Store address in constexpr var:
//
//      static constexpr auto const force = &registration;
//
//    However, this one does not work on gcc (not sure if it
//    works on clang; but doesn't matter since we need to support
//    both).
//
// 3. Voiding the function itself, e.g.:
//
//      type_traits() { (void)registration; }
//
//    which was taken from https://youtu.be/0a3wjaeP6eQ. However,
//    this one won't always work (at least on gcc) because the
//    voided registration must go in a function that is itself
//    used from the standpoint of the linker; in some cases there
//    is no such function outside of the registration process it-
//    self (which we are trying to make used), so there is
//    nowhere to put it in general.
//
// So we will take approach #1, but will modify it slightly to
// avoid explicitly comparing two identical things in order to
// suppress gcc's -Wautological-compare warning.
#define ODR_USE_MEMBER_METHOD( method ) \
  static_assert( &method == &method + 1 - 1 )

} // namespace base
