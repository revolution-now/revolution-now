/****************************************************************
**save-game-macros.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-18.
*
* Description: Macros used for declaring save-game functions.
*
*****************************************************************/
#pragma once

#define DECLARE_SAVEGAME_SERIALIZER( name )                     \
  namespace fb {                                                \
  struct G_##name;                                              \
  }                                                             \
  namespace rn {                                                \
  void SaveGameSerializer(                                      \
      FBBuilder& builder, FBOffset<fb::G_##name>* out_offset ); \
  expect<> SaveGameDeserializer( fb::G_##name const* src );     \
  } // namespace rn

#define DEFINE_SAVEGAME_SERIALIZER_STATE( name ) \
  G_##name& G() {                                \
    static G_##name s;                           \
    return s;                                    \
  }

#define DEFINE_SAVEGAME_SERIALIZERS( name )                   \
  void SaveGameSerializer(                                    \
      FBBuilder&              builder,                        \
      FBOffset<fb::G_##name>* out_offset ) {                  \
    *out_offset =                                             \
        serial::serialize<fb::G_##name>(                      \
            builder, G(), ::rn::serial::rn_adl_tag{} )        \
            .get();                                           \
  }                                                           \
  expect<> SaveGameDeserializer( fb::G_##name const* src ) {  \
    return serial::deserialize( src, &G(),                    \
                                ::rn::serial::rn_adl_tag{} ); \
  }
