/****************************************************************
**fb.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-19.
*
* Description: Stuff needed by headers with serializable types.
*
*****************************************************************/
#include "fb.hpp"

// Revolution Now
#include "logging.hpp"
#include "serial.hpp"

// base
#include "base/variant.hpp"

// Flatbuffers
#include "fb/testing_generated.h"

// Abseil
#include "absl/strings/str_replace.h"

using namespace std;

namespace rn::serial {

/****************************************************************
** Public API
*****************************************************************/
string ns_to_dots( string_view sv ) {
  return absl::StrReplaceAll( sv, { { "::", "." } } );
}

struct Vec2 {
  bool     operator==( Vec2 const& ) const = default;
  expect<> check_invariants_safe() const {
    return xp_success_t{};
  }
  SERIALIZABLE_STRUCT_MEMBERS( Vec2,         //
                               ( float, x ), //
                               ( float, y )  //
  );
};
struct Weapon {
  bool     operator==( Weapon const& ) const = default;
  expect<> check_invariants_safe() const {
    if( name.empty() )
      return UNEXPECTED( "Weapon name cannot be empty." );
    return xp_success_t{};
  }
  SERIALIZABLE_TABLE_MEMBERS( fb, Weapon,       //
                              ( string, name ), //
                              ( short, damage ) //
  );
};

} // namespace rn::serial

DEFINE_FORMAT( ::rn::serial::Vec2, "Vec2{{x={},y={}}}", o.x,
               o.y );
DEFINE_FORMAT( ::rn::serial::Weapon,
               "Weapon{{name={},damage={}}}", o.name, o.damage );

namespace rn::serial {

enum class e_color {
  Red,   //
  Green, //
  Blue   //
};

using MyVariant = base::variant< //
    int,                         //
    Vec2,                        //
    Weapon,                      //
    e_color                      //
    >;

using MyVariantNoIndex = base::variant< //
    Vec2,                               //
    Weapon,                             //
    Weapon                              //
    >;

using MyFloatVariant = base::variant< //
    float,                            //
    float,                            //
    float,                            //
    float                             //
    >;

template<typename FBTable>
struct fb_variant_table_needs_alternative_index;

template<typename... Ts>
struct fb_variant_table_needs_alternative_index<
    std::tuple<Ts...>> {
  static constexpr bool value =
      !mp::and_v<std::is_pointer_v<Ts>...>;
};

template<typename Tuple>
inline constexpr bool
    fb_variant_table_needs_alternative_index_v =
        fb_variant_table_needs_alternative_index<Tuple>::value;

/****************************************************************
** Serialization of Variants
*****************************************************************/
template<size_t Idx, typename FbTypesTuple,
         typename ReturnContainersTuple, typename Variant,
         typename Func>
void visit_tuple_variant_elem( FbTypesTuple const&    t,
                               ReturnContainersTuple& t_return,
                               Variant const&         v,
                               Func const&            func ) {
  auto* p = std::get_if<Idx>( &v );
  if( !p ) return;
  func( *p, std::get<Idx>( t ), std::get<Idx>( t_return ) );
}

template<typename FbTypesTuple, typename ReturnContainersTuple,
         typename Variant, typename Func, size_t... Indexes>
void visit_tuple_variant_impl(
    FbTypesTuple const& t, ReturnContainersTuple& t_return,
    Variant const& v, Func const& func,
    std::index_sequence<Indexes...> ) {
  ( visit_tuple_variant_elem<Indexes>( t, t_return, v, func ),
    ... );
}

template<typename FbTypesTuple, typename ReturnContainersTuple,
         typename Variant, typename Func>
void visit_tuple_variant_serialize(
    FbTypesTuple const& t, ReturnContainersTuple& t_return,
    Variant const& v, Func const& f ) {
  static_assert( std::tuple_size<FbTypesTuple>::value ==
                 std::variant_size_v<Variant> );
  static_assert( std::tuple_size<ReturnContainersTuple>::value ==
                 std::variant_size_v<Variant> );
  visit_tuple_variant_impl(
      t, t_return, v, f,
      std::make_index_sequence<
          std::tuple_size<FbTypesTuple>::value>() );
}

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

// For base::variant.
template<typename Hint, typename... Ts>
auto serialize( FBBuilder&                  builder,
                base::variant<Ts...> const& o, serial::ADL ) {
  using FBVariant  = Hint;
  using FieldTypes = typename FBVariant::FieldTypes;
  constexpr bool has_active_index =
      fb_variant_table_needs_alternative_index_v<FieldTypes>;
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
      mp::type_list_size_v<fb_type_list> == sizeof...( Ts ),
      "There is a mismatch between the number of fields in the "
      "variant and the flatbuffers table." );
  mp::to_tuple_t<fb_type_list> t;
  using return_containers_tuple_t =
      mp::to_tuple_t<std::remove_pointer_t<
          decltype( variant_serialize_return_container_tuple<
                    has_active_index, Hint, Ts...>() )>>;
  return_containers_tuple_t t_return;

  int count = 0;
  // Set the relevant tuple field.
  visit_tuple_variant_serialize(
      t, t_return, o,
      [&]( auto const& variant_elem, auto const& tuple_elem,
           auto& return_container_tuple_elem ) {
        using elem_hint_t =
            fb_serialize_hint_t<decltype( tuple_elem )>;
        return_container_tuple_elem = serialize<elem_hint_t>(
            builder, variant_elem, serial::ADL{} );
        count++;
      } );
  DCHECK( count == 1 );
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
template<size_t Idx, typename Variant, typename T>
bool try_deserialize_alternative( expect<>& err,
                                  int active_index, Variant& dst,
                                  T&& field_value ) {
  if( !err )
    // There's already been an error.
    return false;
  if( active_index != Idx )
    // This alternative is not active.
    return false;
  auto& alternative = dst.template emplace<Idx>();
  err = deserialize( detail::to_const_ptr( field_value ),
                     &alternative, ADL{} );
  return true;
}

template<bool HasActiveIndex, typename Tuple, typename Variant,
         size_t... Indexes>
expect<> visit_tuple_variant_deserialize(
    int active_index, Tuple const& fields_pack, Variant& dst,
    std::index_sequence<Indexes...> ) {
  constexpr bool skip_active_index = HasActiveIndex ? 1 : 0;

  int      count = 0;
  expect<> err   = xp_success_t{};
  ( ( count +=
      try_deserialize_alternative<Indexes>(
          // +1 to skip active_index field.
          err, active_index, dst,
          std::get<Indexes + skip_active_index>( fields_pack ) )
          ? 1
          : 0 ),
    ... );
  if( !err ) return err;
  if( count != 1 )
    return UNEXPECTED(
        "failed to deserialize precisely one active alternative "
        "in variant; instead deserialized {}.",
        count );
  return xp_success_t{};
}

template<size_t Index, typename Tuple>
void check_tuple_field_active( Tuple const& tp, int& count,
                               maybe<int>& active_index ) {
  static_assert(
      std::is_pointer_v<std::remove_reference_t<
          decltype( std::get<Index>( tp ) )>>,
      "expected fields_pack tuple to have all pointer types "
      "since this is supposed to be an FB variant with no "
      "active_index." );
  auto* p = std::get<Index>( tp );
  if( p != nullptr ) {
    ++count;
    active_index = Index;
  }
}

template<typename Tuple, size_t... Indexes>
expect<int> find_active_index_in_tuple(
    Tuple const& tp, std::index_sequence<Indexes...> ) {
  int        count = 0;
  maybe<int> active_index;
  ( check_tuple_field_active<Indexes>( tp, count, active_index ),
    ... );
  if( count != 1 )
    return UNEXPECTED(
        "failed to find precisely one active alternative in FB "
        "table representing variant with no active_index; "
        "instead found {}.",
        count );
  if( !active_index )
    return UNEXPECTED(
        "invalid result obtained when deterining active_index "
        "of FB variant table with no active_index as a first "
        "member; obtained nothing.",
        active_index );
  if( *active_index > int( sizeof...( Indexes ) ) )
    return UNEXPECTED(
        "invalid result obtained when deterining active_index "
        "of FB variant table with no active_index as a first "
        "member; obtained {}.",
        *active_index );
  return *active_index;
}

template<typename SrcT, typename... Vs>
expect<> deserialize( SrcT const* src, base::variant<Vs...>* dst,
                      serial::ADL ) {
  using FBVariant  = SrcT;
  using FieldTypes = typename FBVariant::FieldTypes;
  constexpr bool needs_index =
      fb_variant_table_needs_alternative_index_v<FieldTypes>;
  using fb_type_list = std::conditional_t<
      needs_index, mp::tail_t<fb_creation_tuple_t<FBVariant>>,
      fb_creation_tuple_t<FBVariant>>;
  // =========
  if constexpr( needs_index ) {
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
      mp::type_list_size_v<fb_type_list> == sizeof...( Vs ),
      "There is a mismatch between the number of fields in the "
      "variant and the flatbuffers table." );
  // Rename this for readability now that we've checked it.
  constexpr bool has_active_index = needs_index;
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
    XP_OR_RETURN_(
        visit_tuple_variant_deserialize<has_active_index>(
            active_index, fields_pack, *dst,
            std::make_index_sequence<sizeof...( Vs )>() ) );
  } else {
    expect<int> active_index = find_active_index_in_tuple(
        fields_pack, std::make_index_sequence<
                         std::tuple_size_v<FieldTypes>>() );
    if( !active_index )
      return propagate_unexpected( active_index );
    XP_OR_RETURN_(
        visit_tuple_variant_deserialize<has_active_index>(
            *active_index, fields_pack, *dst,
            std::make_index_sequence<sizeof...( Vs )>() ) );
  }
  return xp_success_t{};
}

template<typename fb_table_t, typename Variant>
void test_serialize_variant( FBBuilder& fbb, Variant const& v ) {
  fbb.Finish(
      serialize<fb_table_t>( fbb, v, serial::ADL{} ).get() );
}

template<typename fb_table_t, typename Variant>
void test_round_trip( Variant const& v ) {
  print_bar( '=', fmt::format( "[ v = {} ]", v ) );
  FBBuilder fbb;
  // =======
  test_serialize_variant<fb_table_t>( fbb, v );
  auto blob = BinaryBlob::from_builder( std::move( fbb ) );
  auto json =
      blob.template to_json<fb_table_t>( /*quotes=*/false );
  lg.info( "json:\n{}", json );
  // =======
  auto    fb_var = blob.root<fb_table_t>();
  Variant new_v;
  CHECK_XP( deserialize( fb_var, &new_v, ADL{} ) );
  CHECK( new_v.index() == v.index(), "{} != {}", new_v.index(),
         v.index() );
  CHECK( new_v == v, "{} != {}", new_v, v );
}

void test_fb() {
  {
    using fb_table_t = ::fb::MyVariant;

    MyVariant v;

    v = 5;
    test_round_trip<fb_table_t>( v );

    v = Vec2{ 4.4, 6.6 };
    test_round_trip<fb_table_t>( v );

    v = Weapon{ "hello", 3 };
    test_round_trip<fb_table_t>( v );

    v.emplace<e_color>( e_color::Green );
    test_round_trip<fb_table_t>( v );
  }
  {
    using fb_table_t = ::fb::MyVariantNoIndex;

    MyVariantNoIndex v;

    v = Vec2{ 4.4, 6.6 };
    test_round_trip<fb_table_t>( v );

    v.emplace<1>() = Weapon{ "hello", 3 };
    test_round_trip<fb_table_t>( v );

    v.emplace<2>() = Weapon{ "world", 4 };
    test_round_trip<fb_table_t>( v );
  }
  {
    using fb_table_t = ::fb::MyFloatVariant;

    MyFloatVariant v;

    v.emplace<0>() = 5.5f;
    test_round_trip<fb_table_t>( v );

    v.emplace<1>() = 0.0f;
    test_round_trip<fb_table_t>( v );

    v.emplace<2>() = 6.6f;
    test_round_trip<fb_table_t>( v );

    v.emplace<3>() = 7.7f;
    test_round_trip<fb_table_t>( v );
  }
}

} // namespace rn::serial
