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

// Abseil
#include "absl/container/flat_hash_map.h"

// C++ standard library
#include <functional>
#include <typeinfo>

using namespace std;

namespace rn {

namespace {

template<typename EnumType>
char const* enum_val_to_str( int );

template<>
char const* enum_val_to_str<ui::e_confirm>( int in ) {
  DCHECK( ui::e_confirm::_is_valid( in ) );
  auto val = ui::e_confirm::_from_integral_unchecked( in );
  switch( val ) {
    case +ui::e_confirm::yes: return "Yes";
    case +ui::e_confirm::no: return "No";
  }
};

template<>
char const* enum_val_to_str<e_music_player>( int in ) {
  DCHECK( e_music_player::_is_valid( in ) );
  auto val = e_music_player::_from_integral_unchecked( in );
  switch( val ) {
    case +e_music_player::midiseq: return "MIDI Sequencer";
    case +e_music_player::ogg: return "OGG Player";
    case +e_music_player::silent: return "Silent";
  }
};

// Takes the index of an enum value.
using EnumNameFunc = char const* (*)( int );

FlatMap<string /*type hash*/, EnumNameFunc> g_enum_display_names{
    {"e_confirm", &enum_val_to_str<ui::e_confirm>},       //
    {"e_music_player", &enum_val_to_str<e_music_player>}, //
};

} // namespace

char const* enum_to_display_name_impl( string const& type_name,
                                       int           index,
                                       char const*   default_ ) {
  if( !g_enum_display_names.contains( type_name ) )
    return default_;
  return g_enum_display_names[type_name]( index );
}

} // namespace rn
