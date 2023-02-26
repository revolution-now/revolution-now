/****************************************************************
**ext.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-04.
*
* Description: Unit tests for the src/refl/ext.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/refl/ext.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace refl {

using namespace std;

/****************************************************************
** Test Data Types
*****************************************************************/
namespace my_ns {

struct NonReflectedStruct {
  int x;
};

struct MyEmptyStruct {};

struct MyStruct {
  int    x;
  double y;
};

template<typename U, typename V>
struct MyTmpStruct {
  U x;
  V y;
};

struct Wrapper {
  Wrapper( MyStruct const& );

  // Implement refl::WrapsReflected.
  MyStruct const&              refl() const;
  static constexpr string_view refl_ns   = "my_ns";
  static constexpr string_view refl_name = "Wrapper";

  MyStruct wrapped;
};

enum class non_reflected_enum { yes, no };

enum class empty_enum {};

enum class my_enum { red, blue, green };

} // namespace my_ns

template<>
struct traits<my_ns::MyEmptyStruct> {
  using type                        = my_ns::MyEmptyStruct;
  static constexpr type_kind   kind = type_kind::struct_kind;
  static constexpr string_view ns   = "my_ns";
  static constexpr string_view name = "MyEmptyStruct";
  static constexpr bool        is_sumtype_alternative = false;

  // Struct specific.
  using template_types = tuple<>;

  static constexpr tuple fields{};
};

template<>
struct traits<my_ns::MyStruct> {
  using type                        = my_ns::MyStruct;
  static constexpr type_kind   kind = type_kind::struct_kind;
  static constexpr string_view ns   = "my_ns";
  static constexpr string_view name = "MyStruct";
  static constexpr bool        is_sumtype_alternative = false;

  // Struct specific.
  using template_types = tuple<>;

  static constexpr tuple fields{
      StructField{ "x", &my_ns::MyStruct::x, base::nothing },
      StructField{ "y", &my_ns::MyStruct::y,
                   offsetof( type, y ) },
  };
};

template<typename U, typename V>
struct traits<my_ns::MyTmpStruct<U, V>> {
  using type                        = my_ns::MyTmpStruct<U, V>;
  static constexpr type_kind   kind = type_kind::struct_kind;
  static constexpr string_view ns   = "my_ns";
  static constexpr string_view name = "MyStruct";
  static constexpr bool        is_sumtype_alternative = false;

  // Struct specific.
  using template_types = tuple<U, V>;

  static constexpr tuple fields{
      StructField{ "x", &my_ns::MyTmpStruct<U, V>::x,
                   offsetof( type, x ) },
      StructField{ "y", &my_ns::MyTmpStruct<U, V>::y,
                   offsetof( type, y ) },
  };
};

template<>
struct traits<my_ns::empty_enum> {
  using type                        = my_ns::empty_enum;
  static constexpr type_kind   kind = type_kind::enum_kind;
  static constexpr string_view ns   = "my_ns";
  static constexpr string_view name = "empty_enum";

  // Enum specific.
  static constexpr array<string_view, 0> value_names{};
};

template<>
struct traits<my_ns::my_enum> {
  using type                        = my_ns::my_enum;
  static constexpr type_kind   kind = type_kind::enum_kind;
  static constexpr string_view ns   = "my_ns";
  static constexpr string_view name = "my_enum";

  // Enum specific.
  static constexpr array<string_view, 3> value_names{
      "red",
      "blue",
      "green",
  };
};

namespace {

/****************************************************************
** struct reflection.
*****************************************************************/
// NonReflectedStruct
static_assert( !Reflected<my_ns::NonReflectedStruct> );
static_assert( !ReflectedStruct<my_ns::NonReflectedStruct> );
static_assert( !ReflectedEnum<my_ns::NonReflectedStruct> );
static_assert( !Reflected<my_ns::NonReflectedStruct> );

// MyEmptyStruct
static_assert( Reflected<my_ns::MyEmptyStruct> );
static_assert( Reflected<my_ns::MyEmptyStruct> );
static_assert( ReflectedStruct<my_ns::MyEmptyStruct> );
static_assert( !ReflectedEnum<my_ns::MyEmptyStruct> );

static_assert(
    tuple_size_v<
        decltype( traits<my_ns::MyEmptyStruct>::fields )> == 0 );
static_assert(
    tuple_size_v<traits<my_ns::MyEmptyStruct>::template_types> ==
    0 );

// MyStruct
static_assert( Reflected<my_ns::MyStruct> );
static_assert( Reflected<my_ns::MyStruct> );
static_assert( ReflectedStruct<my_ns::MyStruct> );
static_assert( !ReflectedEnum<my_ns::MyStruct> );

static_assert(
    tuple_size_v<traits<my_ns::MyStruct>::template_types> == 0 );
static_assert(
    tuple_size_v<decltype( traits<my_ns::MyStruct>::fields )> ==
    2 );
constexpr auto& MyStruct_field_0 =
    get<0>( traits<my_ns::MyStruct>::fields );
constexpr auto& MyStruct_field_1 =
    get<1>( traits<my_ns::MyStruct>::fields );
using MyStruct_field_0_t =
    remove_cvref_t<decltype( MyStruct_field_0 )>;
using MyStruct_field_1_t =
    remove_cvref_t<decltype( MyStruct_field_1 )>;
static_assert( MyStruct_field_0.name == "x" );
static_assert( MyStruct_field_1.name == "y" );
static_assert(
    is_same_v<typename MyStruct_field_0_t::type, int> );
static_assert(
    is_same_v<typename MyStruct_field_1_t::type, double> );

// MyTmpStruct
static_assert( Reflected<my_ns::MyTmpStruct<int, int>> );
static_assert( Reflected<my_ns::MyTmpStruct<int, int>> );
static_assert( ReflectedStruct<my_ns::MyTmpStruct<int, int>> );
static_assert( !ReflectedEnum<my_ns::MyTmpStruct<int, int>> );

/****************************************************************
** enum reflection.
*****************************************************************/
// non_reflected_enum
static_assert( !Reflected<my_ns::non_reflected_enum> );
static_assert( !ReflectedEnum<my_ns::non_reflected_enum> );
static_assert( !ReflectedStruct<my_ns::non_reflected_enum> );
static_assert( !Reflected<my_ns::non_reflected_enum> );

// empty_enum
static_assert( Reflected<my_ns::empty_enum> );
static_assert( Reflected<my_ns::empty_enum> );
static_assert( ReflectedEnum<my_ns::empty_enum> );
static_assert( !ReflectedStruct<my_ns::empty_enum> );

// my_enum
static_assert( Reflected<my_ns::my_enum> );
static_assert( Reflected<my_ns::my_enum> );
static_assert( ReflectedEnum<my_ns::my_enum> );
static_assert( !ReflectedStruct<my_ns::my_enum> );

/****************************************************************
** wrappers.
*****************************************************************/
static_assert( WrapsReflected<my_ns::Wrapper> );
static_assert( !Reflected<my_ns::Wrapper> );
static_assert( !ReflectedStruct<my_ns::Wrapper> );
static_assert( !ReflectedEnum<my_ns::Wrapper> );

} // namespace
} // namespace refl
