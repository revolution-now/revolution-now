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
namespace rn {
// This is used as a dummy parameter to all rn::serial::serialize
// calls so that we are guaranteed to always have at least one
// parameter of a type defined in the rn:: namespace, that way we
// can always rely on ADL to find serialize methods that we de-
// clare throughout the codebase even if they are declared after
// the ones below that might use them.
struct rn_adl_tag {};
} // namespace rn

namespace rn::serial {

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

namespace detail {

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
auto serialize( FBBuilder&, T const& o, ::rn::rn_adl_tag ) {
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
auto serialize( FBBuilder&, T const& o, ::rn::rn_adl_tag ) {
  struct Ret {
    int o_;
    int get() const { return o_; }
  };
  return Ret{o._};
}

// For enums.
template<typename T, decltype( serialize_enum(
                         std::declval<T>() ) )* = nullptr>
auto serialize( FBBuilder&, T const& o, ::rn::rn_adl_tag ) {
  return ReturnValue{serialize_enum( o )};
}

// For C++ classes/structs that get serialized as FB structs.
template<typename T, decltype( &T::serialize_struct )* = nullptr>
auto serialize( FBBuilder& builder, T const& o,
                ::rn::rn_adl_tag ) {
  return ReturnAddress{o.serialize_struct( builder )};
}

// For C++ classes/structs that get serialized as FB tables.
template<typename T, decltype( &T::serialize_table )* = nullptr>
auto serialize( FBBuilder& builder, T const& o,
                ::rn::rn_adl_tag ) {
  return ReturnValue{o.serialize_table( builder )};
}

// For C++ classes/structs that get serialized as FB structs.
template<typename T,               //
         std::enable_if_t<         //
             mp::is_optional_v<T>, //
             int> = 0              //
         >
auto serialize( FBBuilder& builder, T const& o,
                ::rn::rn_adl_tag ) {
  auto factory =
      detail::opt_value_type_to_fb<typename T::value_type>;
  if( o.has_value() ) {
    auto s_value = serialize( builder, *o, ::rn::rn_adl_tag{} );
    return ReturnValue{
        factory( builder, /*has_value=*/true, s_value.get() )};
  } else {
    return ReturnValue{
        factory( builder, /*has_value=*/false, {} )};
  }
}

// For vectors.
template<typename T,             //
         std::enable_if_t<       //
             mp::is_vector_v<T>, //
             int> = 0            //
         >
auto serialize( FBBuilder& builder, T const& o,
                ::rn::rn_adl_tag ) {
  using fb_wrapper_elem_t = std::decay_t<decltype(
      serialize( builder, *o.begin(), ::rn::rn_adl_tag{} ) )>;
  using fb_get_elem_t =
      decltype( std::declval<fb_wrapper_elem_t>().get() );
  // This vector must live until we create the final fb vector.
  std::vector<fb_wrapper_elem_t> wrappers;
  wrappers.reserve( o.size() );
  for( auto const& e : o )
    wrappers.push_back(
        serialize( builder, e, ::rn::rn_adl_tag{} ) );
  // This vector may hold pointers into the previous one if we're
  // dealing with structs.
  std::vector<fb_get_elem_t> gotten;
  gotten.reserve( o.size() );
  for( auto const& e : wrappers ) gotten.push_back( e.get() );
  return ReturnValue{builder.CreateVector( gotten )};
}

} // namespace rn::serial

/****************************************************************
** Table Macros
*****************************************************************/
#define SERIAL_CALL_SERIALIZE_TABLE_IMPL( type, var ) \
  auto PP_JOIN( s_, var ) =                           \
      serialize( builder, var##_, ::rn::rn_adl_tag{} )

#define SERIAL_CALL_SERIALIZE_TABLE( p ) \
  SERIAL_CALL_SERIALIZE_TABLE_IMPL p

#define SERIAL_GET_SERIALIZED_IMPL( type, var ) \
  PP_JOIN( s_, var ).get()
#define SERIAL_GET_SERIALIZED( p ) SERIAL_GET_SERIALIZED_IMPL p

#define SERIAL_DECLARE_VAR_TABLE( type, var ) type var##_;

#define SERIALIZABLE_TABLE_MEMBERS_IMPL( name, ... )           \
  PP_MAP_TUPLE( SERIAL_DECLARE_VAR_TABLE, __VA_ARGS__ )        \
public:                                                        \
  FBOffset<fb::name> serialize_table( FBBuilder& builder )     \
      const {                                                  \
    using ::rn::serial::serialize;                             \
    PP_MAP_SEMI( SERIAL_CALL_SERIALIZE_TABLE, __VA_ARGS__ )    \
    return fb::Create##name(                                   \
        builder,                                               \
        PP_MAP_COMMAS( SERIAL_GET_SERIALIZED, __VA_ARGS__ ) ); \
  }                                                            \
                                                               \
private:

#define SERIALIZABLE_TABLE_MEMBERS( ... ) \
  EVAL( SERIALIZABLE_TABLE_MEMBERS_IMPL( __VA_ARGS__ ) )

/****************************************************************
** Struct Macros
*****************************************************************/
#define SERIAL_CALL_SERIALIZE_STRUCT_IMPL( type, var ) \
  auto PP_JOIN( s_, var ) =                            \
      serialize( builder, var, ::rn::rn_adl_tag{} )

#define SERIAL_CALL_SERIALIZE_STRUCT( p ) \
  SERIAL_CALL_SERIALIZE_STRUCT_IMPL p

#define SERIAL_DECLARE_VAR_STRUCT( type, var ) type var{};

#define SERIALIZABLE_STRUCT_MEMBERS_IMPL( name, ... )          \
  PP_MAP_TUPLE( SERIAL_DECLARE_VAR_STRUCT, __VA_ARGS__ )       \
public:                                                        \
  fb::name serialize_struct( FBBuilder& builder ) const {      \
    using ::rn::serial::serialize;                             \
    PP_MAP_SEMI( SERIAL_CALL_SERIALIZE_STRUCT, __VA_ARGS__ )   \
    return fb::name(                                           \
        PP_MAP_COMMAS( SERIAL_GET_SERIALIZED, __VA_ARGS__ ) ); \
  }

#define SERIALIZABLE_STRUCT_MEMBERS( ... ) \
  EVAL( SERIALIZABLE_STRUCT_MEMBERS_IMPL( __VA_ARGS__ ) )

namespace rn {} // namespace rn
