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
#include "meta.hpp"

// Flatbuffers
#include "fb/vocab_generated.h"

// base-util
#include "base-util/mp.hpp"
#include "base-util/pp.hpp"

/****************************************************************
** Tables/Structs
*****************************************************************/
#define SERIALIZABLE_STRUCT( name ) \
  ND fb::name serialize_struct() const

#define SERIALIZABLE_STRUCT_DEF( name ) \
  fb::name name::serialize_struct() const

/****************************************************************
** Enums
*****************************************************************/
#define SERIALIZABLE_ENUM( name )                             \
  static_assert( static_cast<int>( fb::name::MIN ) == 0 );    \
  static_assert( name::_size() ==                             \
                 static_cast<size_t>( fb::name::MAX ) + 1 );  \
  static_assert(                                              \
      static_cast<int>(                                       \
          ( *std::begin( values<name> ) )._value ) == 0 );    \
  inline fb::name serialize_enum( name e ) {                  \
    auto from_idx = e._value;                                 \
    CHECK( from_idx >= 0 &&                                   \
           from_idx <= static_cast<int>( fb::name::MAX ) );   \
    DCHECK( std::string( fb::EnumNames##name()[from_idx] ) == \
                e._to_string(),                               \
            "{} != {}", fb::EnumNames##name()[from_idx],      \
            e._to_string() );                                 \
    return fb::EnumValues##name()[from_idx];                  \
  }                                                           \
  inline name deserialize_enum( fb::name e ) {                \
    auto from_idx = static_cast<int>( e );                    \
    CHECK( from_idx >= 0 &&                                   \
           from_idx <= static_cast<int>( fb::name::MAX ) );   \
    auto res = name::_from_index_nothrow( from_idx );         \
    CHECK( res );                                             \
    DCHECK( std::string( ( *res )._to_string() ) ==           \
                fb::EnumNames##name()[from_idx],              \
            "{} != {}", ( *res )._to_string(),                \
            fb::EnumNames##name()[from_idx] );                \
    return *res;                                              \
  }

/****************************************************************
** Uniform serialization interface.
*****************************************************************/
namespace rn::serial {

namespace detail {

template<typename SerializedT>
struct ReturnValue {
  SerializedT o_;
  SerializedT get() const { return o_; }
};
template<typename SerializedT>
ReturnValue( SerializedT )->ReturnValue<SerializedT>;

template<typename SerializedT>
struct ReturnAddress {
  SerializedT        o_;
  SerializedT const* get() const&& = delete;
  SerializedT const* get() const& { return &o_; }
};
template<typename SerializedT>
ReturnAddress( SerializedT )->ReturnAddress<SerializedT>;

template<typename From>
auto opt_value_type_to_fb = 0;

#define OPT_FACTORY( cpp_type, fb_type )       \
  template<>                                   \
  inline auto opt_value_type_to_fb<cpp_type> = \
      ::fb::CreateOpt_##fb_type

OPT_FACTORY( bool, bool );
OPT_FACTORY( int, int );
OPT_FACTORY( std::string, string );

} // namespace detail

// For scalars (non-enums).
template<typename T,              //
         std::enable_if_t<        //
             std::is_scalar_v<T>, //
             int> = 0             //
         >
auto serialize( FBBuilder&, T const& o ) {
  struct Ret {
    T o_;
    T get() const { return o_; }
  };
  return Ret{o};
}

// For typed ints.
template<typename T,                                //
         std::enable_if_t<                          //
             std::is_same_v<int, decltype( T::_ )>, //
             int> = 0                               //
         >
auto serialize( FBBuilder&, T const& o ) {
  struct Ret {
    int o_;
    int get() const { return o_; }
  };
  return Ret{o._};
}

// For enums.
template<typename T, decltype( serialize_enum(
                         std::declval<T>() ) )* = nullptr>
auto serialize( FBBuilder&, T const& o ) {
  return detail::ReturnValue{serialize_enum( o )};
}

// For C++ classes/structs that get serialized as FB structs.
template<typename T, decltype( &T::serialize_struct )* = nullptr>
auto serialize( FBBuilder&, T const& o ) {
  return detail::ReturnAddress{o.serialize_struct()};
}

// For C++ classes/structs that get serialized as FB tables.
template<typename T, decltype( &T::serialize_table )* = nullptr>
auto serialize( FBBuilder& builder, T const& o ) {
  return detail::ReturnValue{o.serialize_table( builder )};
}

// For C++ classes/structs that get serialized as FB structs.
template<typename T,                             //
         std::enable_if_t<                       //
             mp::is_optional_v<std::decay_t<T>>, //
             int> = 0                            //
         >
auto serialize( FBBuilder& builder, T const& o ) {
  auto factory =
      detail::opt_value_type_to_fb<typename T::value_type>;
  if( o.has_value() ) {
    auto s_value = serialize( builder, *o );
    return detail::ReturnValue{
        factory( builder, /*has_value=*/true, s_value.get() )};
  } else {
    return detail::ReturnValue{
        factory( builder, /*has_value=*/false, {} )};
  }
}

} // namespace rn::serial

/****************************************************************
** Table Macros
*****************************************************************/
#define SERIAL_CALL_SERIALIZE_IMPL( type, var ) \
  auto PP_JOIN( s_, var ) = serialize( builder, var )

#define SERIAL_CALL_SERIALIZE( p ) SERIAL_CALL_SERIALIZE_IMPL p

#define SERIAL_GET_SERIALIZED_IMPL( type, var ) \
  PP_JOIN( s_, var ).get()
#define SERIAL_GET_SERIALIZED( p ) SERIAL_GET_SERIALIZED_IMPL p

#define SERIAL_DECLARE_VAR( type, var ) type var;

#define SERIALIZABLE_TABLE_MEMBERS_IMPL( name, ... )           \
  PP_MAP_TUPLE( SERIAL_DECLARE_VAR, __VA_ARGS__ )              \
public:                                                        \
  FBOffset<fb::name> serialize_table( FBBuilder& builder )     \
      const {                                                  \
    using ::rn::serial::serialize;                             \
    PP_MAP_SEMI( SERIAL_CALL_SERIALIZE, __VA_ARGS__ )          \
    return fb::Create##name(                                   \
        builder,                                               \
        PP_MAP_COMMAS( SERIAL_GET_SERIALIZED, __VA_ARGS__ ) ); \
  }                                                            \
                                                               \
private:

#define SERIALIZABLE_TABLE_MEMBERS( ... ) \
  EVAL( SERIALIZABLE_TABLE_MEMBERS_IMPL( __VA_ARGS__ ) )

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
