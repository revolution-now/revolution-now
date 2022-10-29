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

// Cdr
cdr::value to_canonical( cdr::converter& conv, UnitId o,
                         cdr::tag_t<UnitId> );

cdr::result<UnitId> from_canonical( cdr::converter&   conv,
                                    cdr::value const& v,
                                    cdr::tag_t<UnitId> );

// lua
void lua_push( lua::cthread L, UnitId o );

base::maybe<UnitId> lua_get( lua::cthread L, int idx,
                             lua::tag<UnitId> );

} // namespace rn
