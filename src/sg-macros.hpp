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
#include "fb.hpp"

// C++ standard library
#include <type_traits>

// All the types in this macro should be forward declarable.
#define DECLARE_SAVEGAME_SERIALIZERS( name )                   \
  } /* namspace rn */                                          \
  namespace fb {                                               \
  struct SG_##name;                                            \
  }                                                            \
  namespace rn {                                               \
  template<typename T>                                         \
  void savegame_serializer( serial::FBBuilder&   builder,      \
                            serial::FBOffset<T>* out_offset ); \
  template<typename T>                                         \
  valid_deserial_t savegame_deserializer( T const* src );      \
  template<typename T>                                         \
  valid_deserial_t savegame_post_validate( T const* );         \
  template<typename T>                                         \
  void default_construct_savegame_state( T const* );

struct SaveGameComponentBase {
  bool operator==( SaveGameComponentBase const& ) const =
      default;
  ::rn::valid_deserial_t check_invariants_safe() const {
    // This function is not used in the top-level save-game com-
    // ponents. Instead we use the `sync` method.
    return ::base::valid;
  }
};

#define SAVEGAME_STRUCT( name ) \
  SG_##name : public SaveGameComponentBase

#define SAVEGAME_MEMBERS( name, ... ) \
  SERIALIZABLE_TABLE_MEMBERS( fb, SG_##name, __VA_ARGS__ )

#define SAVEGAME_IMPL( name )                               \
  SG_##name& SG() {                                         \
    static SG_##name s;                                     \
    return s;                                               \
  }                                                         \
  } /* anonymous namespace */                               \
  template<>                                                \
  void savegame_serializer<fb::SG_##name>(                  \
      serial::FBBuilder & builder,                          \
      serial::FBOffset<fb::SG_##name> * out_offset ) {      \
    auto offset = serial::serialize<fb::SG_##name>(         \
        builder, SG(), serial::ADL{} );                     \
    static_assert(                                          \
        std::is_same_v<decltype( offset.get() ),            \
                       serial::FBOffset<fb::SG_##name>>,    \
        "Top-level save-game state can only be "            \
        "represented with Flatbuffers tables." );           \
    *out_offset = offset.get();                             \
  }                                                         \
  template<>                                                \
  void default_construct_savegame_state<fb::SG_##name>(     \
      fb::SG_##name const* ) {                              \
    SG() = SG_##name{};                                     \
  }                                                         \
  template<>                                                \
  valid_deserial_t savegame_deserializer<fb::SG_##name>(    \
      fb::SG_##name const* src ) {                          \
    default_construct_savegame_state( src );                \
    HAS_VALUE_OR_RET(                                       \
        serial::deserialize( src, &SG(), serial::ADL{} ) ); \
    return SG().sync();                                     \
  }                                                         \
  template<>                                                \
  valid_deserial_t savegame_post_validate<fb::SG_##name>(   \
      fb::SG_##name const* ) {                              \
    return SG().validate();                                 \
  }                                                         \
  namespace {

#define SAVEGAME_FRIENDS( name )                       \
  bool operator==( SG_##name const& ) const = default; \
                                                       \
  friend valid_deserial_t                              \
  rn::savegame_deserializer<fb::SG_##name>(            \
      fb::SG_##name const* src );                      \
  friend valid_deserial_t                              \
  rn::savegame_post_validate<fb::SG_##name>(           \
      fb::SG_##name const* src )

// Called just after a given module is deserialized.
#define SAVEGAME_SYNC() valid_deserial_t sync()
// Called after all modules are deserialized.
#define SAVEGAME_VALIDATE() valid_deserial_t validate()
