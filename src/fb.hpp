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
#include "errors.hpp"
#include "meta.hpp"

// Flatbuffers
#include "fb/vocab_generated.h"

// base-util
#include "base-util/mp.hpp"
#include "base-util/pp.hpp"

/****************************************************************
** Enums
*****************************************************************/
#define SERIALIZABLE_ENUM( name )                              \
  static_assert( static_cast<int>( fb::name::MIN ) == 0 );     \
  static_assert( name::_size() ==                              \
                 static_cast<size_t>( fb::name::MAX ) + 1 );   \
  static_assert(                                               \
      static_cast<int>(                                        \
          ( *std::begin( values<name> ) )._value ) == 0 );     \
  inline fb::name serialize_enum( name e ) {                   \
    auto from_idx = e._value;                                  \
    RN_CHECK( from_idx >= 0 &&                                 \
              from_idx <= static_cast<int>( fb::name::MAX ) ); \
    DCHECK( std::string( fb::EnumNames##name()[from_idx] ) ==  \
                e._to_string(),                                \
            "{} != {}", fb::EnumNames##name()[from_idx],       \
            e._to_string() );                                  \
    return fb::EnumValues##name()[from_idx];                   \
  }                                                            \
  inline expect<> deserialize_enum( fb::name e, name* dst ) {  \
    auto from_idx = static_cast<int>( e );                     \
    if( from_idx < 0 ||                                        \
        from_idx > static_cast<int>( fb::name::MAX ) ) {       \
      return UNEXPECTED(                                       \
          "serialized enum of type fb::" #name                 \
          " has index out of its own range (idx={})",          \
          from_idx );                                          \
    }                                                          \
    auto res = name::_from_index_nothrow( from_idx );          \
    if( !res ) {                                               \
      return UNEXPECTED(                                       \
          "serialized enum of type fb::" #name                 \
          " has index that is outside the range of the "       \
          "corresponding native type (idx={}).",               \
          from_idx );                                          \
    }                                                          \
    char const* name1 = ( *res )._to_string();                 \
    char const* name2 = fb::EnumNames##name()[from_idx];       \
    if( std::strcmp( name1, name2 ) != 0 ) {                   \
      return UNEXPECTED(                                       \
          "error while deserializing enum of type " #name      \
          ": {} != {}",                                        \
          name1, name2 );                                      \
    }                                                          \
    *dst = *res;                                               \
    return xp_success_t{};                                     \
  }

/****************************************************************
** Uniform serialization interface.
*****************************************************************/
namespace rn {
// This is used as a dummy parameter to all serialize / deseri-
// alize calls so that we are guaranteed to always have at least
// one parameter of a type defined in the rn:: namespace, that
// way we can always rely on ADL to find serialize methods that
// we declare throughout the codebase even if they are declared
// after the ones below that might use them.
//
// This is kind of hacky and probably shouldn't be necessary,
// since in theory we could declare our serialize / deserialize
// calls all in the same (predictable namespace). But we are re-
// lying on ADL (I think) because ADL is the only method that can
// find a function declared after the template that uses it; and
// we sometimes have to declare functions after the template that
// uses it because this file has to be included in headers.
struct rn_adl_tag {};
} // namespace rn

namespace rn::serial {

/****************************************************************
** Helpers
*****************************************************************/
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

template<typename T,                //
         std::enable_if_t<          //
             !std::is_pointer_v<T>, //
             int> = 0               //
         >
auto to_const_ptr( T const& arg ) {
  return &arg;
}

template<typename T,               //
         std::enable_if_t<         //
             std::is_pointer_v<T>, //
             int> = 0              //
         >
auto to_const_ptr( T arg ) {
  return arg;
}

// Takes the return type of a FB getter and converts it to a type
// suitable for passing as a Hint to the `serialize` methods.
template<typename T>
using fb_serialize_hint_t =    //
    std::decay_t<              //
        std::remove_pointer_t< //
            std::decay_t<T>    //
            >                  //
        >;

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

/****************************************************************
** serialize()
*****************************************************************/
// First template parameter is a hint for which flattbuffers type
// to serialize to, but in many cases it is not used/needed. It's
// only needed in those cases where we have some kind of generic
// C++ type (like a map) that might need to be serialized to mul-
// tiple different kinds of flatbuffers types, and which type is
// only known by looking at the target flatbuffers type during
// serialization.

// For scalars (non-enums).
template<typename Hint,           //
         typename T,              //
         std::enable_if_t<        //
             std::is_scalar_v<T>, //
             int> = 0             //
         >
auto serialize( FBBuilder&, T const& o, ::rn::rn_adl_tag ) {
  return ReturnValue{o};
}

// For typed ints.
template<typename Hint,                             //
         typename T,                                //
         std::enable_if_t<                          //
             std::is_same_v<int, decltype( T::_ )>, //
             int> = 0                               //
         >
auto serialize( FBBuilder&, T const& o, ::rn::rn_adl_tag ) {
  return ReturnValue{o._};
}

// For strings.
template<typename Hint,                      //
         typename T,                         //
         std::enable_if_t<                   //
             std::is_same_v<std::string, T>, //
             int> = 0                        //
         >
auto serialize( FBBuilder& builder, T const& o,
                ::rn::rn_adl_tag ) {
  auto offset = builder.CreateString( o );
  return ReturnValue{offset};
}

// For enums.
template<
    typename Hint, //
    typename T,
    decltype( serialize_enum( std::declval<T>() ) )* = nullptr>
auto serialize( FBBuilder&, T const& o, ::rn::rn_adl_tag ) {
  return ReturnValue{serialize_enum( o )};
}

// For C++ classes/structs that get serialized as FB structs.
template<typename Hint, //
         typename T, decltype( &T::serialize_struct )* = nullptr>
auto serialize( FBBuilder& builder, T const& o,
                ::rn::rn_adl_tag ) {
  return ReturnAddress{o.serialize_struct( builder )};
}

// For C++ classes/structs that get serialized as FB tables.
template<typename Hint, //
         typename T, decltype( &T::serialize_table )* = nullptr>
auto serialize( FBBuilder& builder, T const& o,
                ::rn::rn_adl_tag ) {
  return ReturnValue{o.serialize_table( builder )};
}

// For std::optional. FIXME: use Hint type and static Create
// functions to automate creation of serialized type.
template<typename Hint,            //
         typename T,               //
         std::enable_if_t<         //
             mp::is_optional_v<T>, //
             int> = 0              //
         >
auto serialize( FBBuilder& builder, T const& o,
                ::rn::rn_adl_tag ) {
  auto factory =
      detail::opt_value_type_to_fb<typename T::value_type>;
  if( o.has_value() ) {
    auto s_value =
        serialize<void>( builder, *o, ::rn::rn_adl_tag{} );
    return ReturnValue{
        factory( builder, /*has_value=*/true, s_value.get() )};
  } else {
    return ReturnValue{
        factory( builder, /*has_value=*/false, {} )};
  }
}

// For vectors. FIXME: does not support extracting hint type for
// the contained type.
template<typename Hint,          //
         typename T,             //
         std::enable_if_t<       //
             mp::is_vector_v<T>, //
             int> = 0            //
         >
auto serialize( FBBuilder& builder, T const& o,
                ::rn::rn_adl_tag ) {
  using fb_wrapper_elem_t =
      std::decay_t<decltype( serialize<void>(
          builder, *o.begin(), ::rn::rn_adl_tag{} ) )>;
  using fb_get_elem_t =
      decltype( std::declval<fb_wrapper_elem_t const&>().get() );
  // This vector must live until we create the final fb vector.
  std::vector<fb_wrapper_elem_t> wrappers;
  wrappers.reserve( o.size() );
  for( auto const& e : o )
    wrappers.push_back(
        serialize<void>( builder, e, ::rn::rn_adl_tag{} ) );
  // This vector may hold pointers into the previous one if we're
  // dealing with structs.
  std::vector<fb_get_elem_t> gotten;
  gotten.reserve( o.size() );
  for( auto const& e : wrappers ) gotten.push_back( e.get() );
  return ReturnValue{builder.CreateVector( gotten )};
}

// For map-like things.
template<typename Hint,          //
         typename T,             //
         std::enable_if_t<       //
             mp::is_map_like<T>, //
             int> = 0            //
         >
auto serialize( FBBuilder&, T const&, ::rn::rn_adl_tag ) {
  // using fb_key_wrapper_elem_t = std::decay_t<decltype(
  //    serialize( builder, std::declval<typename T::key_type>(),
  //               ::rn::rn_adl_tag{} ) )>;
  // using fb_val_wrapper_elem_t = std::decay_t<decltype(
  // serialize(
  //    builder, std::declval<typename T::mapped_type>(),
  //    ::rn::rn_adl_tag{} ) )>;
  // using fb_key_get_elem_t     = decltype(
  //    std::declval<fb_key_wrapper_elem_t const&>().get() );
  // using fb_val_get_elem_t = decltype(
  //    std::declval<fb_val_wrapper_elem_t const&>().get() );
  //// This vector must live until we create the final fb vector.
  // std::vector<fb_wrapper_elem_t> wrappers;
  // wrappers.reserve( o.size() );
  // for( auto const& e : o )
  //  wrappers.push_back(
  //      serialize( builder, e, ::rn::rn_adl_tag{} ) );
  //// This vector may hold pointers into the previous one if
  /// we're / dealing with structs.
  // std::vector<fb_get_elem_t> gotten;
  // gotten.reserve( o.size() );
  // for( auto const& e : wrappers ) gotten.push_back( e.get() );
  // return ReturnValue{builder.CreateVector( gotten )};
}

/****************************************************************
** deserialize()
*****************************************************************/
// For scalars (non-enums).
template<typename SrcT,                    //
         typename DstT,                    //
         std::enable_if_t<                 //
             std::is_scalar_v<SrcT> &&     //
                 std::is_scalar_v<DstT> && //
                 std::is_same_v<std::decay_t<SrcT>,
                                std::decay_t<DstT>>, //
             int> = 0                                //
         >
expect<> deserialize( SrcT const* src, DstT* dst,
                      ::rn::rn_adl_tag ) {
  DCHECK( src != nullptr,
          "`src` is nullptr when deserializing scalar." );
  *dst = *src;
  return xp_success_t{};
}

// For enums.
template<
    typename SrcT, //
    typename DstT, //
    decltype( deserialize_enum(
        std::declval<SrcT>(),
        std::add_pointer_t<std::decay_t<DstT>>{} ) )* = nullptr>
expect<> deserialize( SrcT const* src, DstT* dst,
                      ::rn::rn_adl_tag ) {
  DCHECK( src != nullptr,
          "`src` is nullptr when deserializing enum." );
  return deserialize_enum( *src, dst );
}

// For typed ints.
template<typename SrcT,                                //
         typename DstT,                                //
         std::enable_if_t<                             //
             std::is_same_v<int, decltype( DstT::_ )>, //
             int> = 0                                  //
         >
expect<> deserialize( SrcT const* src, DstT* dst,
                      ::rn::rn_adl_tag ) {
  DCHECK( src != nullptr,
          "`src` is nullptr when deserializing typed int." );
  *dst = DstT{*src};
  return xp_success_t{};
}

// For strings.
template<typename SrcT,                         //
         typename DstT,                         //
         std::enable_if_t<                      //
             std::is_same_v<std::string, DstT>, //
             int> = 0                           //
         >
expect<> deserialize( SrcT const* src, DstT* dst,
                      ::rn::rn_adl_tag ) {
  if( src == nullptr ) return xp_success_t{};
  *dst = src->str();
  return xp_success_t{};
}

// For C++ classes/structs that get serialized as FB structs.
template<typename SrcT, //
         typename DstT, //
         decltype( &DstT::deserialize_struct )* = nullptr>
expect<> deserialize( SrcT const* src, DstT* dst,
                      ::rn::rn_adl_tag ) {
  if( src == nullptr ) return xp_success_t{};
  if( auto xp = DstT::deserialize_struct( *src, dst ); !xp )
    return xp;
  return dst->check_invariants_safe();
}

// For C++ classes/structs that get serialized as FB tables.
template<typename SrcT, //
         typename DstT, //
         decltype( &DstT::deserialize_table )* = nullptr>
expect<> deserialize( SrcT const* src, DstT* dst,
                      ::rn::rn_adl_tag ) {
  if( src == nullptr ) return xp_success_t{};
  if( auto xp = DstT::deserialize_table( *src, dst ); !xp )
    return xp;
  return dst->check_invariants_safe();
}

// For std::optional.
template<typename SrcT,               //
         typename DstT,               //
         std::enable_if_t<            //
             mp::is_optional_v<DstT>, //
             int> = 0                 //
         >
expect<> deserialize( SrcT const* src, DstT* dst,
                      ::rn::rn_adl_tag ) {
  if( src == nullptr || !src->has_value() ) {
    // `dst` should be in its default-constructed state, which is
    // nullopt if it's an optional.
    return xp_success_t{};
  }
  if constexpr( std::is_pointer_v<decltype( src->value() )> ) {
    if( src->value() == nullptr )
      return UNEXPECTED(
          "optional has no `value` but has `has_value` == "
          "true." );
  }
  dst->emplace(); // default construct the value.
  return deserialize( to_const_ptr( src->value() ),
                      std::addressof( **dst ),
                      ::rn::rn_adl_tag{} );
}

// For vectors.
template<typename SrcT,             //
         typename DstT,             //
         std::enable_if_t<          //
             mp::is_vector_v<DstT>, //
             int> = 0               //
         >
expect<> deserialize( SrcT const* src, DstT* dst,
                      ::rn::rn_adl_tag ) {
  // SrcT should be a flatbuffers::Vector.
  if( src == nullptr || src->size() == 0 ) {
    // `dst` should be in its default-constructed state, which is
    // an empty vector.
    return xp_success_t{};
  }
  dst->resize( src->size() );
  using iter_t = decltype( src->size() );
  for( iter_t i = 0; i < src->size(); ++i ) {
    // This should be a value (scalar) or a pointer.
    auto elem = src->Get( i );
    XP_OR_RETURN_(
        deserialize( to_const_ptr( elem ),
                     std::addressof( dst->operator[]( i ) ),
                     ::rn::rn_adl_tag{} ) );
  }
  return xp_success_t{};
}

} // namespace rn::serial

/****************************************************************
** Table Macros
*****************************************************************/
#define SERIAL_CALL_SERIALIZE_TABLE_IMPL( type, var )       \
  auto PP_JOIN( s_, var ) =                                 \
      serialize<::rn::serial::fb_serialize_hint_t<decltype( \
          std::declval<FBTargetType>().var() )>>(           \
          builder, var##_, ::rn::rn_adl_tag{} )

#define SERIAL_CALL_SERIALIZE_TABLE( p ) \
  SERIAL_CALL_SERIALIZE_TABLE_IMPL p

#define SERIAL_GET_SERIALIZED_IMPL( type, var ) \
  PP_JOIN( s_, var ).get()
#define SERIAL_GET_SERIALIZED( p ) SERIAL_GET_SERIALIZED_IMPL p

#define SERIAL_DECLARE_VAR_TABLE( type, var ) type var##_;

#define SERIAL_DESERIALIZE_VAR_TABLE_IMPL( type, var ) \
  XP_OR_RETURN_(                                       \
      deserialize( serial::to_const_ptr( src.var() ),  \
                   &dst->var##_, ::rn::rn_adl_tag{} ) );

#define SERIAL_DESERIALIZE_VAR_TABLE( p ) \
  SERIAL_DESERIALIZE_VAR_TABLE_IMPL p

#define SERIALIZABLE_TABLE_MEMBERS_IMPL( name, ... )           \
  PP_MAP_TUPLE( SERIAL_DECLARE_VAR_TABLE, __VA_ARGS__ )        \
public:                                                        \
  using FBTargetType = fb::name;                               \
  FBOffset<fb::name> serialize_table( FBBuilder& builder )     \
      const {                                                  \
    using ::rn::serial::serialize;                             \
    PP_MAP_SEMI( SERIAL_CALL_SERIALIZE_TABLE, __VA_ARGS__ )    \
    return fb::Create##name(                                   \
        builder,                                               \
        PP_MAP_COMMAS( SERIAL_GET_SERIALIZED, __VA_ARGS__ ) ); \
  }                                                            \
  static expect<> deserialize_table( fb::name const& src,      \
                                     name*           dst ) {             \
    DCHECK( dst );                                             \
    using ::rn::serial::deserialize;                           \
    PP_MAP_SEMI( SERIAL_DESERIALIZE_VAR_TABLE, __VA_ARGS__ )   \
    return xp_success_t{};                                     \
  }                                                            \
                                                               \
private:

#define SERIALIZABLE_TABLE_MEMBERS( ... ) \
  EVAL( SERIALIZABLE_TABLE_MEMBERS_IMPL( __VA_ARGS__ ) )

/****************************************************************
** Struct Macros
*****************************************************************/
#define SERIAL_CALL_SERIALIZE_STRUCT_IMPL( type, var )      \
  auto PP_JOIN( s_, var ) =                                 \
      serialize<::rn::serial::fb_serialize_hint_t<decltype( \
          std::declval<FBTargetType>().var() )>>(           \
          builder, var, ::rn::rn_adl_tag{} )

#define SERIAL_CALL_SERIALIZE_STRUCT( p ) \
  SERIAL_CALL_SERIALIZE_STRUCT_IMPL p

#define SERIAL_DECLARE_VAR_STRUCT( type, var ) type var{};

#define SERIAL_DESERIALIZE_VAR_STRUCT_IMPL( type, var ) \
  XP_OR_RETURN_(                                        \
      deserialize( serial::to_const_ptr( src.var() ),   \
                   &dst->var, ::rn::rn_adl_tag{} ) );

#define SERIAL_DESERIALIZE_VAR_STRUCT( p ) \
  SERIAL_DESERIALIZE_VAR_STRUCT_IMPL p

#define SERIALIZABLE_STRUCT_MEMBERS_IMPL( name, ... )          \
  PP_MAP_TUPLE( SERIAL_DECLARE_VAR_STRUCT, __VA_ARGS__ )       \
public:                                                        \
  using FBTargetType = fb::name;                               \
  fb::name serialize_struct( FBBuilder& builder ) const {      \
    using ::rn::serial::serialize;                             \
    PP_MAP_SEMI( SERIAL_CALL_SERIALIZE_STRUCT, __VA_ARGS__ )   \
    return fb::name(                                           \
        PP_MAP_COMMAS( SERIAL_GET_SERIALIZED, __VA_ARGS__ ) ); \
  }                                                            \
  static expect<> deserialize_struct( fb::name const& src,     \
                                      name*           dst ) {            \
    DCHECK( dst );                                             \
    using ::rn::serial::deserialize;                           \
    PP_MAP_SEMI( SERIAL_DESERIALIZE_VAR_STRUCT, __VA_ARGS__ )  \
    return xp_success_t{};                                     \
  }

#define SERIALIZABLE_STRUCT_MEMBERS( ... ) \
  EVAL( SERIALIZABLE_STRUCT_MEMBERS_IMPL( __VA_ARGS__ ) )

namespace rn {} // namespace rn
