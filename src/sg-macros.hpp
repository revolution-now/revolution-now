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
  expect<> savegame_deserializer( T const* src );              \
  template<typename T>                                         \
  expect<> savegame_post_validate( T const* );                 \
  template<typename T>                                         \
  void default_construct_savegame_state( T const* );

struct SaveGameComponentBase {
  bool operator==( SaveGameComponentBase const& ) const =
      default;
  ::rn::expect<> check_invariants_safe() const {
    // This function is not used in the top-level save-game com-
    // ponents. Instead we use the `sync` method.
    return ::rn::xp_success_t{};
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
  expect<> savegame_deserializer<fb::SG_##name>(            \
      fb::SG_##name const* src ) {                          \
    default_construct_savegame_state( src );                \
    XP_OR_RETURN_(                                          \
        serial::deserialize( src, &SG(), serial::ADL{} ) ); \
    return SG().sync();                                     \
  }                                                         \
  template<>                                                \
  expect<> savegame_post_validate<fb::SG_##name>(           \
      fb::SG_##name const* ) {                              \
    return SG().validate();                                 \
  }                                                         \
  namespace {

#define SAVEGAME_FRIENDS( name )                             \
  bool operator==( SG_##name const& ) const = default;       \
                                                             \
  friend expect<> rn::savegame_deserializer<fb::SG_##name>(  \
      fb::SG_##name const* src );                            \
  friend expect<> rn::savegame_post_validate<fb::SG_##name>( \
      fb::SG_##name const* src )

// Called just after a given module is deserialized.
#define SAVEGAME_SYNC() expect<> sync()
// Called after all modules are deserialized.
#define SAVEGAME_VALIDATE() expect<> validate()
