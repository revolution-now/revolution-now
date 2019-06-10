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
#include "aliases.hpp"
#include "errors.hpp"
#include "hash.hpp"

// Revolution Now (for importing enum types)
#include "conductor.hpp"
#include "window.hpp"

// base-util
#include "base-util/pp.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"

// C++ standard library
#include <functional>
#include <typeinfo>

using namespace std;

namespace rn {

namespace {

template<typename EnumType>
char const* enum_to_str( int );

#define ENUM_TO_STR_SINGLE( val, str ) \
  case +enum_type::val:                \
    return str;

#define ENUM_TO_STR_IMPL( type, ... )                      \
  template<>                                               \
  char const* enum_to_str<type>( int e ) {                 \
    DCHECK( type::_is_valid( e ) );                        \
    auto val        = type::_from_integral_unchecked( e ); \
    using enum_type = type;                                \
    switch( val ) {                                        \
      PP_MAP_TUPLE( ENUM_TO_STR_SINGLE, __VA_ARGS__ )      \
    }                                                      \
  }

#define TRANSLATION( type, ... ) \
  EVAL( ENUM_TO_STR_IMPL( type, __VA_ARGS__ ) )

#include "../config/enum-name.inl"

// Takes the index of an enum value.
using EnumNameFunc = char const* (*)( int );

#define ENUM_TO_STR_FUNC( type ) \
  { type::_name(), &enum_to_str<type> }

using EnumNameMap = FlatMap<char const*, EnumNameFunc>;

EnumNameMap& enum_display_names() {
  static auto m = [] {
    return EnumNameMap{EVAL( PP_MAP_COMMAS(
        ENUM_TO_STR_FUNC, //
        /*************************************************/
        ui::e_confirm, //
        e_music_player //
        /*************************************************/
        ) )};
  }();
  return m;
};

} // namespace

namespace internal {

char const* enum_to_display_name( char const* type_name,
                                  int         index,
                                  char const* default_ ) {
  if( !enum_display_names().contains( type_name ) )
    return default_;
  return enum_display_names()[type_name]( index );
}

} // namespace internal

} // namespace rn
