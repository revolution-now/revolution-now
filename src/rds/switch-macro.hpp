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
//   2. Compiler will warn about missing cases and fallthrough.
//   3. Are easy to write and looks natural; no starting or
//      closing macros needed, though braces are required.
//   4. Automatically provide the reference to the alternative
//      object in each case block.
//
// For example, the following:
//
//   SWITCH( *my_sumtype ) {
//     CASE( first_alt ) {
//       first_alt.field = 1;
//       break;
//     }
//     CASE( another_alt ) {
//       return (another_alt.some_member == e_nation::english);
//     }
//   }
//
// will expand into:
//
//   switch( auto&& __o = *my_sumtype; __o.to_enum() ) {
//     case C::e::first_alt:
//       if( auto& first_alt [[maybe_unused]] =
//             __o.get<C::first_alt>(); true ) {
//         first_alt.field = 1;
//         break;
//       }
//     case C::e::another_alt:
//       if( auto& another_alt [[maybe_unused]] =
//             __o.get<C::another_alt>(); true ) {
//         return (another_alt.some_member == e_nation::english);
//       }
//     }
//   }
//
// where C = std::remove_cvref_t<decltype( __o )>.

#define SWITCH( what ) switch( auto&& __o = what; __o.to_enum() )

#define CASE( alt )                                           \
  case std::remove_cvref_t<decltype( __o )>::e::alt:          \
    if( auto& alt [[maybe_unused]] =                          \
            __o.get<                                          \
                std::remove_cvref_t<decltype( __o )>::alt>(); \
        true )
