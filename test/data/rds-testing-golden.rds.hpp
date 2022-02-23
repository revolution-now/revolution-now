// Auto-Generated file, do not modify! (testing.rds).
#pragma once

/****************************************************************
*                           Includes
*****************************************************************/
// Includes specified in rds file.
#include "maybe.hpp"
#include <string>
#include <vector>
#include <unordered_map>

// Revolution Now
#include "core-config.hpp"

// refl
#include "refl/ext.hpp"

// base
#include "base/variant.hpp"

// C++ standard library
#include <array>
#include <string_view>
#include <tuple>

/****************************************************************
*                          Global Vars
*****************************************************************/
namespace rn {

  // This will be the naem of this header, not the file that it
  // is include in.
  inline constexpr std::string_view rds_testing_genfile = __FILE__;

} // namespace rn

/****************************************************************
*                        Sum Type: Maybe
*****************************************************************/
namespace rdstest {

  namespace Maybe {

    template<typename T>
    struct nothing {
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct nothing const& ) const = default;
      bool operator!=( struct nothing const& ) const = default;
    };

    template<typename T>
    struct just {
      T val;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct just const& ) const = default;
      bool operator!=( struct just const& ) const = default;
    };

    enum class e {
      nothing,
      just,
    };

  } // namespace Maybe

  template<typename T>
  using Maybe_t = base::variant<
    Maybe::nothing<T>,
    Maybe::just<T>
  >;
  NOTHROW_MOVE( Maybe_t<int> );

} // namespace rdstest

// This gives us the enum to use in a switch statement.
template<typename T>
struct base::variant_to_enum<rdstest::Maybe_t<T>> {
  using type = rdstest::Maybe::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct nothing.
  template<typename T>
  struct traits<rdstest::Maybe::nothing<T>> {
    using type = rdstest::Maybe::nothing<T>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::Maybe";
    static constexpr std::string_view name = "nothing";

    using template_types = std::tuple<T>;

    static constexpr std::tuple fields{};
  };

  // Reflection info for struct just.
  template<typename T>
  struct traits<rdstest::Maybe::just<T>> {
    using type = rdstest::Maybe::just<T>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::Maybe";
    static constexpr std::string_view name = "just";

    using template_types = std::tuple<T>;

    static constexpr std::tuple fields{
      refl::StructField{ "val", &rdstest::Maybe::just<T>::val, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                     Sum Type: MyVariant0
*****************************************************************/
namespace rdstest {

  using MyVariant0_t = std::monostate;

} // namespace rdstest

/****************************************************************
*                     Sum Type: MyVariant1
*****************************************************************/
namespace rdstest {

  namespace MyVariant1 {

    struct happy {
      std::pair<char, int> p;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct happy const& ) const = default;
      bool operator!=( struct happy const& ) const = default;
    };

    struct sad {
      bool  hello;
      bool* ptr;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct sad const& ) const = default;
      bool operator!=( struct sad const& ) const = default;
    };

    struct excited {
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct excited const& ) const = default;
      bool operator!=( struct excited const& ) const = default;
    };

    enum class e {
      happy,
      sad,
      excited,
    };

  } // namespace MyVariant1

  using MyVariant1_t = base::variant<
    MyVariant1::happy,
    MyVariant1::sad,
    MyVariant1::excited
  >;
  NOTHROW_MOVE( MyVariant1_t );

} // namespace rdstest

// This gives us the enum to use in a switch statement.
template<>
struct base::variant_to_enum<rdstest::MyVariant1_t> {
  using type = rdstest::MyVariant1::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct happy.
  template<>
  struct traits<rdstest::MyVariant1::happy> {
    using type = rdstest::MyVariant1::happy;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::MyVariant1";
    static constexpr std::string_view name = "happy";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "p", &rdstest::MyVariant1::happy::p, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct sad.
  template<>
  struct traits<rdstest::MyVariant1::sad> {
    using type = rdstest::MyVariant1::sad;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::MyVariant1";
    static constexpr std::string_view name = "sad";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "hello", &rdstest::MyVariant1::sad::hello, /*offset=*/base::nothing },
      refl::StructField{ "ptr", &rdstest::MyVariant1::sad::ptr, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct excited.
  template<>
  struct traits<rdstest::MyVariant1::excited> {
    using type = rdstest::MyVariant1::excited;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::MyVariant1";
    static constexpr std::string_view name = "excited";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{};
  };

} // namespace refl

/****************************************************************
*                     Sum Type: MyVariant2
*****************************************************************/
namespace rdstest {

  namespace MyVariant2 {

    struct first {
      std::string name;
      bool        b;
    };

    struct second {
      bool flag1;
      bool flag2;
    };

    struct third {
      int cost;
    };

    enum class e {
      first,
      second,
      third,
    };

  } // namespace MyVariant2

  using MyVariant2_t = base::variant<
    MyVariant2::first,
    MyVariant2::second,
    MyVariant2::third
  >;
  NOTHROW_MOVE( MyVariant2_t );

} // namespace rdstest

// This gives us the enum to use in a switch statement.
template<>
struct base::variant_to_enum<rdstest::MyVariant2_t> {
  using type = rdstest::MyVariant2::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct first.
  template<>
  struct traits<rdstest::MyVariant2::first> {
    using type = rdstest::MyVariant2::first;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::MyVariant2";
    static constexpr std::string_view name = "first";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "name", &rdstest::MyVariant2::first::name, /*offset=*/base::nothing },
      refl::StructField{ "b", &rdstest::MyVariant2::first::b, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct second.
  template<>
  struct traits<rdstest::MyVariant2::second> {
    using type = rdstest::MyVariant2::second;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::MyVariant2";
    static constexpr std::string_view name = "second";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "flag1", &rdstest::MyVariant2::second::flag1, /*offset=*/base::nothing },
      refl::StructField{ "flag2", &rdstest::MyVariant2::second::flag2, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct third.
  template<>
  struct traits<rdstest::MyVariant2::third> {
    using type = rdstest::MyVariant2::third;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::MyVariant2";
    static constexpr std::string_view name = "third";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "cost", &rdstest::MyVariant2::third::cost, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                     Sum Type: MyVariant3
*****************************************************************/
namespace rdstest::inner {

  namespace MyVariant3 {

    struct a1 {
      MyVariant0_t var0;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct a1 const& ) const = default;
      bool operator!=( struct a1 const& ) const = default;
    };

    struct a2 {
      MyVariant0_t var1;
      MyVariant2_t var2;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct a2 const& ) const = default;
      bool operator!=( struct a2 const& ) const = default;
    };

    struct a3 {
      char c;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct a3 const& ) const = default;
      bool operator!=( struct a3 const& ) const = default;
    };

    enum class e {
      a1,
      a2,
      a3,
    };

  } // namespace MyVariant3

  using MyVariant3_t = base::variant<
    MyVariant3::a1,
    MyVariant3::a2,
    MyVariant3::a3
  >;
  NOTHROW_MOVE( MyVariant3_t );

} // namespace rdstest::inner

// This gives us the enum to use in a switch statement.
template<>
struct base::variant_to_enum<rdstest::inner::MyVariant3_t> {
  using type = rdstest::inner::MyVariant3::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct a1.
  template<>
  struct traits<rdstest::inner::MyVariant3::a1> {
    using type = rdstest::inner::MyVariant3::a1;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::MyVariant3";
    static constexpr std::string_view name = "a1";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "var0", &rdstest::inner::MyVariant3::a1::var0, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct a2.
  template<>
  struct traits<rdstest::inner::MyVariant3::a2> {
    using type = rdstest::inner::MyVariant3::a2;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::MyVariant3";
    static constexpr std::string_view name = "a2";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "var1", &rdstest::inner::MyVariant3::a2::var1, /*offset=*/base::nothing },
      refl::StructField{ "var2", &rdstest::inner::MyVariant3::a2::var2, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct a3.
  template<>
  struct traits<rdstest::inner::MyVariant3::a3> {
    using type = rdstest::inner::MyVariant3::a3;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::MyVariant3";
    static constexpr std::string_view name = "a3";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "c", &rdstest::inner::MyVariant3::a3::c, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                     Sum Type: MyVariant4
*****************************************************************/
namespace rdstest::inner {

  namespace MyVariant4 {

    struct first {
      int                 i;
      char                c;
      bool                b;
      rn::maybe<uint32_t> op;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct first const& ) const = default;
      bool operator!=( struct first const& ) const = default;
    };

    struct _2nd {
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct _2nd const& ) const = default;
      bool operator!=( struct _2nd const& ) const = default;
    };

    struct third {
      std::string  s;
      MyVariant3_t var3;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct third const& ) const = default;
      bool operator!=( struct third const& ) const = default;
    };

    enum class e {
      first,
      _2nd,
      third,
    };

  } // namespace MyVariant4

  using MyVariant4_t = base::variant<
    MyVariant4::first,
    MyVariant4::_2nd,
    MyVariant4::third
  >;
  NOTHROW_MOVE( MyVariant4_t );

} // namespace rdstest::inner

// This gives us the enum to use in a switch statement.
template<>
struct base::variant_to_enum<rdstest::inner::MyVariant4_t> {
  using type = rdstest::inner::MyVariant4::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct first.
  template<>
  struct traits<rdstest::inner::MyVariant4::first> {
    using type = rdstest::inner::MyVariant4::first;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::MyVariant4";
    static constexpr std::string_view name = "first";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "i", &rdstest::inner::MyVariant4::first::i, /*offset=*/base::nothing },
      refl::StructField{ "c", &rdstest::inner::MyVariant4::first::c, /*offset=*/base::nothing },
      refl::StructField{ "b", &rdstest::inner::MyVariant4::first::b, /*offset=*/base::nothing },
      refl::StructField{ "op", &rdstest::inner::MyVariant4::first::op, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct _2nd.
  template<>
  struct traits<rdstest::inner::MyVariant4::_2nd> {
    using type = rdstest::inner::MyVariant4::_2nd;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::MyVariant4";
    static constexpr std::string_view name = "_2nd";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{};
  };

  // Reflection info for struct third.
  template<>
  struct traits<rdstest::inner::MyVariant4::third> {
    using type = rdstest::inner::MyVariant4::third;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::MyVariant4";
    static constexpr std::string_view name = "third";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "s", &rdstest::inner::MyVariant4::third::s, /*offset=*/base::nothing },
      refl::StructField{ "var3", &rdstest::inner::MyVariant4::third::var3, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                  Sum Type: TemplateTwoParams
*****************************************************************/
namespace rdstest::inner {

  namespace TemplateTwoParams {

    template<typename T, typename U>
    struct first_alternative {
      T    t;
      char c;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct first_alternative const& ) const = default;
      bool operator!=( struct first_alternative const& ) const = default;
    };

    template<typename T, typename U>
    struct second_alternative {
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct second_alternative const& ) const = default;
      bool operator!=( struct second_alternative const& ) const = default;
    };

    template<typename T, typename U>
    struct third_alternative {
      Maybe_t<T> hello;
      U          u;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct third_alternative const& ) const = default;
      bool operator!=( struct third_alternative const& ) const = default;
    };

    enum class e {
      first_alternative,
      second_alternative,
      third_alternative,
    };

  } // namespace TemplateTwoParams

  template<typename T, typename U>
  using TemplateTwoParams_t = base::variant<
    TemplateTwoParams::first_alternative<T, U>,
    TemplateTwoParams::second_alternative<T, U>,
    TemplateTwoParams::third_alternative<T, U>
  >;
  NOTHROW_MOVE( TemplateTwoParams_t<int, int> );

} // namespace rdstest::inner

// This gives us the enum to use in a switch statement.
template<typename T, typename U>
struct base::variant_to_enum<rdstest::inner::TemplateTwoParams_t<T, U>> {
  using type = rdstest::inner::TemplateTwoParams::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct first_alternative.
  template<typename T, typename U>
  struct traits<rdstest::inner::TemplateTwoParams::first_alternative<T, U>> {
    using type = rdstest::inner::TemplateTwoParams::first_alternative<T, U>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::TemplateTwoParams";
    static constexpr std::string_view name = "first_alternative";

    using template_types = std::tuple<T, U>;

    static constexpr std::tuple fields{
      refl::StructField{ "t", &rdstest::inner::TemplateTwoParams::first_alternative<T, U>::t, /*offset=*/base::nothing },
      refl::StructField{ "c", &rdstest::inner::TemplateTwoParams::first_alternative<T, U>::c, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct second_alternative.
  template<typename T, typename U>
  struct traits<rdstest::inner::TemplateTwoParams::second_alternative<T, U>> {
    using type = rdstest::inner::TemplateTwoParams::second_alternative<T, U>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::TemplateTwoParams";
    static constexpr std::string_view name = "second_alternative";

    using template_types = std::tuple<T, U>;

    static constexpr std::tuple fields{};
  };

  // Reflection info for struct third_alternative.
  template<typename T, typename U>
  struct traits<rdstest::inner::TemplateTwoParams::third_alternative<T, U>> {
    using type = rdstest::inner::TemplateTwoParams::third_alternative<T, U>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::TemplateTwoParams";
    static constexpr std::string_view name = "third_alternative";

    using template_types = std::tuple<T, U>;

    static constexpr std::tuple fields{
      refl::StructField{ "hello", &rdstest::inner::TemplateTwoParams::third_alternative<T, U>::hello, /*offset=*/base::nothing },
      refl::StructField{ "u", &rdstest::inner::TemplateTwoParams::third_alternative<T, U>::u, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                Sum Type: CompositeTemplateTwo
*****************************************************************/
namespace rdstest::inner {

  namespace CompositeTemplateTwo {

    template<typename T, typename U>
    struct first {
      rdstest::inner::TemplateTwoParams_t<T,U> ttp;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct first const& ) const = default;
      bool operator!=( struct first const& ) const = default;
    };

    template<typename T, typename U>
    struct second {
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct second const& ) const = default;
      bool operator!=( struct second const& ) const = default;
    };

    enum class e {
      first,
      second,
    };

  } // namespace CompositeTemplateTwo

  template<typename T, typename U>
  using CompositeTemplateTwo_t = base::variant<
    CompositeTemplateTwo::first<T, U>,
    CompositeTemplateTwo::second<T, U>
  >;
  NOTHROW_MOVE( CompositeTemplateTwo_t<int, int> );

} // namespace rdstest::inner

// This gives us the enum to use in a switch statement.
template<typename T, typename U>
struct base::variant_to_enum<rdstest::inner::CompositeTemplateTwo_t<T, U>> {
  using type = rdstest::inner::CompositeTemplateTwo::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct first.
  template<typename T, typename U>
  struct traits<rdstest::inner::CompositeTemplateTwo::first<T, U>> {
    using type = rdstest::inner::CompositeTemplateTwo::first<T, U>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::CompositeTemplateTwo";
    static constexpr std::string_view name = "first";

    using template_types = std::tuple<T, U>;

    static constexpr std::tuple fields{
      refl::StructField{ "ttp", &rdstest::inner::CompositeTemplateTwo::first<T, U>::ttp, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct second.
  template<typename T, typename U>
  struct traits<rdstest::inner::CompositeTemplateTwo::second<T, U>> {
    using type = rdstest::inner::CompositeTemplateTwo::second<T, U>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::CompositeTemplateTwo";
    static constexpr std::string_view name = "second";

    using template_types = std::tuple<T, U>;

    static constexpr std::tuple fields{};
  };

} // namespace refl

/****************************************************************
*                      Sum Type: MySumtype
*****************************************************************/
namespace rn {

  namespace MySumtype {

    struct none {
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct none const& ) const = default;
      bool operator!=( struct none const& ) const = default;
    };

    struct some {
      std::string s;
      int         y;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct some const& ) const = default;
      bool operator!=( struct some const& ) const = default;
    };

    struct more {
      double d;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct more const& ) const = default;
      bool operator!=( struct more const& ) const = default;
    };

    enum class e {
      none,
      some,
      more,
    };

  } // namespace MySumtype

  using MySumtype_t = base::variant<
    MySumtype::none,
    MySumtype::some,
    MySumtype::more
  >;
  NOTHROW_MOVE( MySumtype_t );

} // namespace rn

// This gives us the enum to use in a switch statement.
template<>
struct base::variant_to_enum<rn::MySumtype_t> {
  using type = rn::MySumtype::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct none.
  template<>
  struct traits<rn::MySumtype::none> {
    using type = rn::MySumtype::none;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::MySumtype";
    static constexpr std::string_view name = "none";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{};
  };

  // Reflection info for struct some.
  template<>
  struct traits<rn::MySumtype::some> {
    using type = rn::MySumtype::some;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::MySumtype";
    static constexpr std::string_view name = "some";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "s", &rn::MySumtype::some::s, /*offset=*/base::nothing },
      refl::StructField{ "y", &rn::MySumtype::some::y, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct more.
  template<>
  struct traits<rn::MySumtype::more> {
    using type = rn::MySumtype::more;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::MySumtype";
    static constexpr std::string_view name = "more";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "d", &rn::MySumtype::more::d, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                     Sum Type: OnOffState
*****************************************************************/
namespace rn {

  namespace OnOffState {

    struct off {
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct off const& ) const = default;
      bool operator!=( struct off const& ) const = default;
    };

    struct on {
      std::string user;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct on const& ) const = default;
      bool operator!=( struct on const& ) const = default;
    };

    struct switching_on {
      double percent;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct switching_on const& ) const = default;
      bool operator!=( struct switching_on const& ) const = default;
    };

    struct switching_off {
      double percent;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct switching_off const& ) const = default;
      bool operator!=( struct switching_off const& ) const = default;
    };

    enum class e {
      off,
      on,
      switching_on,
      switching_off,
    };

  } // namespace OnOffState

  using OnOffState_t = base::variant<
    OnOffState::off,
    OnOffState::on,
    OnOffState::switching_on,
    OnOffState::switching_off
  >;
  NOTHROW_MOVE( OnOffState_t );

} // namespace rn

// This gives us the enum to use in a switch statement.
template<>
struct base::variant_to_enum<rn::OnOffState_t> {
  using type = rn::OnOffState::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct off.
  template<>
  struct traits<rn::OnOffState::off> {
    using type = rn::OnOffState::off;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::OnOffState";
    static constexpr std::string_view name = "off";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{};
  };

  // Reflection info for struct on.
  template<>
  struct traits<rn::OnOffState::on> {
    using type = rn::OnOffState::on;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::OnOffState";
    static constexpr std::string_view name = "on";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "user", &rn::OnOffState::on::user, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct switching_on.
  template<>
  struct traits<rn::OnOffState::switching_on> {
    using type = rn::OnOffState::switching_on;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::OnOffState";
    static constexpr std::string_view name = "switching_on";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "percent", &rn::OnOffState::switching_on::percent, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct switching_off.
  template<>
  struct traits<rn::OnOffState::switching_off> {
    using type = rn::OnOffState::switching_off;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::OnOffState";
    static constexpr std::string_view name = "switching_off";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "percent", &rn::OnOffState::switching_off::percent, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                     Sum Type: OnOffEvent
*****************************************************************/
namespace rn {

  namespace OnOffEvent {

    struct turn_off {
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct turn_off const& ) const = default;
      bool operator!=( struct turn_off const& ) const = default;
    };

    struct turn_on {
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct turn_on const& ) const = default;
      bool operator!=( struct turn_on const& ) const = default;
    };

    enum class e {
      turn_off,
      turn_on,
    };

  } // namespace OnOffEvent

  using OnOffEvent_t = base::variant<
    OnOffEvent::turn_off,
    OnOffEvent::turn_on
  >;
  NOTHROW_MOVE( OnOffEvent_t );

} // namespace rn

// This gives us the enum to use in a switch statement.
template<>
struct base::variant_to_enum<rn::OnOffEvent_t> {
  using type = rn::OnOffEvent::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct turn_off.
  template<>
  struct traits<rn::OnOffEvent::turn_off> {
    using type = rn::OnOffEvent::turn_off;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::OnOffEvent";
    static constexpr std::string_view name = "turn_off";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{};
  };

  // Reflection info for struct turn_on.
  template<>
  struct traits<rn::OnOffEvent::turn_on> {
    using type = rn::OnOffEvent::turn_on;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::OnOffEvent";
    static constexpr std::string_view name = "turn_on";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{};
  };

} // namespace refl

/****************************************************************
*                         Enum: e_empty
*****************************************************************/
namespace rn {

  enum class e_empty {
  };

} // namespace rn

namespace refl {

  // Reflection info for enum e_empty.
  template<>
  struct traits<rn::e_empty> {
    using type = rn::e_empty;

    static constexpr type_kind kind        = type_kind::enum_kind;
    static constexpr std::string_view ns   = "rn";
    static constexpr std::string_view name = "e_empty";

    static constexpr std::array<std::string_view, 0> value_names{};
  };

} // namespace refl

/****************************************************************
*                        Enum: e_single
*****************************************************************/
namespace rn {

  enum class e_single {
    hello
  };

} // namespace rn

namespace refl {

  // Reflection info for enum e_single.
  template<>
  struct traits<rn::e_single> {
    using type = rn::e_single;

    static constexpr type_kind kind        = type_kind::enum_kind;
    static constexpr std::string_view ns   = "rn";
    static constexpr std::string_view name = "e_single";

    static constexpr std::array<std::string_view, 1> value_names{
      "hello",
    };
  };

} // namespace refl

/****************************************************************
*                          Enum: e_two
*****************************************************************/
namespace rn {

  enum class e_two {
    hello,
    world
  };

} // namespace rn

namespace refl {

  // Reflection info for enum e_two.
  template<>
  struct traits<rn::e_two> {
    using type = rn::e_two;

    static constexpr type_kind kind        = type_kind::enum_kind;
    static constexpr std::string_view ns   = "rn";
    static constexpr std::string_view name = "e_two";

    static constexpr std::array<std::string_view, 2> value_names{
      "hello",
      "world",
    };
  };

} // namespace refl

/****************************************************************
*                         Enum: e_color
*****************************************************************/
namespace rn {

  enum class e_color {
    red,
    green,
    blue
  };

} // namespace rn

namespace refl {

  // Reflection info for enum e_color.
  template<>
  struct traits<rn::e_color> {
    using type = rn::e_color;

    static constexpr type_kind kind        = type_kind::enum_kind;
    static constexpr std::string_view ns   = "rn";
    static constexpr std::string_view name = "e_color";

    static constexpr std::array<std::string_view, 3> value_names{
      "red",
      "green",
      "blue",
    };
  };

} // namespace refl

/****************************************************************
*                         Enum: e_hand
*****************************************************************/
namespace rn {

  enum class e_hand {
    left,
    right
  };

} // namespace rn

namespace refl {

  // Reflection info for enum e_hand.
  template<>
  struct traits<rn::e_hand> {
    using type = rn::e_hand;

    static constexpr type_kind kind        = type_kind::enum_kind;
    static constexpr std::string_view ns   = "rn";
    static constexpr std::string_view name = "e_hand";

    static constexpr std::array<std::string_view, 2> value_names{
      "left",
      "right",
    };
  };

} // namespace refl

/****************************************************************
*                      Struct: EmptyStruct
*****************************************************************/
namespace rn {

  struct EmptyStruct {
    bool operator==( EmptyStruct const& ) const = default;
  };

} // namespace rn

namespace refl {

  // Reflection info for struct EmptyStruct.
  template<>
  struct traits<rn::EmptyStruct> {
    using type = rn::EmptyStruct;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn";
    static constexpr std::string_view name = "EmptyStruct";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{};
  };

} // namespace refl

/****************************************************************
*                     Struct: EmptyStruct2
*****************************************************************/
namespace rn {

  struct EmptyStruct2 {};

} // namespace rn

namespace refl {

  // Reflection info for struct EmptyStruct2.
  template<>
  struct traits<rn::EmptyStruct2> {
    using type = rn::EmptyStruct2;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn";
    static constexpr std::string_view name = "EmptyStruct2";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{};
  };

} // namespace refl

/****************************************************************
*                       Struct: MyStruct
*****************************************************************/
namespace rn {

  struct MyStruct {
    int                                          xxx     = {};
    double                                       yyy     = {};
    std::unordered_map<std::string, std::string> zzz_map = {};

    bool operator==( MyStruct const& ) const = default;
  };

} // namespace rn

namespace refl {

  // Reflection info for struct MyStruct.
  template<>
  struct traits<rn::MyStruct> {
    using type = rn::MyStruct;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn";
    static constexpr std::string_view name = "MyStruct";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "xxx", &rn::MyStruct::xxx, offsetof( type, xxx ) },
      refl::StructField{ "yyy", &rn::MyStruct::yyy, offsetof( type, yyy ) },
      refl::StructField{ "zzz_map", &rn::MyStruct::zzz_map, offsetof( type, zzz_map ) },
    };
  };

} // namespace refl

/****************************************************************
*                 Struct: StructWithValidation
*****************************************************************/
namespace rn {

  struct StructWithValidation {
    int    xxx = {};
    double yyy = {};

    bool operator==( StructWithValidation const& ) const = default;

    // Validates invariants among members.
    // defined in some translation unit.
    base::valid_or<std::string> validate() const;
  };

} // namespace rn

namespace refl {

  // Reflection info for struct StructWithValidation.
  template<>
  struct traits<rn::StructWithValidation> {
    using type = rn::StructWithValidation;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn";
    static constexpr std::string_view name = "StructWithValidation";

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "xxx", &rn::StructWithValidation::xxx, /*offset=*/base::nothing },
      refl::StructField{ "yyy", &rn::StructWithValidation::yyy, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                   Struct: MyTemplateStruct
*****************************************************************/
namespace rn::test {

  template<typename T, typename U>
  struct MyTemplateStruct {
    T                                  xxx     = {};
    double                             yyy     = {};
    std::unordered_map<std::string, U> zzz_map = {};

    bool operator==( MyTemplateStruct const& ) const = default;
  };

} // namespace rn::test

namespace refl {

  // Reflection info for struct MyTemplateStruct.
  template<typename T, typename U>
  struct traits<rn::test::MyTemplateStruct<T, U>> {
    using type = rn::test::MyTemplateStruct<T, U>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::test";
    static constexpr std::string_view name = "MyTemplateStruct";

    using template_types = std::tuple<T, U>;

    static constexpr std::tuple fields{
      refl::StructField{ "xxx", &rn::test::MyTemplateStruct<T, U>::xxx, /*offset=*/base::nothing },
      refl::StructField{ "yyy", &rn::test::MyTemplateStruct<T, U>::yyy, /*offset=*/base::nothing },
      refl::StructField{ "zzz_map", &rn::test::MyTemplateStruct<T, U>::zzz_map, /*offset=*/base::nothing },
    };
  };

} // namespace refl
