// Auto-Generated file, do not modify! (testing.rds).
#pragma once

/****************************************************************
*                           Includes
*****************************************************************/
// Includes specified in rds file.
#include "maybe.hpp"
#include "cdr/ext-builtin.hpp"
#include <string>
#include <vector>
#include <unordered_map>

// Revolution Now
#include "core-config.hpp"

// refl
#include "refl/ext.hpp"

// base
#include "base/variant.hpp"
// Rds helpers.
#include "rds/config-helper.hpp"


// C++ standard library
#include <array>
#include <string_view>
#include <tuple>


/****************************************************************
*                        Sum Type: Maybe
*****************************************************************/
namespace rdstest {

  namespace detail {

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
        T val = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct just const& ) const = default;
        bool operator!=( struct just const& ) const = default;
      };

    } // namespace Maybe

    template<typename T>
    using MaybeBase = base::variant<
      detail::Maybe::nothing<T>,
      detail::Maybe::just<T>
    >;

  } // namespace detail

  template<typename T>
  struct Maybe : public detail::MaybeBase<T> {
    using nothing = detail::Maybe::nothing<T>;
    using just    = detail::Maybe::just<T>;

    enum class e {
      nothing,
      just,
    };

    using i_am_rds_variant = void;
    using Base = detail::MaybeBase<T>;
    using Base::Base;
    Maybe( Base&& b ) : Base( std::move( b ) ) {}
    Base const& as_base() const& { return *this; }
    Base&       as_base()      & { return *this; }
  };

  // TODO: temporary.
  template<typename T>
  using Maybe_t = Maybe<T>;

  NOTHROW_MOVE( Maybe<int> );

} // namespace rdstest

// This gives us the enum to use in a switch statement.
template<typename T>
struct base::variant_to_enum<rdstest::detail::MaybeBase<T>> {
  using type = rdstest::Maybe<T>::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct nothing.
  template<typename T>
  struct traits<rdstest::detail::Maybe::nothing<T>> {
    using type = rdstest::detail::Maybe::nothing<T>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::Maybe";
    static constexpr std::string_view name = "nothing";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<T>;

    static constexpr std::tuple fields{};
  };

  // Reflection info for struct just.
  template<typename T>
  struct traits<rdstest::detail::Maybe::just<T>> {
    using type = rdstest::detail::Maybe::just<T>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::Maybe";
    static constexpr std::string_view name = "just";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<T>;

    static constexpr std::tuple fields{
      refl::StructField{ "val", &rdstest::detail::Maybe::just<T>::val, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                     Sum Type: MyVariant1
*****************************************************************/
namespace rdstest {

  namespace detail {

    namespace MyVariant1 {

      struct happy {
        std::pair<char, int> p = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct happy const& ) const = default;
        bool operator!=( struct happy const& ) const = default;
      };

      struct sad {
        bool  hello = {};
        bool* ptr = {};
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

    } // namespace MyVariant1

    using MyVariant1Base = base::variant<
      detail::MyVariant1::happy,
      detail::MyVariant1::sad,
      detail::MyVariant1::excited
    >;

  } // namespace detail

  struct MyVariant1 : public detail::MyVariant1Base {
    using happy   = detail::MyVariant1::happy;
    using sad     = detail::MyVariant1::sad;
    using excited = detail::MyVariant1::excited;

    enum class e {
      happy,
      sad,
      excited,
    };

    using i_am_rds_variant = void;
    using Base = detail::MyVariant1Base;
    using Base::Base;
    MyVariant1( Base&& b ) : Base( std::move( b ) ) {}
    Base const& as_base() const& { return *this; }
    Base&       as_base()      & { return *this; }
  };

  // TODO: temporary.
  using MyVariant1_t = MyVariant1;

  NOTHROW_MOVE( MyVariant1 );

} // namespace rdstest

// This gives us the enum to use in a switch statement.
template<>
struct base::variant_to_enum<rdstest::detail::MyVariant1Base> {
  using type = rdstest::MyVariant1::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct happy.
  template<>
  struct traits<rdstest::detail::MyVariant1::happy> {
    using type = rdstest::detail::MyVariant1::happy;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::MyVariant1";
    static constexpr std::string_view name = "happy";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "p", &rdstest::detail::MyVariant1::happy::p, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct sad.
  template<>
  struct traits<rdstest::detail::MyVariant1::sad> {
    using type = rdstest::detail::MyVariant1::sad;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::MyVariant1";
    static constexpr std::string_view name = "sad";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "hello", &rdstest::detail::MyVariant1::sad::hello, /*offset=*/base::nothing },
      refl::StructField{ "ptr", &rdstest::detail::MyVariant1::sad::ptr, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct excited.
  template<>
  struct traits<rdstest::detail::MyVariant1::excited> {
    using type = rdstest::detail::MyVariant1::excited;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::MyVariant1";
    static constexpr std::string_view name = "excited";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{};
  };

} // namespace refl

/****************************************************************
*                     Sum Type: MyVariant2
*****************************************************************/
namespace rdstest {

  namespace detail {

    namespace MyVariant2 {

      struct first {
        std::string name = {};
        bool        b = {};
      };

      struct second {
        bool flag1 = {};
        bool flag2 = {};
      };

      struct third {
        int cost = {};
      };

    } // namespace MyVariant2

    using MyVariant2Base = base::variant<
      detail::MyVariant2::first,
      detail::MyVariant2::second,
      detail::MyVariant2::third
    >;

  } // namespace detail

  struct MyVariant2 : public detail::MyVariant2Base {
    using first  = detail::MyVariant2::first;
    using second = detail::MyVariant2::second;
    using third  = detail::MyVariant2::third;

    enum class e {
      first,
      second,
      third,
    };

    using i_am_rds_variant = void;
    using Base = detail::MyVariant2Base;
    using Base::Base;
    MyVariant2( Base&& b ) : Base( std::move( b ) ) {}
    Base const& as_base() const& { return *this; }
    Base&       as_base()      & { return *this; }
  };

  // TODO: temporary.
  using MyVariant2_t = MyVariant2;

  NOTHROW_MOVE( MyVariant2 );

} // namespace rdstest

// This gives us the enum to use in a switch statement.
template<>
struct base::variant_to_enum<rdstest::detail::MyVariant2Base> {
  using type = rdstest::MyVariant2::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct first.
  template<>
  struct traits<rdstest::detail::MyVariant2::first> {
    using type = rdstest::detail::MyVariant2::first;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::MyVariant2";
    static constexpr std::string_view name = "first";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "name", &rdstest::detail::MyVariant2::first::name, /*offset=*/base::nothing },
      refl::StructField{ "b", &rdstest::detail::MyVariant2::first::b, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct second.
  template<>
  struct traits<rdstest::detail::MyVariant2::second> {
    using type = rdstest::detail::MyVariant2::second;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::MyVariant2";
    static constexpr std::string_view name = "second";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "flag1", &rdstest::detail::MyVariant2::second::flag1, /*offset=*/base::nothing },
      refl::StructField{ "flag2", &rdstest::detail::MyVariant2::second::flag2, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct third.
  template<>
  struct traits<rdstest::detail::MyVariant2::third> {
    using type = rdstest::detail::MyVariant2::third;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::MyVariant2";
    static constexpr std::string_view name = "third";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "cost", &rdstest::detail::MyVariant2::third::cost, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                     Sum Type: MyVariant3
*****************************************************************/
namespace rdstest::inner {

  namespace detail {

    namespace MyVariant3 {

      struct a1 {
        std::monostate var0 = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct a1 const& ) const = default;
        bool operator!=( struct a1 const& ) const = default;
      };

      struct a2 {
        std::monostate var1 = {};
        MyVariant2_t   var2 = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct a2 const& ) const = default;
        bool operator!=( struct a2 const& ) const = default;
      };

      struct a3 {
        char c = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct a3 const& ) const = default;
        bool operator!=( struct a3 const& ) const = default;
      };

    } // namespace MyVariant3

    using MyVariant3Base = base::variant<
      detail::MyVariant3::a1,
      detail::MyVariant3::a2,
      detail::MyVariant3::a3
    >;

  } // namespace detail

  struct MyVariant3 : public detail::MyVariant3Base {
    using a1 = detail::MyVariant3::a1;
    using a2 = detail::MyVariant3::a2;
    using a3 = detail::MyVariant3::a3;

    enum class e {
      a1,
      a2,
      a3,
    };

    using i_am_rds_variant = void;
    using Base = detail::MyVariant3Base;
    using Base::Base;
    MyVariant3( Base&& b ) : Base( std::move( b ) ) {}
    Base const& as_base() const& { return *this; }
    Base&       as_base()      & { return *this; }
  };

  // TODO: temporary.
  using MyVariant3_t = MyVariant3;

  NOTHROW_MOVE( MyVariant3 );

} // namespace rdstest::inner

// This gives us the enum to use in a switch statement.
template<>
struct base::variant_to_enum<rdstest::inner::detail::MyVariant3Base> {
  using type = rdstest::inner::MyVariant3::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct a1.
  template<>
  struct traits<rdstest::inner::detail::MyVariant3::a1> {
    using type = rdstest::inner::detail::MyVariant3::a1;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::MyVariant3";
    static constexpr std::string_view name = "a1";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "var0", &rdstest::inner::detail::MyVariant3::a1::var0, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct a2.
  template<>
  struct traits<rdstest::inner::detail::MyVariant3::a2> {
    using type = rdstest::inner::detail::MyVariant3::a2;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::MyVariant3";
    static constexpr std::string_view name = "a2";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "var1", &rdstest::inner::detail::MyVariant3::a2::var1, /*offset=*/base::nothing },
      refl::StructField{ "var2", &rdstest::inner::detail::MyVariant3::a2::var2, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct a3.
  template<>
  struct traits<rdstest::inner::detail::MyVariant3::a3> {
    using type = rdstest::inner::detail::MyVariant3::a3;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::MyVariant3";
    static constexpr std::string_view name = "a3";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "c", &rdstest::inner::detail::MyVariant3::a3::c, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                     Sum Type: MyVariant4
*****************************************************************/
namespace rdstest::inner {

  namespace detail {

    namespace MyVariant4 {

      struct first {
        int                 i = {};
        char                c = {};
        bool                b = {};
        rn::maybe<uint32_t> op = {};
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
        std::string  s = {};
        MyVariant3_t var3 = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct third const& ) const = default;
        bool operator!=( struct third const& ) const = default;
      };

    } // namespace MyVariant4

    using MyVariant4Base = base::variant<
      detail::MyVariant4::first,
      detail::MyVariant4::_2nd,
      detail::MyVariant4::third
    >;

  } // namespace detail

  struct MyVariant4 : public detail::MyVariant4Base {
    using first = detail::MyVariant4::first;
    using _2nd  = detail::MyVariant4::_2nd;
    using third = detail::MyVariant4::third;

    enum class e {
      first,
      _2nd,
      third,
    };

    using i_am_rds_variant = void;
    using Base = detail::MyVariant4Base;
    using Base::Base;
    MyVariant4( Base&& b ) : Base( std::move( b ) ) {}
    Base const& as_base() const& { return *this; }
    Base&       as_base()      & { return *this; }
  };

  // TODO: temporary.
  using MyVariant4_t = MyVariant4;

  NOTHROW_MOVE( MyVariant4 );

} // namespace rdstest::inner

// This gives us the enum to use in a switch statement.
template<>
struct base::variant_to_enum<rdstest::inner::detail::MyVariant4Base> {
  using type = rdstest::inner::MyVariant4::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct first.
  template<>
  struct traits<rdstest::inner::detail::MyVariant4::first> {
    using type = rdstest::inner::detail::MyVariant4::first;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::MyVariant4";
    static constexpr std::string_view name = "first";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "i", &rdstest::inner::detail::MyVariant4::first::i, /*offset=*/base::nothing },
      refl::StructField{ "c", &rdstest::inner::detail::MyVariant4::first::c, /*offset=*/base::nothing },
      refl::StructField{ "b", &rdstest::inner::detail::MyVariant4::first::b, /*offset=*/base::nothing },
      refl::StructField{ "op", &rdstest::inner::detail::MyVariant4::first::op, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct _2nd.
  template<>
  struct traits<rdstest::inner::detail::MyVariant4::_2nd> {
    using type = rdstest::inner::detail::MyVariant4::_2nd;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::MyVariant4";
    static constexpr std::string_view name = "_2nd";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{};
  };

  // Reflection info for struct third.
  template<>
  struct traits<rdstest::inner::detail::MyVariant4::third> {
    using type = rdstest::inner::detail::MyVariant4::third;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::MyVariant4";
    static constexpr std::string_view name = "third";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "s", &rdstest::inner::detail::MyVariant4::third::s, /*offset=*/base::nothing },
      refl::StructField{ "var3", &rdstest::inner::detail::MyVariant4::third::var3, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                  Sum Type: TemplateTwoParams
*****************************************************************/
namespace rdstest::inner {

  namespace detail {

    namespace TemplateTwoParams {

      template<typename T, typename U>
      struct first_alternative {
        T    t = {};
        char c = {};
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
        Maybe_t<T> hello = {};
        U          u = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct third_alternative const& ) const = default;
        bool operator!=( struct third_alternative const& ) const = default;
      };

    } // namespace TemplateTwoParams

    template<typename T, typename U>
    using TemplateTwoParamsBase = base::variant<
      detail::TemplateTwoParams::first_alternative<T, U>,
      detail::TemplateTwoParams::second_alternative<T, U>,
      detail::TemplateTwoParams::third_alternative<T, U>
    >;

  } // namespace detail

  template<typename T, typename U>
  struct TemplateTwoParams : public detail::TemplateTwoParamsBase<T, U> {
    using first_alternative  = detail::TemplateTwoParams::first_alternative<T, U>;
    using second_alternative = detail::TemplateTwoParams::second_alternative<T, U>;
    using third_alternative  = detail::TemplateTwoParams::third_alternative<T, U>;

    enum class e {
      first_alternative,
      second_alternative,
      third_alternative,
    };

    using i_am_rds_variant = void;
    using Base = detail::TemplateTwoParamsBase<T, U>;
    using Base::Base;
    TemplateTwoParams( Base&& b ) : Base( std::move( b ) ) {}
    Base const& as_base() const& { return *this; }
    Base&       as_base()      & { return *this; }
  };

  // TODO: temporary.
  template<typename T, typename U>
  using TemplateTwoParams_t = TemplateTwoParams<T, U>;

  NOTHROW_MOVE( TemplateTwoParams<int, int> );

} // namespace rdstest::inner

// This gives us the enum to use in a switch statement.
template<typename T, typename U>
struct base::variant_to_enum<rdstest::inner::detail::TemplateTwoParamsBase<T, U>> {
  using type = rdstest::inner::TemplateTwoParams<T, U>::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct first_alternative.
  template<typename T, typename U>
  struct traits<rdstest::inner::detail::TemplateTwoParams::first_alternative<T, U>> {
    using type = rdstest::inner::detail::TemplateTwoParams::first_alternative<T, U>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::TemplateTwoParams";
    static constexpr std::string_view name = "first_alternative";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<T, U>;

    static constexpr std::tuple fields{
      refl::StructField{ "t", &rdstest::inner::detail::TemplateTwoParams::first_alternative<T, U>::t, /*offset=*/base::nothing },
      refl::StructField{ "c", &rdstest::inner::detail::TemplateTwoParams::first_alternative<T, U>::c, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct second_alternative.
  template<typename T, typename U>
  struct traits<rdstest::inner::detail::TemplateTwoParams::second_alternative<T, U>> {
    using type = rdstest::inner::detail::TemplateTwoParams::second_alternative<T, U>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::TemplateTwoParams";
    static constexpr std::string_view name = "second_alternative";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<T, U>;

    static constexpr std::tuple fields{};
  };

  // Reflection info for struct third_alternative.
  template<typename T, typename U>
  struct traits<rdstest::inner::detail::TemplateTwoParams::third_alternative<T, U>> {
    using type = rdstest::inner::detail::TemplateTwoParams::third_alternative<T, U>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::TemplateTwoParams";
    static constexpr std::string_view name = "third_alternative";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<T, U>;

    static constexpr std::tuple fields{
      refl::StructField{ "hello", &rdstest::inner::detail::TemplateTwoParams::third_alternative<T, U>::hello, /*offset=*/base::nothing },
      refl::StructField{ "u", &rdstest::inner::detail::TemplateTwoParams::third_alternative<T, U>::u, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                Sum Type: CompositeTemplateTwo
*****************************************************************/
namespace rdstest::inner {

  namespace detail {

    namespace CompositeTemplateTwo {

      template<typename T, typename U>
      struct first {
        rdstest::inner::TemplateTwoParams_t<T,U> ttp = {};
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

    } // namespace CompositeTemplateTwo

    template<typename T, typename U>
    using CompositeTemplateTwoBase = base::variant<
      detail::CompositeTemplateTwo::first<T, U>,
      detail::CompositeTemplateTwo::second<T, U>
    >;

  } // namespace detail

  template<typename T, typename U>
  struct CompositeTemplateTwo : public detail::CompositeTemplateTwoBase<T, U> {
    using first  = detail::CompositeTemplateTwo::first<T, U>;
    using second = detail::CompositeTemplateTwo::second<T, U>;

    enum class e {
      first,
      second,
    };

    using i_am_rds_variant = void;
    using Base = detail::CompositeTemplateTwoBase<T, U>;
    using Base::Base;
    CompositeTemplateTwo( Base&& b ) : Base( std::move( b ) ) {}
    Base const& as_base() const& { return *this; }
    Base&       as_base()      & { return *this; }
  };

  // TODO: temporary.
  template<typename T, typename U>
  using CompositeTemplateTwo_t = CompositeTemplateTwo<T, U>;

  NOTHROW_MOVE( CompositeTemplateTwo<int, int> );

} // namespace rdstest::inner

// This gives us the enum to use in a switch statement.
template<typename T, typename U>
struct base::variant_to_enum<rdstest::inner::detail::CompositeTemplateTwoBase<T, U>> {
  using type = rdstest::inner::CompositeTemplateTwo<T, U>::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct first.
  template<typename T, typename U>
  struct traits<rdstest::inner::detail::CompositeTemplateTwo::first<T, U>> {
    using type = rdstest::inner::detail::CompositeTemplateTwo::first<T, U>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::CompositeTemplateTwo";
    static constexpr std::string_view name = "first";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<T, U>;

    static constexpr std::tuple fields{
      refl::StructField{ "ttp", &rdstest::inner::detail::CompositeTemplateTwo::first<T, U>::ttp, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct second.
  template<typename T, typename U>
  struct traits<rdstest::inner::detail::CompositeTemplateTwo::second<T, U>> {
    using type = rdstest::inner::detail::CompositeTemplateTwo::second<T, U>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::CompositeTemplateTwo";
    static constexpr std::string_view name = "second";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<T, U>;

    static constexpr std::tuple fields{};
  };

} // namespace refl

/****************************************************************
*                      Sum Type: MySumtype
*****************************************************************/
namespace rn {

  namespace detail {

    namespace MySumtype {

      struct none {
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct none const& ) const = default;
        bool operator!=( struct none const& ) const = default;
      };

      struct some {
        std::string s = {};
        int         y = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct some const& ) const = default;
        bool operator!=( struct some const& ) const = default;
      };

      struct more {
        double d = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct more const& ) const = default;
        bool operator!=( struct more const& ) const = default;
      };

    } // namespace MySumtype

    using MySumtypeBase = base::variant<
      detail::MySumtype::none,
      detail::MySumtype::some,
      detail::MySumtype::more
    >;

  } // namespace detail

  struct MySumtype : public detail::MySumtypeBase {
    using none = detail::MySumtype::none;
    using some = detail::MySumtype::some;
    using more = detail::MySumtype::more;

    enum class e {
      none,
      some,
      more,
    };

    using i_am_rds_variant = void;
    using Base = detail::MySumtypeBase;
    using Base::Base;
    MySumtype( Base&& b ) : Base( std::move( b ) ) {}
    Base const& as_base() const& { return *this; }
    Base&       as_base()      & { return *this; }
  };

  // TODO: temporary.
  using MySumtype_t = MySumtype;

  NOTHROW_MOVE( MySumtype );

} // namespace rn

// This gives us the enum to use in a switch statement.
template<>
struct base::variant_to_enum<rn::detail::MySumtypeBase> {
  using type = rn::MySumtype::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct none.
  template<>
  struct traits<rn::detail::MySumtype::none> {
    using type = rn::detail::MySumtype::none;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::MySumtype";
    static constexpr std::string_view name = "none";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{};
  };

  // Reflection info for struct some.
  template<>
  struct traits<rn::detail::MySumtype::some> {
    using type = rn::detail::MySumtype::some;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::MySumtype";
    static constexpr std::string_view name = "some";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "s", &rn::detail::MySumtype::some::s, /*offset=*/base::nothing },
      refl::StructField{ "y", &rn::detail::MySumtype::some::y, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct more.
  template<>
  struct traits<rn::detail::MySumtype::more> {
    using type = rn::detail::MySumtype::more;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::MySumtype";
    static constexpr std::string_view name = "more";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "d", &rn::detail::MySumtype::more::d, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                     Sum Type: OnOffState
*****************************************************************/
namespace rn {

  namespace detail {

    namespace OnOffState {

      struct off {
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct off const& ) const = default;
        bool operator!=( struct off const& ) const = default;
      };

      struct on {
        std::string user = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct on const& ) const = default;
        bool operator!=( struct on const& ) const = default;
      };

      struct switching_on {
        double percent = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct switching_on const& ) const = default;
        bool operator!=( struct switching_on const& ) const = default;
      };

      struct switching_off {
        double percent = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct switching_off const& ) const = default;
        bool operator!=( struct switching_off const& ) const = default;
      };

    } // namespace OnOffState

    using OnOffStateBase = base::variant<
      detail::OnOffState::off,
      detail::OnOffState::on,
      detail::OnOffState::switching_on,
      detail::OnOffState::switching_off
    >;

  } // namespace detail

  struct OnOffState : public detail::OnOffStateBase {
    using off           = detail::OnOffState::off;
    using on            = detail::OnOffState::on;
    using switching_on  = detail::OnOffState::switching_on;
    using switching_off = detail::OnOffState::switching_off;

    enum class e {
      off,
      on,
      switching_on,
      switching_off,
    };

    using i_am_rds_variant = void;
    using Base = detail::OnOffStateBase;
    using Base::Base;
    OnOffState( Base&& b ) : Base( std::move( b ) ) {}
    Base const& as_base() const& { return *this; }
    Base&       as_base()      & { return *this; }
  };

  // TODO: temporary.
  using OnOffState_t = OnOffState;

  NOTHROW_MOVE( OnOffState );

} // namespace rn

// This gives us the enum to use in a switch statement.
template<>
struct base::variant_to_enum<rn::detail::OnOffStateBase> {
  using type = rn::OnOffState::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct off.
  template<>
  struct traits<rn::detail::OnOffState::off> {
    using type = rn::detail::OnOffState::off;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::OnOffState";
    static constexpr std::string_view name = "off";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{};
  };

  // Reflection info for struct on.
  template<>
  struct traits<rn::detail::OnOffState::on> {
    using type = rn::detail::OnOffState::on;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::OnOffState";
    static constexpr std::string_view name = "on";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "user", &rn::detail::OnOffState::on::user, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct switching_on.
  template<>
  struct traits<rn::detail::OnOffState::switching_on> {
    using type = rn::detail::OnOffState::switching_on;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::OnOffState";
    static constexpr std::string_view name = "switching_on";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "percent", &rn::detail::OnOffState::switching_on::percent, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct switching_off.
  template<>
  struct traits<rn::detail::OnOffState::switching_off> {
    using type = rn::detail::OnOffState::switching_off;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::OnOffState";
    static constexpr std::string_view name = "switching_off";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "percent", &rn::detail::OnOffState::switching_off::percent, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                     Sum Type: OnOffEvent
*****************************************************************/
namespace rn {

  namespace detail {

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

    } // namespace OnOffEvent

    using OnOffEventBase = base::variant<
      detail::OnOffEvent::turn_off,
      detail::OnOffEvent::turn_on
    >;

  } // namespace detail

  struct OnOffEvent : public detail::OnOffEventBase {
    using turn_off = detail::OnOffEvent::turn_off;
    using turn_on  = detail::OnOffEvent::turn_on;

    enum class e {
      turn_off,
      turn_on,
    };

    using i_am_rds_variant = void;
    using Base = detail::OnOffEventBase;
    using Base::Base;
    OnOffEvent( Base&& b ) : Base( std::move( b ) ) {}
    Base const& as_base() const& { return *this; }
    Base&       as_base()      & { return *this; }
  };

  // TODO: temporary.
  using OnOffEvent_t = OnOffEvent;

  NOTHROW_MOVE( OnOffEvent );

} // namespace rn

// This gives us the enum to use in a switch statement.
template<>
struct base::variant_to_enum<rn::detail::OnOffEventBase> {
  using type = rn::OnOffEvent::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct turn_off.
  template<>
  struct traits<rn::detail::OnOffEvent::turn_off> {
    using type = rn::detail::OnOffEvent::turn_off;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::OnOffEvent";
    static constexpr std::string_view name = "turn_off";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{};
  };

  // Reflection info for struct turn_on.
  template<>
  struct traits<rn::detail::OnOffEvent::turn_on> {
    using type = rn::detail::OnOffEvent::turn_on;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::OnOffEvent";
    static constexpr std::string_view name = "turn_on";
    static constexpr bool is_sumtype_alternative = true;

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
*                      Enum: e_count_short
*****************************************************************/
namespace rn {

  enum class e_count_short {
    one,
    two,
    three
  };

} // namespace rn

namespace refl {

  // Reflection info for enum e_count_short.
  template<>
  struct traits<rn::e_count_short> {
    using type = rn::e_count_short;

    static constexpr type_kind kind        = type_kind::enum_kind;
    static constexpr std::string_view ns   = "rn";
    static constexpr std::string_view name = "e_count_short";

    static constexpr std::array<std::string_view, 3> value_names{
      "one",
      "two",
      "three",
    };
  };

} // namespace refl

/****************************************************************
*                         Enum: e_count
*****************************************************************/
namespace rn {

  enum class [[nodiscard]] e_count {
    one,
    two,
    three,
    four,
    five,
    six,
    seven,
    eight,
    nine,
    ten,
    eleven,
    twelve,
    thirteen,
    fourteen,
    fifteen
  };

} // namespace rn

namespace refl {

  // Reflection info for enum e_count.
  template<>
  struct traits<rn::e_count> {
    using type = rn::e_count;

    static constexpr type_kind kind        = type_kind::enum_kind;
    static constexpr std::string_view ns   = "rn";
    static constexpr std::string_view name = "e_count";

    static constexpr std::array<std::string_view, 15> value_names{
      "one",
      "two",
      "three",
      "four",
      "five",
      "six",
      "seven",
      "eight",
      "nine",
      "ten",
      "eleven",
      "twelve",
      "thirteen",
      "fourteen",
      "fifteen",
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
    static constexpr bool is_sumtype_alternative = false;

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
    static constexpr bool is_sumtype_alternative = false;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{};
  };

} // namespace refl

/****************************************************************
*                       Struct: MyStruct
*****************************************************************/
namespace rn {

  struct [[nodiscard]] MyStruct {
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
    static constexpr bool is_sumtype_alternative = false;

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
    static constexpr bool is_sumtype_alternative = false;

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
    static constexpr bool is_sumtype_alternative = false;

    using template_types = std::tuple<T, U>;

    static constexpr std::tuple fields{
      refl::StructField{ "xxx", &rn::test::MyTemplateStruct<T, U>::xxx, /*offset=*/base::nothing },
      refl::StructField{ "yyy", &rn::test::MyTemplateStruct<T, U>::yyy, /*offset=*/base::nothing },
      refl::StructField{ "zzz_map", &rn::test::MyTemplateStruct<T, U>::zzz_map, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                   Struct: config_testing_t
*****************************************************************/
namespace rn::test {

  struct config_testing_t {
    int some_field = {};

    bool operator==( config_testing_t const& ) const = default;
  };

} // namespace rn::test

namespace refl {

  // Reflection info for struct config_testing_t.
  template<>
  struct traits<rn::test::config_testing_t> {
    using type = rn::test::config_testing_t;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::test";
    static constexpr std::string_view name = "config_testing_t";
    static constexpr bool is_sumtype_alternative = false;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "some_field", &rn::test::config_testing_t::some_field, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                        Config: testing
*****************************************************************/
namespace rn::test {

  namespace detail {

    inline config_testing_t __config_testing = {};

  } // namespace detail

  inline config_testing_t const& config_testing = detail::__config_testing;

} // namespace rn::test

namespace rds {

  detail::empty_registrar register_config( rn::test::config_testing_t* global );

  namespace detail {

    //  This ensures that if anyone includes the header for this
    //  config file then it is guaranteed to be registered and
    //  populated.
    inline auto __config_testing_registration = register_config( &rn::test::detail::__config_testing );

  } // namespace detail

} // namespace rds
