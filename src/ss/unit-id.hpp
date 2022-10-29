/****************************************************************
**unit-id.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-12.
*
* Description: Strongly-typed IDs for units.
*
*****************************************************************/
#pragma once

// luapp
#include "luapp/ext.hpp"

// cdr
#include "cdr/ext.hpp"

// base
#include "base/to-str.hpp"

namespace rn {

// The types in this module are defined as empty enum classes
// with an underlying type of int. Even though the enums have no
// named values, it is guaranteed that they can assume any value
// that an int can assume (i.e. no undefined behavior).

// TODO: When C++23 is ready replace with std::to_underlying.
template<typename Enum>
inline constexpr auto to_underlying( Enum e ) noexcept {
  return static_cast<std::underlying_type_t<Enum>>( e );
}

/****************************************************************
** UnitId
*****************************************************************/
// European units.
enum class UnitId : int {};

// to_str
void to_str( UnitId o, std::string& out, base::ADL_t );

// cdr
cdr::value to_canonical( cdr::converter& conv, UnitId o,
                         cdr::tag_t<UnitId> );

cdr::result<UnitId> from_canonical( cdr::converter&   conv,
                                    cdr::value const& v,
                                    cdr::tag_t<UnitId> );

// lua
void lua_push( lua::cthread L, UnitId o );

base::maybe<UnitId> lua_get( lua::cthread L, int idx,
                             lua::tag<UnitId> );

/****************************************************************
** NativeUnitId
*****************************************************************/
// Native units.
enum class NativeUnitId : int {};

// to_str
void to_str( NativeUnitId o, std::string& out, base::ADL_t );

// cdr
cdr::value to_canonical( cdr::converter& conv, NativeUnitId o,
                         cdr::tag_t<NativeUnitId> );

cdr::result<NativeUnitId> from_canonical(
    cdr::converter& conv, cdr::value const& v,
    cdr::tag_t<NativeUnitId> );

// lua
void lua_push( lua::cthread L, NativeUnitId o );

base::maybe<NativeUnitId> lua_get( lua::cthread L, int idx,
                                   lua::tag<NativeUnitId> );

/****************************************************************
** GenericUnitId
*****************************************************************/
// Can represent either European units or native units.
enum class GenericUnitId : int {};

// to_str
void to_str( GenericUnitId o, std::string& out, base::ADL_t );

// cdr
cdr::value to_canonical( cdr::converter& conv, GenericUnitId o,
                         cdr::tag_t<GenericUnitId> );

cdr::result<GenericUnitId> from_canonical(
    cdr::converter& conv, cdr::value const& v,
    cdr::tag_t<GenericUnitId> );

// lua
void lua_push( lua::cthread L, GenericUnitId o );

base::maybe<GenericUnitId> lua_get( lua::cthread L, int idx,
                                    lua::tag<GenericUnitId> );

} // namespace rn
