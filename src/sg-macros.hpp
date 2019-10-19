/****************************************************************
**sg-macros.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-18.
*
* Description: Macros used for declaring save-game functions.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "aliases.hpp"
#include "errors.hpp"

// C++ standard library
#include <type_traits>

// All the types in this macro should be forward declarable.
#define DECLARE_SAVEGAME_SERIALIZERS( name ) \
  } /* namspace rn */                        \
  namespace fb {                             \
  struct SG_##name;                          \
  }                                          \
  namespace rn {                             \
  void SaveGameSerializer(                   \
      FBBuilder&               builder,      \
      FBOffset<fb::SG_##name>* out_offset ); \
  expect<> SaveGameDeserializer( fb::SG_##name const* src );

#define SAVEGAME_STRUCT( name ) SG_##name

#define SAVEGAME_MEMBERS( name, ... ) \
  SERIALIZABLE_TABLE_MEMBERS( SG_##name, __VA_ARGS__ )

#define SAVEGAME_IMPL( name )                                 \
  SG_##name& SG() {                                           \
    static SG_##name s;                                       \
    return s;                                                 \
  }                                                           \
  } /* anonymous namespace */                                 \
  void SaveGameSerializer(                                    \
      FBBuilder&               builder,                       \
      FBOffset<fb::SG_##name>* out_offset ) {                 \
    auto offset = serial::serialize<fb::SG_##name>(           \
        builder, SG(), ::rn::serial::rn_adl_tag{} );          \
    static_assert( std::is_same_v<decltype( offset.get() ),   \
                                  FBOffset<fb::SG_##name>>,   \
                   "Top-level save-game state can only be "   \
                   "represented with Flatbuffers tables." );  \
    *out_offset = offset.get();                               \
  }                                                           \
  expect<> SaveGameDeserializer( fb::SG_##name const* src ) { \
    return serial::deserialize( src, &SG(),                   \
                                ::rn::serial::rn_adl_tag{} ); \
  }                                                           \
  namespace {
