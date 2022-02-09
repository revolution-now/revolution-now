// Auto-Generated file, do not modify! (testing).
#pragma once

/****************************************************************
*                           Includes
*****************************************************************/
// Includes specified in rds file.
#include "maybe.hpp"
#include "fb/testing_generated.h"
#include <string>
#include <vector>

// Revolution Now
#include "core-config.hpp"
#include "rds/helper/sumtype-helper.hpp"
#include "error.hpp"
#include "fb.hpp"
#include "maybe.hpp"

// refl
#include "refl/ext.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/to-str.hpp"
#include "base/to-str-ext-std.hpp"
#include "base/variant.hpp"

// base-util
#include "base-util/mp.hpp"

// {fmt}
#include "fmt/format.h"

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

    // nothing
    template<typename T>
    inline void to_str( Maybe::nothing<T> const&, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "Maybe::nothing<{}>"
      , ::base::type_list_to_names<T>() );
    }

    template<typename T>
    struct just {
      T val;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct just const& ) const = default;
      bool operator!=( struct just const& ) const = default;
    };

    // just
    template<typename T>
    inline void to_str( Maybe::just<T> const& o, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "Maybe::just<{}>{{"
          "val={}"
        "}}"
      , ::base::type_list_to_names<T>(), o.val );
    }

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
      refl::StructField{ "val", &rdstest::Maybe::just<T>::val },
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
      refl::StructField{ "p", &rdstest::MyVariant1::happy::p },
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
      refl::StructField{ "hello", &rdstest::MyVariant1::sad::hello },
      refl::StructField{ "ptr", &rdstest::MyVariant1::sad::ptr },
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
      using fb_target_t = fb::MyVariant2::first;

      rn::serial::FBOffset<fb::MyVariant2::first> serialize_table(
          rn::serial::FBBuilder& builder ) const {
        using ::rn::serial::serialize;
          
        auto s_name = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().name() )>>(
            builder, name, ::rn::serial::ADL{} );

        auto s_b = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().b() )>>(
            builder, b, ::rn::serial::ADL{} );

        // We must always serialize this table even if it is
        // empty/default-valued because, for variants, its presence
        // indicates that it is the active alternative.
        return fb::MyVariant2::Createfirst( builder
            , s_name.get(), s_b.get()
        );
      }

      static ::rn::valid_deserial_t deserialize_table(
          fb::MyVariant2::first const& src,
          first* dst ) {
        (void)src;
        (void)dst;
        DCHECK( dst );
        using ::rn::serial::deserialize;
          
        HAS_VALUE_OR_RET( deserialize(
            ::rn::serial::detail::to_const_ptr( src.name() ),
            &dst->name, ::rn::serial::ADL{} ) );

        HAS_VALUE_OR_RET( deserialize(
            ::rn::serial::detail::to_const_ptr( src.b() ),
            &dst->b, ::rn::serial::ADL{} ) );

        return ::rn::valid;
      }

      ::rn::valid_deserial_t check_invariants_safe() const {
        return ::rn::valid;
      }

    };

    // first
    inline void to_str( MyVariant2::first const& o, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "MyVariant2::first{{"
          "name={},"
          "b={}"
        "}}"
      , o.name, o.b );
    }

    struct second {
      bool flag1;
      bool flag2;
      using fb_target_t = fb::MyVariant2::second;

      rn::serial::FBOffset<fb::MyVariant2::second> serialize_table(
          rn::serial::FBBuilder& builder ) const {
        using ::rn::serial::serialize;
          
        auto s_flag1 = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().flag1() )>>(
            builder, flag1, ::rn::serial::ADL{} );

        auto s_flag2 = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().flag2() )>>(
            builder, flag2, ::rn::serial::ADL{} );

        // We must always serialize this table even if it is
        // empty/default-valued because, for variants, its presence
        // indicates that it is the active alternative.
        return fb::MyVariant2::Createsecond( builder
            , s_flag1.get(), s_flag2.get()
        );
      }

      static ::rn::valid_deserial_t deserialize_table(
          fb::MyVariant2::second const& src,
          second* dst ) {
        (void)src;
        (void)dst;
        DCHECK( dst );
        using ::rn::serial::deserialize;
          
        HAS_VALUE_OR_RET( deserialize(
            ::rn::serial::detail::to_const_ptr( src.flag1() ),
            &dst->flag1, ::rn::serial::ADL{} ) );

        HAS_VALUE_OR_RET( deserialize(
            ::rn::serial::detail::to_const_ptr( src.flag2() ),
            &dst->flag2, ::rn::serial::ADL{} ) );

        return ::rn::valid;
      }

      ::rn::valid_deserial_t check_invariants_safe() const {
        return ::rn::valid;
      }

    };

    // second
    inline void to_str( MyVariant2::second const& o, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "MyVariant2::second{{"
          "flag1={},"
          "flag2={}"
        "}}"
      , o.flag1, o.flag2 );
    }

    struct third {
      int cost;
      using fb_target_t = fb::MyVariant2::third;

      rn::serial::FBOffset<fb::MyVariant2::third> serialize_table(
          rn::serial::FBBuilder& builder ) const {
        using ::rn::serial::serialize;
          
        auto s_cost = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().cost() )>>(
            builder, cost, ::rn::serial::ADL{} );

        // We must always serialize this table even if it is
        // empty/default-valued because, for variants, its presence
        // indicates that it is the active alternative.
        return fb::MyVariant2::Createthird( builder
            , s_cost.get()
        );
      }

      static ::rn::valid_deserial_t deserialize_table(
          fb::MyVariant2::third const& src,
          third* dst ) {
        (void)src;
        (void)dst;
        DCHECK( dst );
        using ::rn::serial::deserialize;
          
        HAS_VALUE_OR_RET( deserialize(
            ::rn::serial::detail::to_const_ptr( src.cost() ),
            &dst->cost, ::rn::serial::ADL{} ) );

        return ::rn::valid;
      }

      ::rn::valid_deserial_t check_invariants_safe() const {
        return ::rn::valid;
      }

    };

    // third
    inline void to_str( MyVariant2::third const& o, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "MyVariant2::third{{"
          "cost={}"
        "}}"
      , o.cost );
    }

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
      refl::StructField{ "name", &rdstest::MyVariant2::first::name },
      refl::StructField{ "b", &rdstest::MyVariant2::first::b },
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
      refl::StructField{ "flag1", &rdstest::MyVariant2::second::flag1 },
      refl::StructField{ "flag2", &rdstest::MyVariant2::second::flag2 },
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
      refl::StructField{ "cost", &rdstest::MyVariant2::third::cost },
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

    // a1
    inline void to_str( MyVariant3::a1 const& o, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "MyVariant3::a1{{"
          "var0={}"
        "}}"
      , o.var0 );
    }

    struct a2 {
      MyVariant0_t var1;
      MyVariant2_t var2;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct a2 const& ) const = default;
      bool operator!=( struct a2 const& ) const = default;
    };

    // a2
    inline void to_str( MyVariant3::a2 const& o, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "MyVariant3::a2{{"
          "var1={},"
          "var2={}"
        "}}"
      , o.var1, o.var2 );
    }

    struct a3 {
      char c;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct a3 const& ) const = default;
      bool operator!=( struct a3 const& ) const = default;
    };

    // a3
    inline void to_str( MyVariant3::a3 const& o, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "MyVariant3::a3{{"
          "c={}"
        "}}"
      , o.c );
    }

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
      refl::StructField{ "var0", &rdstest::inner::MyVariant3::a1::var0 },
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
      refl::StructField{ "var1", &rdstest::inner::MyVariant3::a2::var1 },
      refl::StructField{ "var2", &rdstest::inner::MyVariant3::a2::var2 },
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
      refl::StructField{ "c", &rdstest::inner::MyVariant3::a3::c },
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

    // first
    inline void to_str( MyVariant4::first const& o, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "MyVariant4::first{{"
          "i={},"
          "c={},"
          "b={},"
          "op={}"
        "}}"
      , o.i, o.c, o.b, o.op );
    }

    struct _2nd {
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct _2nd const& ) const = default;
      bool operator!=( struct _2nd const& ) const = default;
    };

    // _2nd
    inline void to_str( MyVariant4::_2nd const&, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "MyVariant4::_2nd"
       );
    }

    struct third {
      std::string  s;
      MyVariant3_t var3;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct third const& ) const = default;
      bool operator!=( struct third const& ) const = default;
    };

    // third
    inline void to_str( MyVariant4::third const& o, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "MyVariant4::third{{"
          "s={},"
          "var3={}"
        "}}"
      , o.s, o.var3 );
    }

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
      refl::StructField{ "i", &rdstest::inner::MyVariant4::first::i },
      refl::StructField{ "c", &rdstest::inner::MyVariant4::first::c },
      refl::StructField{ "b", &rdstest::inner::MyVariant4::first::b },
      refl::StructField{ "op", &rdstest::inner::MyVariant4::first::op },
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
      refl::StructField{ "s", &rdstest::inner::MyVariant4::third::s },
      refl::StructField{ "var3", &rdstest::inner::MyVariant4::third::var3 },
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

    // first_alternative
    template<typename T, typename U>
    inline void to_str( TemplateTwoParams::first_alternative<T, U> const& o, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "TemplateTwoParams::first_alternative<{}>{{"
          "t={},"
          "c={}"
        "}}"
      , ::base::type_list_to_names<T, U>(), o.t, o.c );
    }

    template<typename T, typename U>
    struct second_alternative {
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct second_alternative const& ) const = default;
      bool operator!=( struct second_alternative const& ) const = default;
    };

    // second_alternative
    template<typename T, typename U>
    inline void to_str( TemplateTwoParams::second_alternative<T, U> const&, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "TemplateTwoParams::second_alternative<{}>"
      , ::base::type_list_to_names<T, U>() );
    }

    template<typename T, typename U>
    struct third_alternative {
      Maybe_t<T> hello;
      U          u;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct third_alternative const& ) const = default;
      bool operator!=( struct third_alternative const& ) const = default;
    };

    // third_alternative
    template<typename T, typename U>
    inline void to_str( TemplateTwoParams::third_alternative<T, U> const& o, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "TemplateTwoParams::third_alternative<{}>{{"
          "hello={},"
          "u={}"
        "}}"
      , ::base::type_list_to_names<T, U>(), o.hello, o.u );
    }

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
      refl::StructField{ "t", &rdstest::inner::TemplateTwoParams::first_alternative<T, U>::t },
      refl::StructField{ "c", &rdstest::inner::TemplateTwoParams::first_alternative<T, U>::c },
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
      refl::StructField{ "hello", &rdstest::inner::TemplateTwoParams::third_alternative<T, U>::hello },
      refl::StructField{ "u", &rdstest::inner::TemplateTwoParams::third_alternative<T, U>::u },
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

    // first
    template<typename T, typename U>
    inline void to_str( CompositeTemplateTwo::first<T, U> const& o, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "CompositeTemplateTwo::first<{}>{{"
          "ttp={}"
        "}}"
      , ::base::type_list_to_names<T, U>(), o.ttp );
    }

    template<typename T, typename U>
    struct second {
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct second const& ) const = default;
      bool operator!=( struct second const& ) const = default;
    };

    // second
    template<typename T, typename U>
    inline void to_str( CompositeTemplateTwo::second<T, U> const&, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "CompositeTemplateTwo::second<{}>"
      , ::base::type_list_to_names<T, U>() );
    }

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
      refl::StructField{ "ttp", &rdstest::inner::CompositeTemplateTwo::first<T, U>::ttp },
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

namespace rn {

  inline void to_str( e_empty, std::string&, ::base::ADL_t ) {
  }

} // namespace rn

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

namespace rn {

  inline void to_str( e_single o, std::string& out, ::base::ADL_t ) {
    out += refl::traits<e_single>::value_names[static_cast<int>( o )];
  }

} // namespace rn

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

namespace rn {

  inline void to_str( e_two o, std::string& out, ::base::ADL_t ) {
    out += refl::traits<e_two>::value_names[static_cast<int>( o )];
  }

} // namespace rn

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

namespace rn {

  inline void to_str( e_color o, std::string& out, ::base::ADL_t ) {
    out += refl::traits<e_color>::value_names[static_cast<int>( o )];
  }

} // namespace rn

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

namespace rn {

  inline void to_str( e_hand o, std::string& out, ::base::ADL_t ) {
    out += refl::traits<e_hand>::value_names[static_cast<int>( o )];
  }

} // namespace rn

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
      using fb_target_t = fb::MySumtype::none;

      rn::serial::FBOffset<fb::MySumtype::none> serialize_table(
          rn::serial::FBBuilder& builder ) const {
        using ::rn::serial::serialize;
          
        // We must always serialize this table even if it is
        // empty/default-valued because, for variants, its presence
        // indicates that it is the active alternative.
        return fb::MySumtype::Createnone( builder
              
        );
      }

      static ::rn::valid_deserial_t deserialize_table(
          fb::MySumtype::none const& src,
          none* dst ) {
        (void)src;
        (void)dst;
        DCHECK( dst );
        using ::rn::serial::deserialize;
          
        return ::rn::valid;
      }

      ::rn::valid_deserial_t check_invariants_safe() const {
        return ::rn::valid;
      }

    };

    // none
    inline void to_str( MySumtype::none const&, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "MySumtype::none"
       );
    }

    struct some {
      std::string s;
      int         y;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct some const& ) const = default;
      bool operator!=( struct some const& ) const = default;
      using fb_target_t = fb::MySumtype::some;

      rn::serial::FBOffset<fb::MySumtype::some> serialize_table(
          rn::serial::FBBuilder& builder ) const {
        using ::rn::serial::serialize;
          
        auto s_s = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().s() )>>(
            builder, s, ::rn::serial::ADL{} );

        auto s_y = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().y() )>>(
            builder, y, ::rn::serial::ADL{} );

        // We must always serialize this table even if it is
        // empty/default-valued because, for variants, its presence
        // indicates that it is the active alternative.
        return fb::MySumtype::Createsome( builder
            , s_s.get(), s_y.get()
        );
      }

      static ::rn::valid_deserial_t deserialize_table(
          fb::MySumtype::some const& src,
          some* dst ) {
        (void)src;
        (void)dst;
        DCHECK( dst );
        using ::rn::serial::deserialize;
          
        HAS_VALUE_OR_RET( deserialize(
            ::rn::serial::detail::to_const_ptr( src.s() ),
            &dst->s, ::rn::serial::ADL{} ) );

        HAS_VALUE_OR_RET( deserialize(
            ::rn::serial::detail::to_const_ptr( src.y() ),
            &dst->y, ::rn::serial::ADL{} ) );

        return ::rn::valid;
      }

      ::rn::valid_deserial_t check_invariants_safe() const {
        return ::rn::valid;
      }

    };

    // some
    inline void to_str( MySumtype::some const& o, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "MySumtype::some{{"
          "s={},"
          "y={}"
        "}}"
      , o.s, o.y );
    }

    struct more {
      double d;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct more const& ) const = default;
      bool operator!=( struct more const& ) const = default;
      using fb_target_t = fb::MySumtype::more;

      rn::serial::FBOffset<fb::MySumtype::more> serialize_table(
          rn::serial::FBBuilder& builder ) const {
        using ::rn::serial::serialize;
          
        auto s_d = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().d() )>>(
            builder, d, ::rn::serial::ADL{} );

        // We must always serialize this table even if it is
        // empty/default-valued because, for variants, its presence
        // indicates that it is the active alternative.
        return fb::MySumtype::Createmore( builder
            , s_d.get()
        );
      }

      static ::rn::valid_deserial_t deserialize_table(
          fb::MySumtype::more const& src,
          more* dst ) {
        (void)src;
        (void)dst;
        DCHECK( dst );
        using ::rn::serial::deserialize;
          
        HAS_VALUE_OR_RET( deserialize(
            ::rn::serial::detail::to_const_ptr( src.d() ),
            &dst->d, ::rn::serial::ADL{} ) );

        return ::rn::valid;
      }

      ::rn::valid_deserial_t check_invariants_safe() const {
        return ::rn::valid;
      }

    };

    // more
    inline void to_str( MySumtype::more const& o, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "MySumtype::more{{"
          "d={}"
        "}}"
      , o.d );
    }

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
      refl::StructField{ "s", &rn::MySumtype::some::s },
      refl::StructField{ "y", &rn::MySumtype::some::y },
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
      refl::StructField{ "d", &rn::MySumtype::more::d },
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
      using fb_target_t = fb::OnOffState::off;

      rn::serial::FBOffset<fb::OnOffState::off> serialize_table(
          rn::serial::FBBuilder& builder ) const {
        using ::rn::serial::serialize;
          
        // We must always serialize this table even if it is
        // empty/default-valued because, for variants, its presence
        // indicates that it is the active alternative.
        return fb::OnOffState::Createoff( builder
              
        );
      }

      static ::rn::valid_deserial_t deserialize_table(
          fb::OnOffState::off const& src,
          off* dst ) {
        (void)src;
        (void)dst;
        DCHECK( dst );
        using ::rn::serial::deserialize;
          
        return ::rn::valid;
      }

      ::rn::valid_deserial_t check_invariants_safe() const {
        return ::rn::valid;
      }

    };

    // off
    inline void to_str( OnOffState::off const&, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "OnOffState::off"
       );
    }

    struct on {
      std::string user;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct on const& ) const = default;
      bool operator!=( struct on const& ) const = default;
      using fb_target_t = fb::OnOffState::on;

      rn::serial::FBOffset<fb::OnOffState::on> serialize_table(
          rn::serial::FBBuilder& builder ) const {
        using ::rn::serial::serialize;
          
        auto s_user = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().user() )>>(
            builder, user, ::rn::serial::ADL{} );

        // We must always serialize this table even if it is
        // empty/default-valued because, for variants, its presence
        // indicates that it is the active alternative.
        return fb::OnOffState::Createon( builder
            , s_user.get()
        );
      }

      static ::rn::valid_deserial_t deserialize_table(
          fb::OnOffState::on const& src,
          on* dst ) {
        (void)src;
        (void)dst;
        DCHECK( dst );
        using ::rn::serial::deserialize;
          
        HAS_VALUE_OR_RET( deserialize(
            ::rn::serial::detail::to_const_ptr( src.user() ),
            &dst->user, ::rn::serial::ADL{} ) );

        return ::rn::valid;
      }

      ::rn::valid_deserial_t check_invariants_safe() const {
        return ::rn::valid;
      }

    };

    // on
    inline void to_str( OnOffState::on const& o, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "OnOffState::on{{"
          "user={}"
        "}}"
      , o.user );
    }

    struct switching_on {
      double percent;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct switching_on const& ) const = default;
      bool operator!=( struct switching_on const& ) const = default;
      using fb_target_t = fb::OnOffState::switching_on;

      rn::serial::FBOffset<fb::OnOffState::switching_on> serialize_table(
          rn::serial::FBBuilder& builder ) const {
        using ::rn::serial::serialize;
          
        auto s_percent = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().percent() )>>(
            builder, percent, ::rn::serial::ADL{} );

        // We must always serialize this table even if it is
        // empty/default-valued because, for variants, its presence
        // indicates that it is the active alternative.
        return fb::OnOffState::Createswitching_on( builder
            , s_percent.get()
        );
      }

      static ::rn::valid_deserial_t deserialize_table(
          fb::OnOffState::switching_on const& src,
          switching_on* dst ) {
        (void)src;
        (void)dst;
        DCHECK( dst );
        using ::rn::serial::deserialize;
          
        HAS_VALUE_OR_RET( deserialize(
            ::rn::serial::detail::to_const_ptr( src.percent() ),
            &dst->percent, ::rn::serial::ADL{} ) );

        return ::rn::valid;
      }

      ::rn::valid_deserial_t check_invariants_safe() const {
        return ::rn::valid;
      }

    };

    // switching_on
    inline void to_str( OnOffState::switching_on const& o, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "OnOffState::switching_on{{"
          "percent={}"
        "}}"
      , o.percent );
    }

    struct switching_off {
      double percent;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct switching_off const& ) const = default;
      bool operator!=( struct switching_off const& ) const = default;
      using fb_target_t = fb::OnOffState::switching_off;

      rn::serial::FBOffset<fb::OnOffState::switching_off> serialize_table(
          rn::serial::FBBuilder& builder ) const {
        using ::rn::serial::serialize;
          
        auto s_percent = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().percent() )>>(
            builder, percent, ::rn::serial::ADL{} );

        // We must always serialize this table even if it is
        // empty/default-valued because, for variants, its presence
        // indicates that it is the active alternative.
        return fb::OnOffState::Createswitching_off( builder
            , s_percent.get()
        );
      }

      static ::rn::valid_deserial_t deserialize_table(
          fb::OnOffState::switching_off const& src,
          switching_off* dst ) {
        (void)src;
        (void)dst;
        DCHECK( dst );
        using ::rn::serial::deserialize;
          
        HAS_VALUE_OR_RET( deserialize(
            ::rn::serial::detail::to_const_ptr( src.percent() ),
            &dst->percent, ::rn::serial::ADL{} ) );

        return ::rn::valid;
      }

      ::rn::valid_deserial_t check_invariants_safe() const {
        return ::rn::valid;
      }

    };

    // switching_off
    inline void to_str( OnOffState::switching_off const& o, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "OnOffState::switching_off{{"
          "percent={}"
        "}}"
      , o.percent );
    }

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
      refl::StructField{ "user", &rn::OnOffState::on::user },
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
      refl::StructField{ "percent", &rn::OnOffState::switching_on::percent },
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
      refl::StructField{ "percent", &rn::OnOffState::switching_off::percent },
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

    // turn_off
    inline void to_str( OnOffEvent::turn_off const&, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "OnOffEvent::turn_off"
       );
    }

    struct turn_on {
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct turn_on const& ) const = default;
      bool operator!=( struct turn_on const& ) const = default;
    };

    // turn_on
    inline void to_str( OnOffEvent::turn_on const&, std::string& out, ::base::ADL_t ) {
      out += fmt::format(
        "OnOffEvent::turn_on"
       );
    }

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
      refl::StructField{ "xxx", &rn::MyStruct::xxx },
      refl::StructField{ "yyy", &rn::MyStruct::yyy },
      refl::StructField{ "zzz_map", &rn::MyStruct::zzz_map },
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

    // Validates invariants among members.  Must be manually
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
      refl::StructField{ "xxx", &rn::StructWithValidation::xxx },
      refl::StructField{ "yyy", &rn::StructWithValidation::yyy },
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
      refl::StructField{ "xxx", &rn::test::MyTemplateStruct<T, U>::xxx },
      refl::StructField{ "yyy", &rn::test::MyTemplateStruct<T, U>::yyy },
      refl::StructField{ "zzz_map", &rn::test::MyTemplateStruct<T, U>::zzz_map },
    };
  };

} // namespace refl
