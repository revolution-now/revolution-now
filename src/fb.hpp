/****************************************************************
**fb.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-05.
*
* Description: Stuff needed by headers with serializable types.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"

/****************************************************************
** Tables/Structs
*****************************************************************/
#define SERIALIZABLE_TABLE( name )                            \
  ND FBOffset<fb::name> serialize_table( FBBuilder& builder ) \
      const

#define SERIALIZABLE_TABLE_DEF( name )      \
  FBOffset<fb::name> name::serialize_table( \
      FBBuilder& builder ) const

#define SERIALIZABLE_STRUCT( name ) \
  ND fb::name serialize_struct() const

#define SERIALIZABLE_STRUCT_DEF( name ) \
  fb::name name::serialize_struct() const

/****************************************************************
** Enums
*****************************************************************/
#define SERIALIZABLE_ENUM( name )                            \
  static_assert( static_cast<int>( fb::name::MIN ) == 0 );   \
  static_assert( name::_size() ==                            \
                 static_cast<size_t>( fb::name::MAX ) + 1 ); \
  static_assert(                                             \
      static_cast<int>(                                      \
          ( *std::begin( values<name> ) )._value ) == 0 );   \
  inline fb::name serialize_enum( name e ) {                 \
    auto from_idx = e._value;                                \
    CHECK( from_idx >= 0 &&                                  \
           from_idx <= static_cast<int>( fb::name::MIN ) );  \
    DCHECK( fb::EnumNames##name()[from_idx] == e._name() );  \
    return fb::EnumValues##name()[from_idx];                 \
  }                                                          \
  inline name deserialize_enum( fb::name e ) {               \
    auto from_idx = static_cast<int>( e );                   \
    CHECK( from_idx >= 0 &&                                  \
           from_idx <= static_cast<int>( fb::name::MAX ) );  \
    auto res = name::_from_index_nothrow( from_idx );        \
    CHECK( res );                                            \
    DCHECK( ( *res )._name() ==                              \
            fb::EnumNames##name()[from_idx] );               \
    return *res;                                             \
  }

/****************************************************************
** Member Macros
*****************************************************************/
namespace detail {
//
}

#define SERIALIZABL_SAVE_ONE( name ) \
  ar.save( TO_STRING( name ), DEFER( name ) )
#define SERIALIZABL_SIZE_CHECK( name )                   \
  decltype( DEFER( name ) ) PP_JOIN( name, __ );         \
  static_assert( !std::is_reference_v<decltype( name )>, \
                 "cannot serialize references." )

#define SERIALIZABL_IMPL( ... )                            \
  std::byte last_member__;                                 \
  /* This goes out here so it doesn't depend on the */     \
  /* Archiver template argument, that way the static */    \
  /* assert can be checked while compiling the header */   \
  /* and does not need to wait until instantiation of */   \
  /* the serialize method in a cpp. */                     \
  struct serialize_size_checker__ {                        \
    PP_MAP_SEMI( SERIALIZABL_SIZE_CHECK, __VA_ARGS__ )     \
    std::byte last_member__;                               \
  };                                                       \
  template<typename Archiver>                              \
  void serialize( Archiver&& ar ) const {                  \
    PP_MAP_SEMI( SERIALIZABL_SAVE_ONE, __VA_ARGS__ )       \
    using Parent_t = std::decay_t<decltype( *this )>;      \
    static_assert( offsetof( Parent_t, last_member__ ) ==  \
                       offsetof( serialize_size_checker__, \
                                 last_member__ ),          \
                   "some members are missing from the "    \
                   "serialization list." );                \
  }

#define SERIALIZABLE( ... ) \
  EVAL( SERIALIZABL_IMPL( __VA_ARGS__ ) )

namespace rn {} // namespace rn
