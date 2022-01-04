/****************************************************************
**refl.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-04.
*
* Description: Unit tests for the src/base/refl.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/refl.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace base {

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

enum class non_reflected_enum { yes, no };

enum class empty_enum {};

enum class my_enum { red, blue, green };

} // namespace my_ns

template<>
struct refl_traits<my_ns::MyEmptyStruct> {
  using type = my_ns::MyEmptyStruct;
  static constexpr refl_type_kind kind =
      refl_type_kind::struct_kind;
  static constexpr std::string_view ns   = "my_ns";
  static constexpr std::string_view name = "MyEmptyStruct";

  // Struct specific.
  static constexpr std::tuple<> template_types;

  static constexpr std::tuple fields{};
};

template<>
struct refl_traits<my_ns::MyStruct> {
  using type = my_ns::MyStruct;
  static constexpr refl_type_kind kind =
      refl_type_kind::struct_kind;
  static constexpr std::string_view ns   = "my_ns";
  static constexpr std::string_view name = "MyStruct";

  // Struct specific.
  static constexpr std::tuple<> template_types;

  static constexpr std::tuple fields{
      ReflectedStructField{ "x", &my_ns::MyStruct::x },
      ReflectedStructField{ "y", &my_ns::MyStruct::y },
  };
};

template<typename U, typename V>
struct refl_traits<my_ns::MyTmpStruct<U, V>> {
  using type = my_ns::MyTmpStruct<U, V>;
  static constexpr refl_type_kind kind =
      refl_type_kind::struct_kind;
  static constexpr std::string_view ns   = "my_ns";
  static constexpr std::string_view name = "MyStruct";

  // Struct specific.
  static constexpr std::tuple<U, V> template_types;

  static constexpr std::tuple fields{
      ReflectedStructField{ "x", &my_ns::MyTmpStruct<U, V>::x },
      ReflectedStructField{ "y", &my_ns::MyTmpStruct<U, V>::y },
  };
};

template<>
struct refl_traits<my_ns::empty_enum> {
  using type = my_ns::empty_enum;
  static constexpr refl_type_kind kind =
      refl_type_kind::enum_kind;
  static constexpr std::string_view ns   = "my_ns";
  static constexpr std::string_view name = "empty_enum";

  // Enum specific.
  static constexpr std::array<std::string_view, 0> value_names{};
};

template<>
struct refl_traits<my_ns::my_enum> {
  using type = my_ns::my_enum;
  static constexpr refl_type_kind kind =
      refl_type_kind::enum_kind;
  static constexpr std::string_view ns   = "my_ns";
  static constexpr std::string_view name = "my_enum";

  // Enum specific.
  static constexpr std::array<std::string_view, 3> value_names{
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
    std::tuple_size_v<
        decltype( refl_traits<my_ns::MyEmptyStruct>::fields )> ==
    0 );
static_assert( std::tuple_size_v<
                   decltype( refl_traits<my_ns::MyEmptyStruct>::
                                 template_types )> == 0 );

// MyStruct
static_assert( Reflected<my_ns::MyStruct> );
static_assert( Reflected<my_ns::MyStruct> );
static_assert( ReflectedStruct<my_ns::MyStruct> );
static_assert( !ReflectedEnum<my_ns::MyStruct> );

static_assert(
    std::tuple_size_v<decltype( refl_traits<my_ns::MyStruct>::
                                    template_types )> == 0 );
static_assert(
    std::tuple_size_v<
        decltype( refl_traits<my_ns::MyStruct>::fields )> == 2 );
constexpr auto& MyStruct_field_0 =
    std::get<0>( refl_traits<my_ns::MyStruct>::fields );
constexpr auto& MyStruct_field_1 =
    std::get<1>( refl_traits<my_ns::MyStruct>::fields );
using MyStruct_field_0_t =
    std::remove_cvref_t<decltype( MyStruct_field_0 )>;
using MyStruct_field_1_t =
    std::remove_cvref_t<decltype( MyStruct_field_1 )>;
static_assert( MyStruct_field_0.name == "x" );
static_assert( MyStruct_field_1.name == "y" );
static_assert(
    std::is_same_v<typename MyStruct_field_0_t::type, int> );
static_assert(
    std::is_same_v<typename MyStruct_field_1_t::type, double> );

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

} // namespace
} // namespace base
