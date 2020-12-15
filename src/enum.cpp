/****************************************************************
**enum.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-06-09.
*
* Description: Helpers for dealing with enums.
*
*****************************************************************/
#include "enum.hpp"

// Revolution Now
#include "commodity.hpp"
#include "errors.hpp"
#include "hash.hpp"
#include "macros.hpp"

// Revolution Now (for importing enum types)
#include "conductor.hpp"
#include "main-menu.hpp"
#include "window.hpp"

// base-util
#include "base-util/pp.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_split.h"

// magic enum
#include "magic_enum.hpp"

// C++ standard library
#include <functional>
#include <typeinfo>

using namespace std;

namespace rn {

namespace {

template<typename EnumType>
char const* enum_to_str( int );

#define ENUM_TO_STR_SINGLE( val, str ) \
  case enum_type::val:                 \
    return str;

#define ENUM_TO_STR_IMPL( type, ... )                    \
  template<>                                             \
  char const* enum_to_str<type>( int e ) {               \
    DCHECK( magic_enum::enum_cast<type>( e ) );          \
    auto val        = *magic_enum::enum_cast<type>( e ); \
    using enum_type = type;                              \
    switch( val ) {                                      \
      PP_MAP_TUPLE( ENUM_TO_STR_SINGLE, __VA_ARGS__ )    \
    }                                                    \
  }

#define TRANSLATION( type, ... ) \
  EVAL( ENUM_TO_STR_IMPL( type, __VA_ARGS__ ) )

#include "../config/c++/enum-name.inl"

// Takes the index of an enum value.
using EnumNameFunc = char const* (*)( int );

#define ENUM_TO_STR_FUNC( type ) \
  { internal::remove_namespaces( #type ), &enum_to_str<type> }

using EnumNameMap =
    absl::flat_hash_map<string_view, EnumNameFunc>;

EnumNameMap& enum_display_names() {
  static auto m = [] {
    return EnumNameMap{ EVAL( PP_MAP_COMMAS(
        ENUM_TO_STR_FUNC, //
        /*************************************************/
        ui::e_confirm,   //
        e_music_player,  //
        e_commodity,     //
        e_main_menu_item //
        /*************************************************/
        ) ) };
  }();
  return m;
};

} // namespace

namespace internal {

string_view enum_to_display_name( string_view type_name,
                                  int         index,
                                  string_view default_ ) {
  if( !enum_display_names().contains( type_name ) )
    return default_;
  return enum_display_names()[type_name]( index );
}

} // namespace internal

} // namespace rn
