/****************************************************************
**switch-macro.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-27.
*
* Description: Macros for creating switch/case statements for Rds
*              sumtypes.
*
*****************************************************************/
#pragma once

// These macros will create switch/case statements for Rds sum-
// types that:
//
//   1. Are as friendly to control flow as normal switch state-
//      ments. `break`, `return`, `continue`, etc. have exactly
//      the expected effect.
//   2. Compiler will warn about fallthrough.
//   3. Are easy to write and look nice.
//   4. Automatically provide the reference to the alternative
//      object in each case block.
//
// For example, the following:
//
//  SWITCH( *my_sumtype ) {
//    CASE( first_alt ) { //
//      return true;
//    }
//    CASE( another_alt ) {
//      return (o.some_member == e_nation::english);
//    }
//    END_CASES;
//  }
//
// will expand into:
//
//  switch( auto&& __o = *my_sumtype; __o.to_enum() ) {
//    using __C = std::remove_cvref_t<decltype( __o )>;
//    {
//      {}
//      case __C::e::first_alt: {
//        auto & o [[maybe_unused]] = __o.get<__C::first_alt>();
//        {
//          return true;
//        }
//      }
//      case __C::e::another_alt: {
//        auto& o [[maybe_unused]] = __o.get<__C::another_alt>();
//        {
//          return (o.some_member == e_nation::english);
//        }
//      }
//    };
//  }

#define SWITCH( what )                                \
  switch( auto&& __o = what; __o.to_enum() ) {        \
    using __C = std::remove_cvref_t<decltype( __o )>; \
    {
#define CASE( alt )   \
  }                   \
  case __C::e::alt: { \
    auto& o [[maybe_unused]] = __o.get<__C::alt>();

#define END_CASES \
  }               \
  }
