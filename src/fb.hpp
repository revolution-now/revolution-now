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
#include "cc-specific.hpp"
#include "errors.hpp"
#include "meta.hpp"

// Flatbuffers
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/minireflect.h"

// base-util
#include "base-util/mp.hpp"
#include "base-util/pp.hpp"

// C++ standard library
#include <utility>

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
namespace rn::serial {
// This is used as a dummy parameter to all serialize / deseri-
// alize calls so that we are guaranteed to always have at least
// one parameter of a type defined in the rn::serial namespace,
// that way we can always rely on ADL to find serialize methods
// that we declare throughout the codebase even if they are de-
// clared after the ones below that might use them.
//
// This is kind of hacky and probably shouldn't be necessary,
// since in theory we could declare our serialize / deserialize
// calls all in the same (predictable namespace). But we are re-
// lying on ADL (I think) because ADL is the only method that can
// find a function declared after the template that uses it; and
// we sometimes have to declare functions after the template that
// uses it because this file has to be included in headers.
struct rn_adl_tag {};
} // namespace rn::serial

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

namespace detail {

template<typename T>
auto* to_const_ptr( T&& arg ) {
  if constexpr( std::is_pointer_v<std::remove_reference_t<T>> )
    return arg;
  else
    return &arg;
}

template<typename T>
auto ptr_to_ref( T&& t ) {
  if constexpr( std::is_pointer_v<std::remove_reference_t<T>> )
    return *t;
  else
    return std::forward<T>( t );
}

template<typename>
inline constexpr bool is_fb_vector_v = false;

template<typename T>
inline constexpr bool is_fb_vector_v<::flatbuffers::Vector<T>> =
    true;

template<typename...>
struct remove_fb_vector;

template<typename T>
struct remove_fb_vector<flatbuffers::Vector<T>> {
  using type = T;
};

template<typename T>
using remove_fb_vector_t = typename remove_fb_vector<T>::type;

template<typename T>
struct remove_fb_offset {
  using type = T;
};

template<typename U>
struct remove_fb_offset<flatbuffers::Offset<U>> {
  using type = U;
};

template<typename T>
using remove_fb_offset_t = typename remove_fb_offset<T>::type;

// Takes the return type of a FB getter and converts it to a type
// suitable for passing as a Hint to the `serialize` methods.
template<typename T>
using fb_serialize_hint_t =        //
    remove_fb_offset_t<            //
        std::decay_t<              //
            std::remove_pointer_t< //
                std::decay_t<T>    //
                >                  //
            >                      //
        >;

template<typename T, typename = void>
inline constexpr bool has_create_v = false;

template<typename T>
inline constexpr bool
    has_create_v<T, std::void_t<decltype( T::Create )>> = true;

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
auto serialize( FBBuilder&, T const& o,
                ::rn::serial::rn_adl_tag ) {
  return ReturnValue{o};
}

// For typed ints.
template<typename Hint,                             //
         typename T,                                //
         std::enable_if_t<                          //
             std::is_same_v<int, decltype( T::_ )>, //
             int> = 0                               //
         >
auto serialize( FBBuilder&, T const& o,
                ::rn::serial::rn_adl_tag ) {
  return ReturnValue{o._};
}

// For strings.
template<typename Hint>
auto serialize( FBBuilder& builder, std::string const& o,
                ::rn::serial::rn_adl_tag ) {
  auto offset = builder.CreateString( o );
  return ReturnValue{offset};
}

// For enums.
template<
    typename Hint, //
    typename T,
    decltype( serialize_enum( std::declval<T>() ) )* = nullptr>
auto serialize( FBBuilder&, T const& o,
                ::rn::serial::rn_adl_tag ) {
  return ReturnValue{serialize_enum( o )};
}

// For C++ classes/structs that get serialized as FB structs.
template<typename Hint, //
         typename T, decltype( &T::serialize_struct )* = nullptr>
auto serialize( FBBuilder& builder, T const& o,
                ::rn::serial::rn_adl_tag ) {
  return ReturnAddress{o.serialize_struct( builder )};
}

// For C++ classes/structs that get serialized as FB tables.
template<typename Hint, //
         typename T, decltype( &T::serialize_table )* = nullptr>
auto serialize( FBBuilder& builder, T const& o,
                ::rn::serial::rn_adl_tag ) {
  return ReturnValue{o.serialize_table( builder )};
}

// For std::reference_wrapper.
template<typename Hint, typename T>
auto serialize( FBBuilder&                       builder,
                std::reference_wrapper<T> const& rr,
                ::rn::serial::rn_adl_tag ) {
  return serialize<Hint>( builder, rr.get(),
                          ::rn::serial::rn_adl_tag{} );
}

// For std::optional.
template<typename Hint, typename T>
auto serialize( FBBuilder& builder, std::optional<T> const& o,
                ::rn::serial::rn_adl_tag ) {
  if( o.has_value() ) {
    auto s_value = serialize<void>( builder, *o,
                                    ::rn::serial::rn_adl_tag{} );
    return ReturnValue{Hint::Create( builder, /*has_value=*/true,
                                     s_value.get() )};
  } else {
    return ReturnValue{
        Hint::Create( builder, /*has_value=*/false, {} )};
  }
}

// For pairs.
template<typename Hint, typename F, typename S>
auto serialize( FBBuilder& builder, std::pair<F, S> const& o,
                ::rn::serial::rn_adl_tag ) {
  // Assume we don't need to supply hints for components.
  auto const& s_fst = serialize<void>(
      builder, o.first, ::rn::serial::rn_adl_tag{} );
  auto const& s_snd = serialize<void>(
      builder, o.second, ::rn::serial::rn_adl_tag{} );
  if constexpr( detail::has_create_v<Hint> )
    return ReturnValue{
        Hint::Create( builder, s_fst.get(), s_snd.get() )};
  else
    return ReturnAddress{
        Hint( detail::ptr_to_ref( s_fst.get() ),
              detail::ptr_to_ref( s_snd.get() ) )};
}

// For vectors.
template<typename Hint, typename T>
auto serialize( FBBuilder& builder, std::vector<T> const& o,
                ::rn::serial::rn_adl_tag ) {
  using namespace detail;
  using inner_hint_t =
      fb_serialize_hint_t<remove_fb_vector_t<Hint>>;
  using fb_wrapper_elem_t =
      std::decay_t<decltype( serialize<inner_hint_t>(
          builder, *o.begin(), ::rn::serial::rn_adl_tag{} ) )>;
  using fb_get_elem_t =
      decltype( std::declval<fb_wrapper_elem_t const&>().get() );
  // This vector must live until we create the final fb vector.
  std::vector<fb_wrapper_elem_t> wrappers;
  wrappers.reserve( o.size() );
  for( auto const& e : o )
    wrappers.emplace_back( serialize<inner_hint_t>(
        builder, e, ::rn::serial::rn_adl_tag{} ) );
  if constexpr( std::is_pointer_v<fb_get_elem_t> ) {
    // This is a struct.
    std::vector<inner_hint_t> values;
    values.reserve( o.size() );
    for( auto const& e : wrappers )
      values.emplace_back( *e.get() );
    return ReturnValue{builder.CreateVectorOfStructs( values )};
  } else {
    // This is anything other than a struct, such as a table,
    // scalar, etc.
    std::vector<fb_get_elem_t> gotten;
    gotten.reserve( o.size() );
    for( auto const& e : wrappers ) gotten.push_back( e.get() );
    return ReturnValue{builder.CreateVector( gotten )};
  }
}

// For map-like things.
template<typename Hint,                        //
         typename T,                           //
         std::enable_if_t<                     //
             mp::is_map_like<T> &&             //
                 detail::is_fb_vector_v<Hint>, //
             int> = 0                          //
         >
auto serialize( FBBuilder& builder, T const& m,
                ::rn::serial::rn_adl_tag ) {
  std::vector<CRef<typename T::value_type>> v;
  v.reserve( m.size() );
  for( auto const& p : m ) v.emplace_back( p );
  return serialize<Hint>( builder, v,
                          ::rn::serial::rn_adl_tag{} );
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
                      ::rn::serial::rn_adl_tag ) {
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
                      ::rn::serial::rn_adl_tag ) {
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
                      ::rn::serial::rn_adl_tag ) {
  DCHECK( src != nullptr,
          "`src` is nullptr when deserializing typed int." );
  *dst = DstT{*src};
  return xp_success_t{};
}

// For strings.
template<typename SrcT>
expect<> deserialize( SrcT const* src, std::string* dst,
                      ::rn::serial::rn_adl_tag ) {
  if( src == nullptr ) return xp_success_t{};
  *dst = src->str();
  return xp_success_t{};
}

// For C++ classes/structs that get serialized as FB structs.
template<typename SrcT, //
         typename DstT, //
         decltype( &DstT::deserialize_struct )* = nullptr>
expect<> deserialize( SrcT const* src, DstT* dst,
                      ::rn::serial::rn_adl_tag ) {
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
                      ::rn::serial::rn_adl_tag ) {
  if( src == nullptr ) return xp_success_t{};
  if( auto xp = DstT::deserialize_table( *src, dst ); !xp )
    return xp;
  return dst->check_invariants_safe();
}

// For std::optional.
template<typename SrcT, typename T>
expect<> deserialize( SrcT const* src, std::optional<T>* dst,
                      ::rn::serial::rn_adl_tag ) {
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
  return deserialize( detail::to_const_ptr( src->value() ),
                      std::addressof( **dst ),
                      ::rn::serial::rn_adl_tag{} );
}

// For std::reference_wrapper.
template<typename SrcT, typename T>
expect<> deserialize( SrcT const*                src,
                      std::reference_wrapper<T>* dst,
                      ::rn::serial::rn_adl_tag ) = delete;

// For pairs.
template<typename SrcT, typename F, typename S>
expect<> deserialize( SrcT const* src, std::pair<F, S>* dst,
                      ::rn::serial::rn_adl_tag ) {
  if constexpr( std::is_pointer_v<decltype( src->fst() )> ) {
    if( src->fst() == nullptr )
      return UNEXPECTED( "pair has no `fst` value." );
  }
  if constexpr( std::is_pointer_v<decltype( src->snd() )> ) {
    if( src->snd() == nullptr )
      return UNEXPECTED( "pair has no `snd` value." );
  }
  XP_OR_RETURN_( deserialize( detail::to_const_ptr( src->fst() ),
                              &dst->first,
                              ::rn::serial::rn_adl_tag{} ) );
  XP_OR_RETURN_( deserialize( detail::to_const_ptr( src->snd() ),
                              &dst->second,
                              ::rn::serial::rn_adl_tag{} ) );
  return xp_success_t{};
}

// For vectors.
template<typename SrcT, typename T>
expect<> deserialize( SrcT const* src, std::vector<T>* dst,
                      ::rn::serial::rn_adl_tag ) {
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
        deserialize( detail::to_const_ptr( elem ),
                     std::addressof( dst->operator[]( i ) ),
                     ::rn::serial::rn_adl_tag{} ) );
  }
  return xp_success_t{};
}

// For map-like things.
template<typename SrcT,                        //
         typename DstT,                        //
         std::enable_if_t<                     //
             mp::is_map_like<DstT> &&          //
                 detail::is_fb_vector_v<SrcT>, //
             int> = 0                          //
         >
expect<> deserialize( SrcT const* src, DstT* m,
                      ::rn::serial::rn_adl_tag ) {
  // SrcT should be a flatbuffers::Vector.
  if( src == nullptr || src->size() == 0 ) {
    // `dst` should be in its default-constructed state, which is
    // an empty map.
    return xp_success_t{};
  }
  using iter_t = decltype( src->size() );
  using key_t  = typename DstT::key_type;
  if constexpr( mp::has_reserve_method<std::decay_t<DstT>> ) {
    m->reserve( src->size() );
  }
  for( iter_t i = 0; i < src->size(); ++i ) {
    auto* elem = src->Get( i );
    key_t key{};
    XP_OR_RETURN_(
        deserialize( detail::to_const_ptr( elem->fst() ), &key,
                     ::rn::serial::rn_adl_tag{} ) );

    // FIXME
    // if constexpr( is_fmtable<key_t> ) {
    //  if( m->find( key ) != m->end() )
    //    return UNEXPECTED(
    //        "duplicate key ({}) found when deserializing map.",
    //        key );
    //} else {
    if( m->find( key ) != m->end() )
      return UNEXPECTED(
          "duplicate key (type {}) found when deserializing "
          "map.",
          ::rn::demangled_typename<key_t>() );
    //}

    XP_OR_RETURN_(
        deserialize( detail::to_const_ptr( elem->snd() ),
                     std::addressof( m->operator[]( key ) ),
                     ::rn::serial::rn_adl_tag{} ) );
  }
  return xp_success_t{};
}

} // namespace rn::serial

/****************************************************************
** Table Macros
*****************************************************************/
#define SERIAL_CALL_SERIALIZE_TABLE_IMPL( type, var )      \
  auto PP_JOIN( s_, var ) =                                \
      serialize<::rn::serial::detail::fb_serialize_hint_t< \
          decltype( std::declval<fb_target_t>().var() )>>( \
          builder, var, ::rn::serial::rn_adl_tag{} )

#define SERIAL_CALL_SERIALIZE_TABLE( p ) \
  SERIAL_CALL_SERIALIZE_TABLE_IMPL p

#define SERIAL_GET_SERIALIZED_IMPL( type, var ) \
  PP_JOIN( s_, var ).get()
#define SERIAL_GET_SERIALIZED( p ) SERIAL_GET_SERIALIZED_IMPL p

#define SERIAL_DECLARE_VAR_TABLE( type, var ) type var;

#define SERIAL_DESERIALIZE_VAR_TABLE_IMPL( type, var )        \
  XP_OR_RETURN_(                                              \
      deserialize( serial::detail::to_const_ptr( src.var() ), \
                   &dst->var, ::rn::serial::rn_adl_tag{} ) );

#define SERIAL_DESERIALIZE_VAR_TABLE( p ) \
  SERIAL_DESERIALIZE_VAR_TABLE_IMPL p

#define SERIALIZABLE_TABLE_MEMBERS_IMPL( name, ... )           \
  PP_MAP_TUPLE( SERIAL_DECLARE_VAR_TABLE, __VA_ARGS__ )        \
public:                                                        \
  using fb_target_t = fb::name;                                \
  static constexpr std::string_view fb_root_type_name =        \
      "fb." #name;                                             \
  FBOffset<fb::name> serialize_table( FBBuilder& builder )     \
      const {                                                  \
    (void)fb_root_type_name;                                   \
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
#define SERIAL_CALL_SERIALIZE_STRUCT_IMPL( type, var )     \
  auto PP_JOIN( s_, var ) =                                \
      serialize<::rn::serial::detail::fb_serialize_hint_t< \
          decltype( std::declval<fb_target_t>().var() )>>( \
          builder, var, ::rn::serial::rn_adl_tag{} )

#define SERIAL_CALL_SERIALIZE_STRUCT( p ) \
  SERIAL_CALL_SERIALIZE_STRUCT_IMPL p

#define SERIAL_DECLARE_VAR_STRUCT( type, var ) type var{};

#define SERIAL_DESERIALIZE_VAR_STRUCT_IMPL( type, var )       \
  XP_OR_RETURN_(                                              \
      deserialize( serial::detail::to_const_ptr( src.var() ), \
                   &dst->var, ::rn::serial::rn_adl_tag{} ) );

#define SERIAL_DESERIALIZE_VAR_STRUCT( p ) \
  SERIAL_DESERIALIZE_VAR_STRUCT_IMPL p

#define SERIALIZABLE_STRUCT_MEMBERS_IMPL( name, ... )          \
  PP_MAP_TUPLE( SERIAL_DECLARE_VAR_STRUCT, __VA_ARGS__ )       \
public:                                                        \
  using fb_target_t = fb::name;                                \
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
