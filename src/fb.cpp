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

// Flatbuffers
#include "fb/testing_generated.h"

// Abseil
#include "absl/strings/str_replace.h"

// C++ standard library
#include <tuple>

using namespace std;

namespace rn::serial {

/****************************************************************
** Public API
*****************************************************************/
string ns_to_dots( string_view sv ) {
  return absl::StrReplaceAll( sv, {{"::", "."}} );
}

/****************************************************************
** Testing
*****************************************************************/
template<size_t Idx, typename T>
struct enumerated_pair;

template<typename...>
struct enumerate_tuple;

template<size_t... Indexes, typename... Ts>
struct enumerate_tuple<std::index_sequence<Indexes...>,
                       std::tuple<Ts...>> {
  using type = std::tuple<enumerated_pair<Indexes, Ts>...>;
};

template<typename Tuple>
using enumerate_tuple_t = typename enumerate_tuple<
    decltype( std::make_index_sequence<
              std::tuple_size<Tuple>::value>() ),
    Tuple>::type;

template<typename...>
struct fb_creation_tuple;

template<typename Ret, typename... Args>
struct fb_creation_tuple<Ret( FBBuilder&, Args... )> {
  using tuple            = std::tuple<Args...>;
  using enumerated_tuple = enumerate_tuple_t<tuple>;
};

template<typename FB>
using fb_creation_tuple_t =
    typename fb_creation_tuple<decltype( FB::Create )>::tuple;

// template<typename Tuple, size_t Idx,
template<typename...>
struct tuple_set_from_variant;

template<typename... TupleElems, typename... VariantElems>
struct tuple_set_from_variant<std::tuple<TupleElems...>,
                              std::variant<VariantElems...>> {
  using tuple_t   = std::tuple<TupleElems...>;
  using variant_t = std::variant<TupleElems...>;

  // template<size_t Idx, typename Func>
  // auto apply( Func&& f,
};

struct Point {
  int x, y;
};
struct Weapon {
  string name;
  short  damage;
};

enum class e_( color, //
               Red,   //
               Green, //
               Blue   //
);

using MyVariant = std::variant< //
    int,                        //
    Point,                      //
    Weapon,                     //
    e_color                     //
    >;

// For std::variant.
template<typename Hint, typename... Ts>
auto serialize( FBBuilder& builder, std::variant<Ts...> const& o,
                ::rn::serial::rn_adl_tag ) {
  using tuple_t = fb_creation_tuple_t<Hint>;
  tuple_t t;
  // ---
  auto visitor = []( auto const& e ) {
    //
  };
  std::visit( visitor, o );
  // auto s_value =
  //    serialize<void>( builder, *o, ::rn::serial::rn_adl_tag{}
  //    );
  // ---
  auto apply_with_builder = [&]( auto... ts ) {
    return Hint::Create( builder, ts... );
  };
  return ReturnValue{std::apply( apply_with_builder, t )};
}

void test_fb() {
  using fb_table_t = ::fb::MyVariant;
  MyVariant v      = Weapon{"hello", 3};
  FBBuilder fbb;
  auto      res = serialize<fb_table_t>( fbb, v,
                                    ::rn::serial::rn_adl_tag{} );
}

} // namespace rn::serial
