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
#include "error.hpp"
#include "expect.hpp"
#include "fmt-helper.hpp"
#include "maybe.hpp"

// Rnl
#include "rnl/helper/enum.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/meta.hpp"
#include "base/variant.hpp"

// Flatbuffers
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/minireflect.h"

// base-util
#include "base-util/mp.hpp"
#include "base-util/pp.hpp"

// C++ standard library
#include <list>
#include <queue>
#include <tuple>
#include <unordered_set>
#include <utility>

namespace rn {

/****************************************************************
** Result of Deserialization Routines.
*****************************************************************/
using DeserialError = ::base::generic_err;

using valid_deserial_t = valid_or<DeserialError>;

inline valid_deserial_t invalid_deserial(
    std::string_view msg,
    base::SourceLoc  loc = base::SourceLoc::current() ) {
  return ::base::GenericError::create( msg, loc );
}

#define VERIFY_DESERIAL( cond, msg ) \
  if( !( cond ) )                    \
    return invalid_deserial( msg, base::SourceLoc::current() );

} // namespace rn

/****************************************************************
** Uniform serialization interface.
*****************************************************************/
namespace rn::serial {

using FBBuilder = ::flatbuffers::FlatBufferBuilder;
template<typename T>
using FBOffset = ::flatbuffers::Offset<T>;

/****************************************************************
** Serialization ADL Helper
*****************************************************************/
// In general, a serialize / deserialize method (say, for a
// container) will need to call another serialize / deserialize
// method on the contained type, and this contained type depends
// on a template parameter which is not known until the point of
// instantiation. This can sometimes be a problem because the
// serialize / deserialize method for the contained type might
// have been declared after the one for the container type; this
// is because these methods are scattered about the code base and
// it is hard and/or undesirable to either forward declare or to
// precisely control the order. In those cases, the serialize /
// deserialize method for the contained type will not be found in
// phase I of the Two-Phase-Lookup mechanism; it can only be
// found in phase II, which is ADL-only.
//
// However, sometimes a serialize / deserialize method for a
// contained type (e.g., std::variant) will not contain any
// parameters whose types are defined in the rn::serial
// namespace, and so therefore even ADL will fail to find the
// serialize / deserialize method.
//
// To resolve this, we introduce the following tag (dummy)
// parameter to all serialize / deserialize calls so that we
// are guaranteed to always have at least one argument of a type
// defined in the rn::serial namespace. This allows the compiler
// to always be able to find the serialize / deserialize function
// (at least) using ADL.
//
// From a C++ expert:
//
//   For an unqualified name to be found in a template
//   instantiation, it must either have been declared before the
//   place where the template was defined, or it must be the name
//   of a function (in a function call expression) that is
//   declared in a namespace associated with one of its
//   arguments.
struct ADL {};

/****************************************************************
** Helpers
*****************************************************************/
std::string ns_to_dots( std::string_view sv );

template<typename SerializedT>
struct ReturnValue {
  SerializedT o_;
  SerializedT get() const { return o_; }
};
template<typename SerializedT>
ReturnValue( SerializedT ) -> ReturnValue<SerializedT>;

template<typename SerializedT>
struct ReturnAddress {
  SerializedT        o_;
  SerializedT const* get() const&& = delete;
  SerializedT const* get() const& {
    // Return the address of the object unless the object is
    // equal to a default-constructed instance, in which case we
    // return nullptr to indicate that the object should not be
    // serialized (this really only applies when it is contained
    // with a parent table wherein it can be optionally serial-
    // ized). Unfortunately, the flatbuffers structs do not have
    // operator== defined, so we have to compare using memcmp.
    // This should work because structs are only suppoed to con-
    // tain POD and inline data members.
    static const auto rhs = SerializedT{};
    auto&             lhs = o_;
    if( std::memcmp( &lhs, &rhs, sizeof( SerializedT ) ) == 0 )
      return nullptr;
    return &o_;
  }
};
template<typename SerializedT>
ReturnAddress( SerializedT ) -> ReturnAddress<SerializedT>;

// Obtains a type list of the parameter types (after the builder)
// that need to be passed to a table's Create method.
template<typename FB>
using fb_creation_tuple_t = mp::tail_t<
    mp::callable_arg_types_t<decltype( FB::Traits::Create )>>;

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

template<typename T, typename = void>
inline constexpr bool has_create_v = false;

template<typename T>
inline constexpr bool
    has_create_v<T, std::void_t<decltype( T::Traits::Create )>> =
        true;

} // namespace detail

/****************************************************************
** [variant] Helpers for (de)serialization of variants.
*****************************************************************/
namespace detail {

// Determine if FB table needs active_index field.
template<typename SrcT, size_t... Idxs>
constexpr bool fb_variant_table_needs_alternative_index(
    std::index_sequence<Idxs...> ) {
  return !mp::and_v<std::is_pointer_v<
      typename SrcT::Traits::template FieldType<Idxs>>...>;
}

template<typename FBVariant, typename Variant>
constexpr auto validate_variant_active_index() {
  constexpr bool has_active_index =
      detail::fb_variant_table_needs_alternative_index<
          FBVariant>( std::make_index_sequence<
                      FBVariant::Traits::fields_number>() );
  using fb_type_list = std::conditional_t<
      has_active_index,
      mp::tail_t<fb_creation_tuple_t<FBVariant>>,
      fb_creation_tuple_t<FBVariant>>;
  if constexpr( has_active_index ) {
    using first_field_t =
        typename FBVariant::Traits::template FieldType<0>;
    constexpr std::string_view first_field_name =
        FBVariant::Traits::field_names[0];
    static_assert(
        first_field_name == "active_index",
        "A Flatbuffers table representing a variant containing "
        "at least one primitive type must have a field called "
        "'active_index' as its first element of type 'int'." );
    static_assert(
        std::is_same_v<first_field_t, int32_t>,
        "A Flatbuffers table representing a variant containing "
        "at least one primitive type must have a field called "
        "'active_index' as its first element of type 'int'." );
    static_assert(
        FBVariant::Traits::fields_number >= 2,
        "A Flatbuffers table representing a variant containing "
        "at least one primitive type must have at least two "
        "fields, one the 'active_index' and then one other." );
  }
  static_assert(
      mp::type_list_size_v<fb_type_list> ==
          std::variant_size_v<Variant>,
      "There is a mismatch between the number of fields in the "
      "variant and the flatbuffers table." );

  using res_t = std::pair<fb_type_list*, bool>;
  return res_t{ {}, has_active_index };
}

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
template<typename Hint,              //
         typename T,                 //
         std::enable_if_t<           //
             std::is_scalar_v<T> &&  //
                 !std::is_enum_v<T>, //
             int> = 0                //
         >
auto serialize( FBBuilder&, T const& o, serial::ADL ) {
  return ReturnValue{ o };
}

// For typed ints.
template<typename Hint,                             //
         typename T,                                //
         std::enable_if_t<                          //
             std::is_same_v<int, decltype( T::_ )>, //
             int> = 0                               //
         >
auto serialize( FBBuilder&, T const& o, serial::ADL ) {
  return ReturnValue{ o._ };
}

// For strings.
template<typename Hint>
auto serialize( FBBuilder& builder, std::string const& o,
                serial::ADL ) {
  // If the string is empty, do not serialize it, as that that it
  // is "not present" and will be deserialized into an empty
  // string without taking any space in the serialized data.
  if( o.empty() ) return ReturnValue{ FBOffset<Hint>{} };
  auto offset = builder.CreateString( o );
  return ReturnValue{ offset };
}

// For reflected enums.
template<typename Hint, //
         typename T,
         std::enable_if_t<      //
             std::is_enum_v<T>, //
             int> = 0           //
         >
auto serialize( FBBuilder&, T const& e, serial::ADL ) {
  // Both of these must be reflected enums.
  using fbs_enum_t = Hint;
  using src_enum_t = T;
  // FB enums have two additional members (MIN/MAX) but their in-
  // dexes are repeated from previous values.
  static_assert( enum_traits<src_enum_t>::count ==
                 static_cast<int>( fbs_enum_t::MAX ) + 1 );
  static_assert( static_cast<int>(
                     enum_traits<src_enum_t>::values[0] ) == 0 );
  static_assert( static_cast<int>( fbs_enum_t::MIN ) == 0 );
  auto fbs_enum =
      static_cast<fbs_enum_t>( static_cast<int>( e ) );
  // FIXME: improve below after FB enums get static reflection.
  // BASE_CHECK( maybe_fbs_enum.has_value(),
  //             "failed to serialize enum value {}", e );
  // DCHECK( enum_name( *maybe_fbs_enum ) == enum_name( e ),
  //         "{} != {}", enum_name( *maybe_fbs_enum ),
  //         enum_name( e ) );
  return ReturnValue{ fbs_enum };
}

// For C++ classes/structs that get serialized as FB structs.
template<typename Hint, //
         typename T, decltype( &T::serialize_struct )* = nullptr>
auto serialize( FBBuilder& builder, T const& o, serial::ADL ) {
  return ReturnAddress{ o.serialize_struct( builder ) };
}

// For C++ classes/structs that get serialized as FB tables.
template<typename Hint, //
         typename T, decltype( &T::serialize_table )* = nullptr>
auto serialize( FBBuilder& builder, T const& o, serial::ADL ) {
  return ReturnValue{ o.serialize_table( builder ) };
}

// For std::reference_wrapper.
template<typename Hint, typename T>
auto serialize( FBBuilder&                       builder,
                std::reference_wrapper<T> const& rr,
                serial::ADL ) {
  return serialize<Hint>( builder, rr.get(), serial::ADL{} );
}

// For maybe.
template<typename Hint, typename T>
auto serialize( FBBuilder& builder, maybe<T> const& o,
                serial::ADL ) {
  if( o.has_value() ) {
    using value_hint_t = fb_serialize_hint_t<
        decltype( std::declval<Hint>().value() )>;
    auto s_value =
        serialize<value_hint_t>( builder, *o, serial::ADL{} );
    return ReturnValue{ Hint::Traits::Create(
        builder, /*has_value=*/true, s_value.get() ) };
  } else {
    // The maybes are stored as tables, which can only be held by
    // other tables. Therefore, when they are nothing, we can
    // just return null for them so they are not serialized.
    return ReturnValue{ FBOffset<Hint>{} };
  }
}

// For pairs.
template<typename Hint, typename F, typename S>
auto serialize( FBBuilder& builder, std::pair<F, S> const& o,
                serial::ADL ) {
  using fst_hint_t = fb_serialize_hint_t<
      decltype( std::declval<Hint>().fst() )>;
  using snd_hint_t = fb_serialize_hint_t<
      decltype( std::declval<Hint>().snd() )>;
  auto const& s_fst =
      serialize<fst_hint_t>( builder, o.first, serial::ADL{} );
  auto const& s_snd =
      serialize<snd_hint_t>( builder, o.second, serial::ADL{} );
  if constexpr( detail::has_create_v<Hint> )
    return ReturnValue{ Hint::Traits::Create(
        builder, s_fst.get(), s_snd.get() ) };
  else
    return ReturnAddress{
        Hint( detail::ptr_to_ref( s_fst.get() ),
              detail::ptr_to_ref( s_snd.get() ) ) };
}

// For vectors.
template<typename Hint, typename T>
auto serialize( FBBuilder& builder, std::vector<T> const& o,
                serial::ADL ) {
  using namespace detail;
  using inner_hint_t =
      fb_serialize_hint_t<remove_fb_vector_t<Hint>>;
  using fb_wrapper_elem_t =
      std::decay_t<decltype( serialize<inner_hint_t>(
          builder, *o.begin(), serial::ADL{} ) )>;
  using fb_get_elem_t =
      decltype( std::declval<fb_wrapper_elem_t const&>().get() );
  // This vector must live until we create the final fb vector.
  std::vector<fb_wrapper_elem_t> wrappers;
  wrappers.reserve( o.size() );
  for( auto const& e : o )
    wrappers.emplace_back(
        serialize<inner_hint_t>( builder, e, serial::ADL{} ) );
  if constexpr( std::is_pointer_v<fb_get_elem_t> ) {
    // This is a struct.
    std::vector<inner_hint_t> values;
    values.reserve( o.size() );
    for( auto const& e : wrappers )
      values.emplace_back( *e.get() );
    return ReturnValue{
        builder.CreateVectorOfStructs( values ) };
  } else {
    // This is anything other than a struct, such as a table,
    // scalar, etc.
    std::vector<fb_get_elem_t> gotten;
    gotten.reserve( o.size() );
    for( auto const& e : wrappers ) gotten.push_back( e.get() );
    return ReturnValue{ builder.CreateVector( gotten ) };
  }
}

// For lists. Make a vector of references to the elements and
// just serialize that.
template<typename Hint, typename T>
auto serialize( FBBuilder& builder, std::list<T> const& l,
                serial::ADL ) {
  std::vector<std::reference_wrapper<T const>> v;
  v.reserve( l.size() );
  for( auto const& e : l ) v.emplace_back( e );
  return serialize<Hint>( builder, v, serial::ADL{} );
}

// For unordered_set. Make a vector of references to the elements
// and just serialize that.
template<typename Hint, typename T>
auto serialize( FBBuilder&                   builder,
                std::unordered_set<T> const& s, serial::ADL ) {
  std::vector<std::reference_wrapper<T const>> v;
  v.reserve( s.size() );
  for( auto const& e : s ) v.emplace_back( e );
  return serialize<Hint>( builder, v, serial::ADL{} );
}

// For std::set. Make a vector of references to the elements and
// just serialize that.
template<typename Hint, typename T>
auto serialize( FBBuilder& builder, std::set<T> const& s,
                serial::ADL ) {
  std::vector<std::reference_wrapper<T const>> v;
  v.reserve( s.size() );
  for( auto const& e : s ) v.emplace_back( e );
  return serialize<Hint>( builder, v, serial::ADL{} );
}

// For map-like things.
template<typename Hint,                        //
         typename T,                           //
         std::enable_if_t<                     //
             mp::is_map_like<T> &&             //
                 detail::is_fb_vector_v<Hint>, //
             int> = 0                          //
         >
auto serialize( FBBuilder& builder, T const& m, serial::ADL ) {
  std::vector<
      std::reference_wrapper<typename T::value_type const>>
      v;
  v.reserve( m.size() );
  for( auto const& p : m ) v.emplace_back( p );
  // This sorting adds overhead, but avoids non-determinism in-
  // troduced by unordered map key ordering.
  std::sort( v.begin(), v.end(),
             []( auto const& l, auto const& r ) {
               return l.get().first < r.get().first;
             } );
  return serialize<Hint>( builder, v, serial::ADL{} );
}

// For std::queue.
template<typename Hint, typename T>
auto serialize( FBBuilder& builder, std::queue<T> const& q,
                serial::ADL ) {
  std::vector<T> data;
  data.reserve( size_t( q.size() ) );
  auto m_copy = q;
  while( !m_copy.empty() ) {
    data.emplace_back( m_copy.front() );
    m_copy.pop();
  }
  return serialize<Hint>( builder, data, serial::ADL{} );
}

// For std::deque.
template<typename Hint, typename T>
auto serialize( FBBuilder& builder, std::deque<T> const& d,
                serial::ADL ) {
  std::vector<T> data;
  data.reserve( size_t( d.size() ) );
  auto m_copy = d;
  while( !m_copy.empty() ) {
    data.emplace_back( m_copy.front() );
    m_copy.pop_front();
  }
  return serialize<Hint>( builder, data, serial::ADL{} );
}

/****************************************************************
** Serialization of Variants
*****************************************************************/
namespace detail {

template<typename FBType, typename SrcT>
using serialize_return_container_t =
    decltype( serialize<fb_serialize_hint_t<FBType>>(
        std::declval<FBBuilder&>(), std::declval<SrcT const&>(),
        serial::ADL{} ) );

template<typename... FBTypes, typename... VarTypes>
auto variant_serialize_return_containers_tuple(
    mp::type_list<FBTypes...> const&,
    mp::type_list<VarTypes...> const& )
    -> mp::type_list<
        serialize_return_container_t<FBTypes, VarTypes>...>;

template<bool HasActiveIndex, typename Hint,
         typename... VarTypes>
auto variant_serialize_return_container_tuple() {
  if constexpr( HasActiveIndex ) {
    using ret_t =
        decltype( variant_serialize_return_containers_tuple(
            std::declval<
                mp::tail_t<fb_creation_tuple_t<Hint>> const&>(),
            std::declval<
                mp::type_list<VarTypes...> const&>() ) );
    return static_cast<ret_t*>( nullptr );
  } else {
    using ret_t =
        decltype( variant_serialize_return_containers_tuple(
            std::declval<fb_creation_tuple_t<Hint> const&>(),
            std::declval<
                mp::type_list<VarTypes...> const&>() ) );
    return static_cast<ret_t*>( nullptr );
  }
}

template<bool HasActiveIndex, typename Hint,
         typename... VarTypes>
using variant_serialize_return_container_tuple_t =
    std::remove_pointer_t<
        decltype( variant_serialize_return_container_tuple<
                  HasActiveIndex, Hint, VarTypes...>() )>;

} // namespace detail

// For base::variant.
template<typename Hint, typename... Ts>
auto serialize( FBBuilder&                  builder,
                base::variant<Ts...> const& o, serial::ADL ) {
  constexpr auto info = detail::validate_variant_active_index<
      Hint, base::variant<Ts...>>();
  using fb_type_list = std::remove_pointer_t<
      typename decltype( info )::first_type>;
  constexpr bool has_active_index = info.second;

  mp::to_tuple_t<fb_type_list> t;
  using return_containers_tuple_t = mp::to_tuple_t<
      detail::variant_serialize_return_container_tuple_t<
          has_active_index, Hint, Ts...>>;
  return_containers_tuple_t t_return;

  int count = 0;

  mp::for_index_seq<sizeof...( Ts )>(
      [&]<size_t Idx>( std::integral_constant<size_t, Idx> ) {
        auto* p = std::get_if<Idx>( &o );
        if( !p ) return;
        auto& tuple_elem = std::get<Idx>( t );
        auto& return_container_tuple_elem =
            std::get<Idx>( t_return );
        using elem_hint_t =
            fb_serialize_hint_t<decltype( tuple_elem )>;
        return_container_tuple_elem =
            serialize<elem_hint_t>( builder, *p, serial::ADL{} );
        count++;
      } );

  CHECK( count == 1,
         "Failed to find one active variant state to serialize. "
         " This could happen if the variant is in the "
         "valueless_by_exception state." );
  int32_t active_index       = static_cast<int32_t>( o.index() );
  auto    apply_with_builder = [&]( auto&&... ts ) {
    if constexpr( has_active_index ) {
      return Hint::Traits::Create( builder, active_index,
                                      ts.get()... );
    } else {
      (void)active_index;
      return Hint::Traits::Create( builder, ts.get()... );
    }
  };
  return ReturnValue{
      std::apply( apply_with_builder, t_return ) };
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
                 !std::is_enum_v<DstT> &&  //
                 std::is_same_v<std::decay_t<SrcT>,
                                std::decay_t<DstT>>, //
             int> = 0                                //
         >
valid_deserial_t deserialize( SrcT const* src, DstT* dst,
                              serial::ADL ) {
  DCHECK( src != nullptr,
          "`src` is nullptr when deserializing scalar." );
  *dst = *src;
  return valid;
}

// For reflected enums.
template<typename SrcT,            //
         typename DstT,            //
         std::enable_if_t<         //
             std::is_enum_v<DstT>, //
             int> = 0              //
         >
valid_deserial_t deserialize( SrcT const* src, DstT* dst,
                              serial::ADL ) {
  // Both of these must be reflected enums.
  using fbs_enum_t = SrcT;
  using dst_enum_t = DstT;
  // FB enums have two additional members (MIN/MAX) but their in-
  // dexes are repeated from previous values.
  static_assert( enum_traits<dst_enum_t>::count ==
                 static_cast<int>( fbs_enum_t::MAX ) + 1 );
  static_assert( static_cast<int>(
                     enum_traits<dst_enum_t>::values[0] ) == 0 );
  static_assert( static_cast<int>( fbs_enum_t::MIN ) == 0 );
  CHECK( src != nullptr,
         "`src` is nullptr when deserializing enum." );
  // FIXME: improve the below after FB enums get static
  // reflection.
  auto dst_enum =
      static_cast<dst_enum_t>( static_cast<int>( *src ) );
  // if( !maybe_dst_enum.has_value() )
  //   return invalid_deserial(
  //       fmt::format( "failed to deserialize enum value {}",
  //                    enum_name( *src ) ) );
  // DCHECK( enum_name( *maybe_dst_enum ) == enum_name( *src ),
  //         "{} != {}", enum_name( *maybe_dst_enum ),
  //         enum_name( *src ) );
  *dst = dst_enum;
  return valid;
}

// For typed ints.
template<typename SrcT,                                //
         typename DstT,                                //
         std::enable_if_t<                             //
             std::is_same_v<int, decltype( DstT::_ )>, //
             int> = 0                                  //
         >
valid_deserial_t deserialize( SrcT const* src, DstT* dst,
                              serial::ADL ) {
  DCHECK( src != nullptr,
          "`src` is nullptr when deserializing typed int." );
  *dst = DstT{ *src };
  return valid;
}

// For strings.
template<typename SrcT>
valid_deserial_t deserialize( SrcT const* src, std::string* dst,
                              serial::ADL ) {
  if( src == nullptr ) return valid;
  *dst = src->str();
  return valid;
}

// For C++ classes/structs that get serialized as FB structs.
template<typename SrcT, //
         typename DstT, //
         decltype( &DstT::deserialize_struct )* = nullptr>
valid_deserial_t deserialize( SrcT const* src, DstT* dst,
                              serial::ADL ) {
  if( src == nullptr ) {
    // An input value of nullptr for a struct means that the
    // struct is not present within its parent (which is probably
    // a table); "not present" means that it should assume its
    // default value. Since the `dst` pointer is already assumed
    // to point to a default-initialized value (by contract) we
    // can just return here.
    return valid;
  }
  if( auto xp = DstT::deserialize_struct( *src, dst ); !xp )
    return xp;
  return dst->check_invariants_safe();
}

// For C++ classes/structs that get serialized as FB tables.
template<typename SrcT, //
         typename DstT, //
         decltype( &DstT::deserialize_table )* = nullptr>
valid_deserial_t deserialize( SrcT const* src, DstT* dst,
                              serial::ADL ) {
  if( src == nullptr ) return valid;
  if( auto xp = DstT::deserialize_table( *src, dst ); !xp )
    return xp;
  return dst->check_invariants_safe();
}

// For maybe.
template<typename SrcT, typename T>
valid_deserial_t deserialize( SrcT const* src, maybe<T>* dst,
                              serial::ADL ) {
  if( src == nullptr || !src->has_value() ) {
    // `dst` should be in its default-constructed state, which is
    // nothing if it's a maybe.
    return valid;
  }
  if constexpr( std::is_pointer_v<decltype( src->value() )> ) {
    if( src->value() == nullptr )
      return invalid_deserial(
          "maybe has no `value` but has `has_value` == "
          "true." );
  }
  dst->emplace(); // default construct the value.
  return deserialize( detail::to_const_ptr( src->value() ),
                      std::addressof( **dst ), serial::ADL{} );
}

// For std::reference_wrapper.
template<typename SrcT, typename T>
valid_deserial_t deserialize( SrcT const*                src,
                              std::reference_wrapper<T>* dst,
                              serial::ADL ) = delete;

// For pairs.
template<typename SrcT, typename F, typename S>
valid_deserial_t deserialize( SrcT const*      src,
                              std::pair<F, S>* dst,
                              serial::ADL ) {
  if constexpr( std::is_pointer_v<decltype( src->fst() )> ) {
    if( src->fst() == nullptr ) {
      // Ok -- assumes default value.
      // return invalid_deserial( "pair has no `fst` value." );
    }
  }
  if constexpr( std::is_pointer_v<decltype( src->snd() )> ) {
    if( src->snd() == nullptr ) {
      // Ok -- assumes default value.
      // return invalid_deserial( "pair has no `snd` value." );
    }
  }
  HAS_VALUE_OR_RET(
      deserialize( detail::to_const_ptr( src->fst() ),
                   &dst->first, serial::ADL{} ) );
  HAS_VALUE_OR_RET(
      deserialize( detail::to_const_ptr( src->snd() ),
                   &dst->second, serial::ADL{} ) );
  return valid;
}

// For vectors.
template<typename SrcT, typename T>
valid_deserial_t deserialize( SrcT const*     src,
                              std::vector<T>* dst,
                              serial::ADL ) {
  // SrcT should be a flatbuffers::Vector.
  if( src == nullptr || src->size() == 0 ) {
    // `dst` should be in its default-constructed state, which is
    // an empty vector.
    return valid;
  }
  dst->resize( src->size() );
  using iter_t = decltype( src->size() );
  for( iter_t i = 0; i < src->size(); ++i ) {
    // This should be a value (scalar) or a pointer.
    auto elem = src->Get( i );
    HAS_VALUE_OR_RET(
        deserialize( detail::to_const_ptr( elem ),
                     std::addressof( dst->operator[]( i ) ),
                     serial::ADL{} ) );
  }
  return valid;
}

// For lists. Deserialize to vector first.
template<typename SrcT, typename T>
valid_deserial_t deserialize( SrcT const* src, std::list<T>* dst,
                              serial::ADL ) {
  // SrcT should be a flatbuffers::Vector.
  if( src == nullptr || src->size() == 0 ) {
    // `dst` should be in its default-constructed state, which is
    // an empty map.
    return valid;
  }
  std::vector<T> v;
  v.reserve( src->size() );
  HAS_VALUE_OR_RET( deserialize( src, &v, serial::ADL{} ) );
  // The list may have elements in it if it is the member of an
  // object that initializes it to always have at least one ele-
  // ment.
  dst->clear();
  for( auto& e : v ) dst->push_back( std::move( e ) );
  return valid;
}

// For unordered_set. Deserialize to vector first.
template<typename SrcT, typename T>
valid_deserial_t deserialize( SrcT const*            src,
                              std::unordered_set<T>* dst,
                              serial::ADL ) {
  // SrcT should be a flatbuffers::Vector.
  if( src == nullptr || src->size() == 0 ) {
    // `dst` should be in its default-constructed state, which is
    // an empty map.
    return valid;
  }
  std::vector<T> v;
  v.reserve( src->size() );
  HAS_VALUE_OR_RET( deserialize( src, &v, serial::ADL{} ) );
  dst->clear();
  for( auto& e : v ) dst->insert( std::move( e ) );
  return valid;
}

// For std::sets. Deserialize to vector first.
template<typename SrcT, typename T>
valid_deserial_t deserialize( SrcT const* src, std::set<T>* dst,
                              serial::ADL ) {
  // SrcT should be a flatbuffers::Vector.
  if( src == nullptr || src->size() == 0 ) {
    // `dst` should be in its default-constructed state, which is
    // an empty map.
    return valid;
  }
  std::vector<T> v;
  v.reserve( src->size() );
  HAS_VALUE_OR_RET( deserialize( src, &v, serial::ADL{} ) );
  dst->clear();
  for( auto& e : v ) dst->insert( std::move( e ) );
  return valid;
}

// For map-like things.
template<typename SrcT,                        //
         typename DstT,                        //
         std::enable_if_t<                     //
             mp::is_map_like<DstT> &&          //
                 detail::is_fb_vector_v<SrcT>, //
             int> = 0                          //
         >
valid_deserial_t deserialize( SrcT const* src, DstT* m,
                              serial::ADL ) {
  // SrcT should be a flatbuffers::Vector.
  if( src == nullptr || src->size() == 0 ) {
    // `dst` should be in its default-constructed state, which is
    // an empty map.
    return valid;
  }
  using iter_t = decltype( src->size() );
  using key_t  = typename DstT::key_type;
  if constexpr( mp::has_reserve_method<std::decay_t<DstT>> ) {
    m->reserve( src->size() );
  }
  for( iter_t i = 0; i < src->size(); ++i ) {
    auto* elem = src->Get( i );
    key_t key{};
    HAS_VALUE_OR_RET(
        deserialize( detail::to_const_ptr( elem->fst() ), &key,
                     serial::ADL{} ) );

    if constexpr( base::has_fmt<key_t> ) {
      if( m->find( key ) != m->end() )
        return invalid_deserial( fmt::format(
            "duplicate key ({}) found when deserializing map.",
            key ) );
    } else {
      if( m->find( key ) != m->end() )
        return invalid_deserial( fmt::format(
            "duplicate key (type {}) found when deserializing "
            "map.",
            ::base::demangled_typename<key_t>() ) );
    }

    HAS_VALUE_OR_RET(
        deserialize( detail::to_const_ptr( elem->snd() ),
                     std::addressof( m->operator[]( key ) ),
                     serial::ADL{} ) );
  }
  return valid;
}

// For std::queue.
template<typename SrcT, typename T>
valid_deserial_t deserialize( SrcT const* src, std::queue<T>* q,
                              serial::ADL ) {
  if( src == nullptr ) {
    // `dst` should be in its default-constructed state, which is
    // an empty queue.
    return valid;
  }
  std::vector<T> data;
  HAS_VALUE_OR_RET( deserialize( src, &data, serial::ADL{} ) );
  DCHECK( q->empty() );
  for( auto& e : data ) q->emplace( std::move( e ) );
  return valid;
}

// For std::deque.
template<typename SrcT, typename T>
valid_deserial_t deserialize( SrcT const* src, std::deque<T>* d,
                              serial::ADL ) {
  if( src == nullptr ) {
    // `dst` should be in its default-constructed state, which is
    // an empty queue.
    return valid;
  }
  std::vector<T> data;
  HAS_VALUE_OR_RET( deserialize( src, &data, serial::ADL{} ) );
  for( auto& e : data ) d->push_back( std::move( e ) );
  return valid;
}

/****************************************************************
** Deserialization of Variants
*****************************************************************/
namespace detail {

// Extract the `active_index` field from the fields list and put
// it into the corresponding alternative in the variant. This
// function takes an `active_index`, but it works and is used
// also for tables that don't have an `active_index` field. Ei-
// ther way, the index of the active member is computed and given
// to this function, so it doesn't care how it is computed. The
// `Offset` is 1 for types have have an active_index field and is
// used to skip over it.
template<size_t Offset, typename SrcT, typename Variant>
valid_deserial_t visit_tuple_variant_deserialize(
    SrcT const* src, Variant& dst, int active_index ) {
  valid_deserial_t err   = valid;
  bool             found = false;
  mp::for_index_seq<std::variant_size_v<Variant>>(
      [&]<size_t Idx>( std::integral_constant<size_t, Idx> ) {
        // Return true means we will stop iterating.
        if( active_index != Idx ) return false;
        err = deserialize(
            detail::to_const_ptr(
                src->template get_field<Idx + Offset>() ),
            &dst.template emplace<Idx>(), ADL{} );
        if( !err ) return true;
        found = true;
        return true; // stop iterating early.
      } );
  if( !err ) return err;
  if( !found )
    return invalid_deserial(
        "failed to deserialize precisely one active alternative "
        "in variant." );
  return valid;
}

// Given a tuple representing the field types of a Flatbuffer
// table that is supposed to hold only pointer members (and thus
// does not need/have an active_index member) this will iterate
// through the fields to find the (hopefully only) one that is
// non-null, meaning active.
template<typename SrcT>
expect<int, DeserialError> find_active_index_in_fb(
    SrcT const* src ) {
  int              count = 0;
  maybe<int>       active_index;
  constexpr size_t field_count = SrcT::Traits::fields_number;
  mp::for_index_seq<field_count>(
      [&]<size_t Idx>( std::integral_constant<size_t, Idx> ) {
        static_assert(
            std::is_pointer_v<std::remove_reference_t<
                decltype( src->template get_field<Idx>() )>>,
            "expected get_field to have all pointer types since "
            "this is supposed to be an FB variant with no "
            "active_index." );
        auto* p = src->template get_field<Idx>();
        if( p != nullptr ) {
          ++count;
          active_index = Idx;
        }
      } );
  if( count != 1 )
    return invalid_deserial(
               fmt::format( "failed to find precisely one "
                            "active alternative in FB table "
                            "representing variant with no "
                            "active_index; instead found {}.",
                            count ) )
        .error();
  DCHECK( *active_index < int( field_count ) );
  return *active_index;
}

} // namespace detail

// For base::variant.
template<typename SrcT, typename... Vs>
valid_deserial_t deserialize( SrcT const*           src,
                              base::variant<Vs...>* dst,
                              serial::ADL ) {
  constexpr auto info = detail::validate_variant_active_index<
      SrcT, base::variant<Vs...>>();
  constexpr bool has_active_index = info.second;
  if( src == nullptr ) {
    // `dst` should be in its default-constructed state, which
    // would be the first alternative in a default-constructed
    // state. If there is an active_index then it will be zero
    // which is consistent with this.
    return valid;
  }
  if constexpr( has_active_index ) {
    int32_t active_index = src->template get_field<0>();
    HAS_VALUE_OR_RET( detail::visit_tuple_variant_deserialize<1>(
        src, *dst, active_index ) );
  } else {
    UNWRAP_RETURN( active_index,
                   detail::find_active_index_in_fb( src ) );
    HAS_VALUE_OR_RET( detail::visit_tuple_variant_deserialize<0>(
        src, *dst, active_index ) );
  }
  return valid;
}

} // namespace rn::serial

/****************************************************************
** Table Macros
*****************************************************************/
#define SERIAL_CALL_SERIALIZE_TABLE_IMPL( type, var )      \
  auto PP_JOIN( s_, var ) =                                \
      serialize<::rn::serial::fb_serialize_hint_t<         \
          decltype( std::declval<fb_target_t>().var() )>>( \
          builder, var, serial::ADL{} )

#define SERIAL_CALL_SERIALIZE_TABLE( p ) \
  SERIAL_CALL_SERIALIZE_TABLE_IMPL p

#define SERIAL_GET_SERIALIZED_IMPL( type, var ) \
  PP_JOIN( s_, var ).get()
#define SERIAL_GET_SERIALIZED( p ) SERIAL_GET_SERIALIZED_IMPL p

#define SERIAL_DECLARE_VAR_TABLE( type, var ) type var{};

#define SERIAL_DESERIALIZE_VAR_TABLE_IMPL( type, var )        \
  HAS_VALUE_OR_RET(                                           \
      deserialize( serial::detail::to_const_ptr( src.var() ), \
                   &dst->var, serial::ADL{} ) );

#define SERIAL_DESERIALIZE_VAR_TABLE( p ) \
  SERIAL_DESERIALIZE_VAR_TABLE_IMPL p

#define SERIALIZABLE_TABLE_MEMBERS_NO_EVAL( ns, name, ... )  \
  PP_MAP_TUPLE( SERIAL_DECLARE_VAR_TABLE, __VA_ARGS__ )      \
public:                                                      \
  using fb_target_t = ns::name;                              \
  serial::FBOffset<ns::name> serialize_table(                \
      serial::FBBuilder& builder ) const {                   \
    static const name def_val{};                             \
    /* If the table has its fully default value then */      \
    /* do not serialize it, return null offset. */           \
    if( *this == def_val ) return {};                        \
    using ::rn::serial::serialize;                           \
    PP_MAP_SEMI( SERIAL_CALL_SERIALIZE_TABLE, __VA_ARGS__ )  \
    return ns::Create##name(                                 \
        builder __VA_OPT__(, ) PP_MAP_COMMAS(                \
            SERIAL_GET_SERIALIZED, __VA_ARGS__ ) );          \
  }                                                          \
  static valid_deserial_t deserialize_table(                 \
      ns::name const& src, name* dst ) {                     \
    DCHECK( dst );                                           \
    (void)src;                                               \
    (void)dst;                                               \
    using ::rn::serial::deserialize;                         \
    PP_MAP_SEMI( SERIAL_DESERIALIZE_VAR_TABLE, __VA_ARGS__ ) \
    return valid;                                            \
  }                                                          \
                                                             \
private:

#define SERIALIZABLE_TABLE_MEMBERS( ... ) \
  EVAL( SERIALIZABLE_TABLE_MEMBERS_NO_EVAL( __VA_ARGS__ ) )

/****************************************************************
** Struct Macros
*****************************************************************/
#define SERIAL_CALL_SERIALIZE_STRUCT_NO_EVAL( type, var )  \
  auto PP_JOIN( s_, var ) =                                \
      serialize<::rn::serial::fb_serialize_hint_t<         \
          decltype( std::declval<fb_target_t>().var() )>>( \
          builder, var, serial::ADL{} )

#define SERIAL_CALL_SERIALIZE_STRUCT( p ) \
  SERIAL_CALL_SERIALIZE_STRUCT_NO_EVAL p

#define SERIAL_DECLARE_VAR_STRUCT( type, var ) type var{};

#define SERIAL_DESERIALIZE_VAR_STRUCT_IMPL( type, var )       \
  HAS_VALUE_OR_RET(                                           \
      deserialize( serial::detail::to_const_ptr( src.var() ), \
                   &dst->var, serial::ADL{} ) );

#define SERIAL_DESERIALIZE_VAR_STRUCT( p ) \
  SERIAL_DESERIALIZE_VAR_STRUCT_IMPL p

#define SERIALIZABLE_STRUCT_MEMBERS_IMPL( name, ... )          \
  PP_MAP_TUPLE( SERIAL_DECLARE_VAR_STRUCT, __VA_ARGS__ )       \
public:                                                        \
  using fb_target_t = fb::name;                                \
  fb::name serialize_struct( serial::FBBuilder& builder )      \
      const {                                                  \
    using ::rn::serial::serialize;                             \
    PP_MAP_SEMI( SERIAL_CALL_SERIALIZE_STRUCT, __VA_ARGS__ )   \
    return fb::name(                                           \
        PP_MAP_COMMAS( SERIAL_GET_SERIALIZED, __VA_ARGS__ ) ); \
  }                                                            \
  static valid_deserial_t deserialize_struct(                  \
      fb::name const& src, name* dst ) {                       \
    DCHECK( dst );                                             \
    using ::rn::serial::deserialize;                           \
    PP_MAP_SEMI( SERIAL_DESERIALIZE_VAR_STRUCT, __VA_ARGS__ )  \
    return valid;                                              \
  }

#define SERIALIZABLE_STRUCT_MEMBERS( ... ) \
  EVAL( SERIALIZABLE_STRUCT_MEMBERS_IMPL( __VA_ARGS__ ) )
