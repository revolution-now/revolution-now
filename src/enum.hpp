/****************************************************************
**enum.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-03.
*
* Description: Helpers for dealing with enums.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// magic enum
#include "magic_enum.hpp"

static_assert( magic_enum::is_magic_enum_supported,
               "Magic Enum is not supported on this compiler." );

namespace rn {

/****************************************************************
** Enum Value Display Names
*****************************************************************/
namespace internal {

std::string_view enum_to_display_name(
    std::string_view type_name, int index,
    std::string_view default_ );

// NOTE: duplicated from util.hpp.
inline constexpr std::string_view remove_namespaces(
    std::string_view input ) {
  auto pos = input.find_last_of( ':' );
  if( pos == std::string_view::npos ) return input;
  input.remove_prefix( pos + 1 );
  return input;
}

} // namespace internal

template<typename Enum>
std::string_view enum_to_display_name( Enum value ) {
  return internal::enum_to_display_name(
      /*type_name=*/internal::remove_namespaces(
          magic_enum::enum_type_name<Enum>() ),    //
      /*index=*/magic_enum::enum_integer( value ), //
      /*default=*/magic_enum::enum_name( value ) );
}

} // namespace rn
