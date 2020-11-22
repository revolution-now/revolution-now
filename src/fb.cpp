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
#include "enum.hpp"
#include "logging.hpp"
#include "serial.hpp"

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

/****************************************************************
** Testing
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
void visit_tuple_variant( FbTypesTuple const&    t,
                          ReturnContainersTuple& t_return,
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

struct Vec2 {
  SERIALIZABLE_STRUCT_MEMBERS( Vec2,         //
                               ( float, x ), //
                               ( float, y )  //
  );
};
struct Weapon {
  SERIALIZABLE_TABLE_MEMBERS( fb, Weapon,       //
                              ( string, name ), //
                              ( short, damage ) //
  );
};

enum class e_( color, //
               Red,   //
               Green, //
               Blue   //
);
SERIALIZABLE_BETTER_ENUM( e_color );

using MyVariant = std::variant< //
    int,                        //
    Vec2,                       //
    Weapon,                     //
    e_color                     //
    >;

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

template<typename Hint, typename... VarTypes>
using variant_serialize_return_container_tuple_t =
    decltype( variant_serialize_return_containers_tuple(
        std::declval<fb_creation_tuple_t<Hint> const&>(),
        std::declval<mp::type_list<VarTypes...> const&>() ) );

// For std::variant.
template<typename Hint, typename... Ts>
auto serialize( FBBuilder& builder, std::variant<Ts...> const& o,
                serial::ADL ) {
  using fb_type_list = fb_creation_tuple_t<Hint>;
  static_assert(
      mp::type_list_size_v<fb_type_list> == sizeof...( Ts ),
      "There is a mismatch between the number of fields in the "
      "variant and the flatbuffers table." );
  mp::to_tuple_t<fb_type_list> t;
  using return_containers_tuple_t = mp::to_tuple_t<
      variant_serialize_return_container_tuple_t<Hint, Ts...>>;
  return_containers_tuple_t t_return;

  int count = 0;
  // Set the relevant tuple field.
  visit_tuple_variant(
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
  auto apply_with_builder = [&]( auto&&... ts ) {
    return Hint::Traits::Create( builder, ts.get()... );
  };
  return ReturnValue{
      std::apply( apply_with_builder, t_return ) };
}

template<typename fb_table_t, typename Variant>
void test_serialize_variant( Variant const& v ) {
  FBBuilder fbb;
  auto      res = serialize<fb_table_t>( fbb, v, serial::ADL{} );
  fbb.Finish( res.get() );
  auto blob = BinaryBlob::from_builder( std::move( fbb ) );
  auto json =
      blob.template to_json<fb_table_t>( /*quotes=*/false );
  lg.info( "json:\n{}", json );
}

void test_fb() {
  using fb_table_t = ::fb::MyVariant;

  MyVariant v;

  v = 5;
  test_serialize_variant<fb_table_t>( v );

  v = Vec2{ 4.4, 6.6 };
  test_serialize_variant<fb_table_t>( v );

  v = Weapon{ "hello", 3 };
  test_serialize_variant<fb_table_t>( v );

  v.emplace<e_color>( e_color::Green );
  test_serialize_variant<fb_table_t>( v );
}

} // namespace rn::serial
