/****************************************************************
**fb-variant.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-13.
*
* Description: Helpers for generically serializing variants.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "fb.hpp"

// base
#include "base/meta.hpp"
#include "base/variant.hpp"

namespace rn::serial {

/****************************************************************
** Perform validation related to the active_index.
*****************************************************************/
namespace detail {

template<typename FBTable>
struct fb_variant_table_needs_alternative_index;

template<typename... Ts>
struct fb_variant_table_needs_alternative_index<
    std::tuple<Ts...>> {
  static constexpr bool value =
      !mp::and_v<std::is_pointer_v<Ts>...>;
};

// Determine if FB table needs active_index field.
template<typename Tuple>
inline constexpr bool
    fb_variant_table_needs_alternative_index_v =
        fb_variant_table_needs_alternative_index<Tuple>::value;

template<typename FBVariant, typename Variant>
constexpr auto validate_variant_active_index() {
  using FieldTypes = typename FBVariant::FieldTypes;
  constexpr bool has_active_index =
      detail::fb_variant_table_needs_alternative_index_v<
          FieldTypes>;
  using fb_type_list = std::conditional_t<
      has_active_index,
      mp::tail_t<fb_creation_tuple_t<FBVariant>>,
      fb_creation_tuple_t<FBVariant>>;
  if constexpr( has_active_index ) {
    using first_field_t = std::tuple_element_t<0, FieldTypes>;
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
        std::tuple_size_v<FieldTypes> >= 2,
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
** Deserialization of Variants
*****************************************************************/
namespace detail {

// Extract the `active_index` field from the fields pack and put
// it into the corresponding alternative in the variant. This
// function takes an `active_index`, but it works and is used
// also for tables that don't have an `active_index` field. Ei-
// ther way, the index of the active member is computed and given
// to this function, so it doesn't care how it is computed.
template<typename Tuple, typename Variant>
expect<> visit_tuple_variant_deserialize(
    Tuple const& fields_pack, Variant& dst, int active_index ) {
  expect<> err   = xp_success_t{};
  bool     found = false;
  mp::for_index_seq<std::variant_size_v<Variant>>(
      [&]<size_t Idx>( std::integral_constant<size_t, Idx> ) {
        // Return true means we will stop iterating.
        if( active_index != Idx ) return false;
        err = deserialize(
            detail::to_const_ptr( std::get<Idx>( fields_pack ) ),
            &dst.template emplace<Idx>(), ADL{} );
        if( !err ) return true;
        found = true;
        return true; // stop iterating early.
      } );
  if( !err ) return err;
  if( !found )
    return UNEXPECTED(
        "failed to deserialize precisely one active alternative "
        "in variant." );
  return xp_success_t{};
}

// Given a tuple representing the field types of a Flatbuffer
// table that is supposed to hold only pointer members (and thus
// does not need/have an active_index member) this will iterate
// through the fields to find the (hopefully only) one that is
// non-null, meaning active.
template<typename Tuple>
expect<int> find_active_index_in_tuple( Tuple const& tp ) {
  int        count = 0;
  maybe<int> active_index;
  mp::for_index_seq<std::tuple_size_v<
      Tuple>>( [&]<size_t Idx>(
                   std::integral_constant<size_t, Idx> ) {
    static_assert(
        std::is_pointer_v<std::remove_reference_t<
            decltype( std::get<Idx>( tp ) )>>,
        "expected fields_pack tuple to have all pointer types "
        "since this is supposed to be an FB variant with no "
        "active_index." );
    auto* p = std::get<Idx>( tp );
    if( p != nullptr ) {
      ++count;
      active_index = Idx;
    }
  } );
  if( count != 1 )
    return UNEXPECTED(
        "failed to find precisely one active alternative in FB "
        "table representing variant with no active_index; "
        "instead found {}.",
        count );
  DCHECK( *active_index < int( std::tuple_size_v<Tuple> ) );
  return *active_index;
}

} // namespace detail

// For base::variant.
template<typename SrcT, typename... Vs>
expect<> deserialize( SrcT const* src, base::variant<Vs...>* dst,
                      serial::ADL ) {
  constexpr auto info = detail::validate_variant_active_index<
      SrcT, base::variant<Vs...>>();
  constexpr bool has_active_index = info.second;
  if( src == nullptr ) {
    // `dst` should be in its default-constructed state, which
    // would be the first alternative in a default-constructed
    // state. If there is an active_index then it will be zero
    // which is consistent with this.
    return xp_success_t{};
  }
  auto fields_pack = src->fields_pack();
  if constexpr( has_active_index ) {
    int32_t active_index = std::get<0>( fields_pack );
    XP_OR_RETURN_( detail::visit_tuple_variant_deserialize(
        mp::tuple_tail( fields_pack ), *dst, active_index ) );
  } else {
    XP_OR_RETURN(
        active_index,
        detail::find_active_index_in_tuple( fields_pack ) );
    XP_OR_RETURN_( detail::visit_tuple_variant_deserialize(
        fields_pack, *dst, active_index ) );
  }
  return xp_success_t{};
}

} // namespace rn::serial
