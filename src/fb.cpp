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
template<size_t Idx, typename Tuple, typename Variant,
         typename Func>
void visit_tuple_variant_elem( Tuple& t, Variant const& v,
                               Func const& func ) {
  auto* p = std::get_if<Idx>( &v );
  if( !p ) return;
  func( *p, std::get<Idx>( t ) );
}

template<typename Tuple, typename Variant, typename Func,
         size_t... Indexes>
void visit_tuple_variant_impl(
    Tuple& t, Variant const& v, Func const& func,
    std::index_sequence<Indexes...> ) {
  ( visit_tuple_variant_elem<Indexes>( t, v, func ), ... );
}

template<typename Tuple, typename Variant, typename Func>
void visit_tuple_variant( Tuple& t, Variant const& v,
                          Func const& f ) {
  static_assert( std::tuple_size<Tuple>::value ==
                 std::variant_size_v<Variant> );
  visit_tuple_variant_impl(
      t, v, f,
      std::make_index_sequence<
          std::tuple_size<Tuple>::value>() );
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
SERIALIZABLE_ENUM( e_color );

using MyVariant = std::variant< //
    int,                        //
    Vec2,                       //
    Weapon,                     //
    e_color                     //
    >;

// For std::variant.
template<typename Hint, typename... Ts>
auto serialize( FBBuilder& builder, std::variant<Ts...> const& o,
                serial::ADL ) {
  using tuple_t = fb_creation_tuple_t<Hint>;
  tuple_t t;
  int     count = 0;
  // Set the relevant tuple field.
  visit_tuple_variant(
      t, o, [&]( auto const& variant_elem, auto& tuple_elem ) {
        using elem_hint_t =
            fb_serialize_hint_t<decltype( tuple_elem )>;
        auto res = serialize<elem_hint_t>(
            builder, variant_elem, serial::ADL{} );
        // FIXME: does not work for structs. For those, we need
        // also to have a tuple of Return*{} structs to hold the
        // struct results.
        tuple_elem = res.get();
        count++;
      } );
  DCHECK( count == 1 );
  auto apply_with_builder = [&]( auto... ts ) {
    return Hint::Create( builder, ts... );
  };
  return ReturnValue{ std::apply( apply_with_builder, t ) };
}

template<typename fb_table_t, typename Variant>
void test_serialize_variant( Variant const& v ) {
  FBBuilder fbb;
  auto      res = serialize<fb_table_t>( fbb, v,
                                    serial::ADL{} );
  fbb.Finish( res.get() );
  auto blob = BinaryBlob::from_builder( std::move( fbb ) );
  auto json = blob.template to_json<fb_table_t>();
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

  v = e_color::Red;
  test_serialize_variant<fb_table_t>( v );
}

} // namespace rn::serial
