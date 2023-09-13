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

    namespace Maybe_alternatives {

      template<typename T>
      struct nothing {
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct nothing const& ) const = default;
      };

      template<typename T>
      struct just {
        T val = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct just const& ) const = default;
      };

    } // namespace Maybe_alternatives

    template<typename T>
    using MaybeBase = base::variant<
      detail::Maybe_alternatives::nothing<T>,
      detail::Maybe_alternatives::just<T>
    >;

  } // namespace detail

  template<typename T>
  struct Maybe : public detail::MaybeBase<T> {
    using nothing = detail::Maybe_alternatives::nothing<T>;
    using just    = detail::Maybe_alternatives::just<T>;

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

    // Comparison with alternatives.
    bool operator==( nothing const& rhs ) const {
      return this->template holds<nothing>() && (this->template get<nothing>() == rhs);
    }
    bool operator==( just const& rhs ) const {
      return this->template holds<just>() && (this->template get<just>() == rhs);
    }
  };


} // namespace rdstest

// This gives us the enum to use in a switch statement.
template<typename T>
struct base::variant_to_enum<rdstest::detail::MaybeBase<T>> {
  using type = typename rdstest::Maybe<T>::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct nothing.
  template<typename T>
  struct traits<rdstest::detail::Maybe_alternatives::nothing<T>> {
    using type = rdstest::detail::Maybe_alternatives::nothing<T>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::Maybe";
    static constexpr std::string_view name = "nothing";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<T>;

    static constexpr std::tuple fields{};
  };

  // Reflection info for struct just.
  template<typename T>
  struct traits<rdstest::detail::Maybe_alternatives::just<T>> {
    using type = rdstest::detail::Maybe_alternatives::just<T>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::Maybe";
    static constexpr std::string_view name = "just";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<T>;

    static constexpr std::tuple fields{
      refl::StructField{ "val", &rdstest::detail::Maybe_alternatives::just<T>::val, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                     Sum Type: MyVariant1
*****************************************************************/
namespace rdstest {

  namespace detail {

    namespace MyVariant1_alternatives {

      struct happy {
        std::pair<char, int> p = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct happy const& ) const = default;
      };

      struct sad {
        bool  hello = {};
        bool* ptr = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct sad const& ) const = default;
      };

      struct excited {
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct excited const& ) const = default;
      };

    } // namespace MyVariant1_alternatives

    using MyVariant1Base = base::variant<
      detail::MyVariant1_alternatives::happy,
      detail::MyVariant1_alternatives::sad,
      detail::MyVariant1_alternatives::excited
    >;

  } // namespace detail

  struct MyVariant1 : public detail::MyVariant1Base {
    using happy   = detail::MyVariant1_alternatives::happy;
    using sad     = detail::MyVariant1_alternatives::sad;
    using excited = detail::MyVariant1_alternatives::excited;

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

    // Comparison with alternatives.
    bool operator==( happy const& rhs ) const {
      return this->template holds<happy>() && (this->template get<happy>() == rhs);
    }
    bool operator==( sad const& rhs ) const {
      return this->template holds<sad>() && (this->template get<sad>() == rhs);
    }
    bool operator==( excited const& rhs ) const {
      return this->template holds<excited>() && (this->template get<excited>() == rhs);
    }
  };


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
  struct traits<rdstest::detail::MyVariant1_alternatives::happy> {
    using type = rdstest::detail::MyVariant1_alternatives::happy;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::MyVariant1";
    static constexpr std::string_view name = "happy";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "p", &rdstest::detail::MyVariant1_alternatives::happy::p, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct sad.
  template<>
  struct traits<rdstest::detail::MyVariant1_alternatives::sad> {
    using type = rdstest::detail::MyVariant1_alternatives::sad;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::MyVariant1";
    static constexpr std::string_view name = "sad";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "hello", &rdstest::detail::MyVariant1_alternatives::sad::hello, /*offset=*/base::nothing },
      refl::StructField{ "ptr", &rdstest::detail::MyVariant1_alternatives::sad::ptr, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct excited.
  template<>
  struct traits<rdstest::detail::MyVariant1_alternatives::excited> {
    using type = rdstest::detail::MyVariant1_alternatives::excited;

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

    namespace MyVariant2_alternatives {

      struct first {
        std::string name = {};
        bool        b = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct first const& ) const = default;
      };

      struct second {
        bool flag1 = {};
        bool flag2 = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct second const& ) const = default;
      };

      struct third {
        int cost = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct third const& ) const = default;
      };

    } // namespace MyVariant2_alternatives

    using MyVariant2Base = base::variant<
      detail::MyVariant2_alternatives::first,
      detail::MyVariant2_alternatives::second,
      detail::MyVariant2_alternatives::third
    >;

  } // namespace detail

  struct MyVariant2 : public detail::MyVariant2Base {
    using first  = detail::MyVariant2_alternatives::first;
    using second = detail::MyVariant2_alternatives::second;
    using third  = detail::MyVariant2_alternatives::third;

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

    // Comparison with alternatives.
    bool operator==( first const& rhs ) const {
      return this->template holds<first>() && (this->template get<first>() == rhs);
    }
    bool operator==( second const& rhs ) const {
      return this->template holds<second>() && (this->template get<second>() == rhs);
    }
    bool operator==( third const& rhs ) const {
      return this->template holds<third>() && (this->template get<third>() == rhs);
    }
  };


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
  struct traits<rdstest::detail::MyVariant2_alternatives::first> {
    using type = rdstest::detail::MyVariant2_alternatives::first;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::MyVariant2";
    static constexpr std::string_view name = "first";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "name", &rdstest::detail::MyVariant2_alternatives::first::name, /*offset=*/base::nothing },
      refl::StructField{ "b", &rdstest::detail::MyVariant2_alternatives::first::b, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct second.
  template<>
  struct traits<rdstest::detail::MyVariant2_alternatives::second> {
    using type = rdstest::detail::MyVariant2_alternatives::second;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::MyVariant2";
    static constexpr std::string_view name = "second";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "flag1", &rdstest::detail::MyVariant2_alternatives::second::flag1, /*offset=*/base::nothing },
      refl::StructField{ "flag2", &rdstest::detail::MyVariant2_alternatives::second::flag2, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct third.
  template<>
  struct traits<rdstest::detail::MyVariant2_alternatives::third> {
    using type = rdstest::detail::MyVariant2_alternatives::third;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::MyVariant2";
    static constexpr std::string_view name = "third";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "cost", &rdstest::detail::MyVariant2_alternatives::third::cost, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                     Sum Type: MyVariant3
*****************************************************************/
namespace rdstest::inner {

  namespace detail {

    namespace MyVariant3_alternatives {

      struct a1 {
        std::monostate var0 = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct a1 const& ) const = default;
      };

      struct a2 {
        std::monostate var1 = {};
        MyVariant2     var2 = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct a2 const& ) const = default;
      };

      struct a3 {
        char c = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct a3 const& ) const = default;
      };

    } // namespace MyVariant3_alternatives

    using MyVariant3Base = base::variant<
      detail::MyVariant3_alternatives::a1,
      detail::MyVariant3_alternatives::a2,
      detail::MyVariant3_alternatives::a3
    >;

  } // namespace detail

  struct MyVariant3 : public detail::MyVariant3Base {
    using a1 = detail::MyVariant3_alternatives::a1;
    using a2 = detail::MyVariant3_alternatives::a2;
    using a3 = detail::MyVariant3_alternatives::a3;

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

    // Comparison with alternatives.
    bool operator==( a1 const& rhs ) const {
      return this->template holds<a1>() && (this->template get<a1>() == rhs);
    }
    bool operator==( a2 const& rhs ) const {
      return this->template holds<a2>() && (this->template get<a2>() == rhs);
    }
    bool operator==( a3 const& rhs ) const {
      return this->template holds<a3>() && (this->template get<a3>() == rhs);
    }
  };


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
  struct traits<rdstest::inner::detail::MyVariant3_alternatives::a1> {
    using type = rdstest::inner::detail::MyVariant3_alternatives::a1;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::MyVariant3";
    static constexpr std::string_view name = "a1";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "var0", &rdstest::inner::detail::MyVariant3_alternatives::a1::var0, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct a2.
  template<>
  struct traits<rdstest::inner::detail::MyVariant3_alternatives::a2> {
    using type = rdstest::inner::detail::MyVariant3_alternatives::a2;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::MyVariant3";
    static constexpr std::string_view name = "a2";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "var1", &rdstest::inner::detail::MyVariant3_alternatives::a2::var1, /*offset=*/base::nothing },
      refl::StructField{ "var2", &rdstest::inner::detail::MyVariant3_alternatives::a2::var2, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct a3.
  template<>
  struct traits<rdstest::inner::detail::MyVariant3_alternatives::a3> {
    using type = rdstest::inner::detail::MyVariant3_alternatives::a3;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::MyVariant3";
    static constexpr std::string_view name = "a3";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "c", &rdstest::inner::detail::MyVariant3_alternatives::a3::c, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                     Sum Type: MyVariant4
*****************************************************************/
namespace rdstest::inner {

  namespace detail {

    namespace MyVariant4_alternatives {

      struct first {
        int                 i = {};
        char                c = {};
        bool                b = {};
        rn::maybe<uint32_t> op = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct first const& ) const = default;
      };

      struct _2nd {
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct _2nd const& ) const = default;
      };

      struct third {
        std::string s = {};
        MyVariant3  var3 = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct third const& ) const = default;
      };

    } // namespace MyVariant4_alternatives

    using MyVariant4Base = base::variant<
      detail::MyVariant4_alternatives::first,
      detail::MyVariant4_alternatives::_2nd,
      detail::MyVariant4_alternatives::third
    >;

  } // namespace detail

  struct MyVariant4 : public detail::MyVariant4Base {
    using first = detail::MyVariant4_alternatives::first;
    using _2nd  = detail::MyVariant4_alternatives::_2nd;
    using third = detail::MyVariant4_alternatives::third;

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

    // Comparison with alternatives.
    bool operator==( first const& rhs ) const {
      return this->template holds<first>() && (this->template get<first>() == rhs);
    }
    bool operator==( _2nd const& rhs ) const {
      return this->template holds<_2nd>() && (this->template get<_2nd>() == rhs);
    }
    bool operator==( third const& rhs ) const {
      return this->template holds<third>() && (this->template get<third>() == rhs);
    }
  };


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
  struct traits<rdstest::inner::detail::MyVariant4_alternatives::first> {
    using type = rdstest::inner::detail::MyVariant4_alternatives::first;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::MyVariant4";
    static constexpr std::string_view name = "first";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "i", &rdstest::inner::detail::MyVariant4_alternatives::first::i, /*offset=*/base::nothing },
      refl::StructField{ "c", &rdstest::inner::detail::MyVariant4_alternatives::first::c, /*offset=*/base::nothing },
      refl::StructField{ "b", &rdstest::inner::detail::MyVariant4_alternatives::first::b, /*offset=*/base::nothing },
      refl::StructField{ "op", &rdstest::inner::detail::MyVariant4_alternatives::first::op, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct _2nd.
  template<>
  struct traits<rdstest::inner::detail::MyVariant4_alternatives::_2nd> {
    using type = rdstest::inner::detail::MyVariant4_alternatives::_2nd;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::MyVariant4";
    static constexpr std::string_view name = "_2nd";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{};
  };

  // Reflection info for struct third.
  template<>
  struct traits<rdstest::inner::detail::MyVariant4_alternatives::third> {
    using type = rdstest::inner::detail::MyVariant4_alternatives::third;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::MyVariant4";
    static constexpr std::string_view name = "third";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "s", &rdstest::inner::detail::MyVariant4_alternatives::third::s, /*offset=*/base::nothing },
      refl::StructField{ "var3", &rdstest::inner::detail::MyVariant4_alternatives::third::var3, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                  Sum Type: TemplateTwoParams
*****************************************************************/
namespace rdstest::inner {

  namespace detail {

    namespace TemplateTwoParams_alternatives {

      template<typename T, typename U>
      struct first_alternative {
        T    t = {};
        char c = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct first_alternative const& ) const = default;
      };

      template<typename T, typename U>
      struct second_alternative {
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct second_alternative const& ) const = default;
      };

      template<typename T, typename U>
      struct third_alternative {
        Maybe<T> hello = {};
        U        u = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct third_alternative const& ) const = default;
      };

    } // namespace TemplateTwoParams_alternatives

    template<typename T, typename U>
    using TemplateTwoParamsBase = base::variant<
      detail::TemplateTwoParams_alternatives::first_alternative<T, U>,
      detail::TemplateTwoParams_alternatives::second_alternative<T, U>,
      detail::TemplateTwoParams_alternatives::third_alternative<T, U>
    >;

  } // namespace detail

  template<typename T, typename U>
  struct TemplateTwoParams : public detail::TemplateTwoParamsBase<T, U> {
    using first_alternative  = detail::TemplateTwoParams_alternatives::first_alternative<T, U>;
    using second_alternative = detail::TemplateTwoParams_alternatives::second_alternative<T, U>;
    using third_alternative  = detail::TemplateTwoParams_alternatives::third_alternative<T, U>;

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

    // Comparison with alternatives.
    bool operator==( first_alternative const& rhs ) const {
      return this->template holds<first_alternative>() && (this->template get<first_alternative>() == rhs);
    }
    bool operator==( second_alternative const& rhs ) const {
      return this->template holds<second_alternative>() && (this->template get<second_alternative>() == rhs);
    }
    bool operator==( third_alternative const& rhs ) const {
      return this->template holds<third_alternative>() && (this->template get<third_alternative>() == rhs);
    }
  };


} // namespace rdstest::inner

// This gives us the enum to use in a switch statement.
template<typename T, typename U>
struct base::variant_to_enum<rdstest::inner::detail::TemplateTwoParamsBase<T, U>> {
  using type = typename rdstest::inner::TemplateTwoParams<T, U>::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct first_alternative.
  template<typename T, typename U>
  struct traits<rdstest::inner::detail::TemplateTwoParams_alternatives::first_alternative<T, U>> {
    using type = rdstest::inner::detail::TemplateTwoParams_alternatives::first_alternative<T, U>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::TemplateTwoParams";
    static constexpr std::string_view name = "first_alternative";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<T, U>;

    static constexpr std::tuple fields{
      refl::StructField{ "t", &rdstest::inner::detail::TemplateTwoParams_alternatives::first_alternative<T, U>::t, /*offset=*/base::nothing },
      refl::StructField{ "c", &rdstest::inner::detail::TemplateTwoParams_alternatives::first_alternative<T, U>::c, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct second_alternative.
  template<typename T, typename U>
  struct traits<rdstest::inner::detail::TemplateTwoParams_alternatives::second_alternative<T, U>> {
    using type = rdstest::inner::detail::TemplateTwoParams_alternatives::second_alternative<T, U>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::TemplateTwoParams";
    static constexpr std::string_view name = "second_alternative";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<T, U>;

    static constexpr std::tuple fields{};
  };

  // Reflection info for struct third_alternative.
  template<typename T, typename U>
  struct traits<rdstest::inner::detail::TemplateTwoParams_alternatives::third_alternative<T, U>> {
    using type = rdstest::inner::detail::TemplateTwoParams_alternatives::third_alternative<T, U>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::TemplateTwoParams";
    static constexpr std::string_view name = "third_alternative";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<T, U>;

    static constexpr std::tuple fields{
      refl::StructField{ "hello", &rdstest::inner::detail::TemplateTwoParams_alternatives::third_alternative<T, U>::hello, /*offset=*/base::nothing },
      refl::StructField{ "u", &rdstest::inner::detail::TemplateTwoParams_alternatives::third_alternative<T, U>::u, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                Sum Type: CompositeTemplateTwo
*****************************************************************/
namespace rdstest::inner {

  namespace detail {

    namespace CompositeTemplateTwo_alternatives {

      template<typename T, typename U>
      struct first {
        rdstest::inner::TemplateTwoParams<T,U> ttp = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct first const& ) const = default;
      };

      template<typename T, typename U>
      struct second {
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct second const& ) const = default;
      };

    } // namespace CompositeTemplateTwo_alternatives

    template<typename T, typename U>
    using CompositeTemplateTwoBase = base::variant<
      detail::CompositeTemplateTwo_alternatives::first<T, U>,
      detail::CompositeTemplateTwo_alternatives::second<T, U>
    >;

  } // namespace detail

  template<typename T, typename U>
  struct CompositeTemplateTwo : public detail::CompositeTemplateTwoBase<T, U> {
    using first  = detail::CompositeTemplateTwo_alternatives::first<T, U>;
    using second = detail::CompositeTemplateTwo_alternatives::second<T, U>;

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

    // Comparison with alternatives.
    bool operator==( first const& rhs ) const {
      return this->template holds<first>() && (this->template get<first>() == rhs);
    }
    bool operator==( second const& rhs ) const {
      return this->template holds<second>() && (this->template get<second>() == rhs);
    }
  };


} // namespace rdstest::inner

// This gives us the enum to use in a switch statement.
template<typename T, typename U>
struct base::variant_to_enum<rdstest::inner::detail::CompositeTemplateTwoBase<T, U>> {
  using type = typename rdstest::inner::CompositeTemplateTwo<T, U>::e;
};

// Reflection traits for alternatives.
namespace refl {

  // Reflection info for struct first.
  template<typename T, typename U>
  struct traits<rdstest::inner::detail::CompositeTemplateTwo_alternatives::first<T, U>> {
    using type = rdstest::inner::detail::CompositeTemplateTwo_alternatives::first<T, U>;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rdstest::inner::CompositeTemplateTwo";
    static constexpr std::string_view name = "first";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<T, U>;

    static constexpr std::tuple fields{
      refl::StructField{ "ttp", &rdstest::inner::detail::CompositeTemplateTwo_alternatives::first<T, U>::ttp, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct second.
  template<typename T, typename U>
  struct traits<rdstest::inner::detail::CompositeTemplateTwo_alternatives::second<T, U>> {
    using type = rdstest::inner::detail::CompositeTemplateTwo_alternatives::second<T, U>;

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

    namespace MySumtype_alternatives {

      struct none {
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct none const& ) const = default;
      };

      struct some {
        std::string s = {};
        int         y = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct some const& ) const = default;
      };

      struct more {
        double d = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct more const& ) const = default;
      };

    } // namespace MySumtype_alternatives

    using MySumtypeBase = base::variant<
      detail::MySumtype_alternatives::none,
      detail::MySumtype_alternatives::some,
      detail::MySumtype_alternatives::more
    >;

  } // namespace detail

  struct MySumtype : public detail::MySumtypeBase {
    using none = detail::MySumtype_alternatives::none;
    using some = detail::MySumtype_alternatives::some;
    using more = detail::MySumtype_alternatives::more;

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

    // Comparison with alternatives.
    bool operator==( none const& rhs ) const {
      return this->template holds<none>() && (this->template get<none>() == rhs);
    }
    bool operator==( some const& rhs ) const {
      return this->template holds<some>() && (this->template get<some>() == rhs);
    }
    bool operator==( more const& rhs ) const {
      return this->template holds<more>() && (this->template get<more>() == rhs);
    }
  };


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
  struct traits<rn::detail::MySumtype_alternatives::none> {
    using type = rn::detail::MySumtype_alternatives::none;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::MySumtype";
    static constexpr std::string_view name = "none";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{};
  };

  // Reflection info for struct some.
  template<>
  struct traits<rn::detail::MySumtype_alternatives::some> {
    using type = rn::detail::MySumtype_alternatives::some;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::MySumtype";
    static constexpr std::string_view name = "some";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "s", &rn::detail::MySumtype_alternatives::some::s, /*offset=*/base::nothing },
      refl::StructField{ "y", &rn::detail::MySumtype_alternatives::some::y, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct more.
  template<>
  struct traits<rn::detail::MySumtype_alternatives::more> {
    using type = rn::detail::MySumtype_alternatives::more;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::MySumtype";
    static constexpr std::string_view name = "more";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "d", &rn::detail::MySumtype_alternatives::more::d, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                     Sum Type: OnOffState
*****************************************************************/
namespace rn {

  namespace detail {

    namespace OnOffState_alternatives {

      struct off {
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct off const& ) const = default;
      };

      struct on {
        std::string user = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct on const& ) const = default;
      };

      struct switching_on {
        double percent = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct switching_on const& ) const = default;
      };

      struct switching_off {
        double percent = {};
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct switching_off const& ) const = default;
      };

    } // namespace OnOffState_alternatives

    using OnOffStateBase = base::variant<
      detail::OnOffState_alternatives::off,
      detail::OnOffState_alternatives::on,
      detail::OnOffState_alternatives::switching_on,
      detail::OnOffState_alternatives::switching_off
    >;

  } // namespace detail

  struct OnOffState : public detail::OnOffStateBase {
    using off           = detail::OnOffState_alternatives::off;
    using on            = detail::OnOffState_alternatives::on;
    using switching_on  = detail::OnOffState_alternatives::switching_on;
    using switching_off = detail::OnOffState_alternatives::switching_off;

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

    // Comparison with alternatives.
    bool operator==( off const& rhs ) const {
      return this->template holds<off>() && (this->template get<off>() == rhs);
    }
    bool operator==( on const& rhs ) const {
      return this->template holds<on>() && (this->template get<on>() == rhs);
    }
    bool operator==( switching_on const& rhs ) const {
      return this->template holds<switching_on>() && (this->template get<switching_on>() == rhs);
    }
    bool operator==( switching_off const& rhs ) const {
      return this->template holds<switching_off>() && (this->template get<switching_off>() == rhs);
    }
  };


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
  struct traits<rn::detail::OnOffState_alternatives::off> {
    using type = rn::detail::OnOffState_alternatives::off;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::OnOffState";
    static constexpr std::string_view name = "off";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{};
  };

  // Reflection info for struct on.
  template<>
  struct traits<rn::detail::OnOffState_alternatives::on> {
    using type = rn::detail::OnOffState_alternatives::on;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::OnOffState";
    static constexpr std::string_view name = "on";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "user", &rn::detail::OnOffState_alternatives::on::user, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct switching_on.
  template<>
  struct traits<rn::detail::OnOffState_alternatives::switching_on> {
    using type = rn::detail::OnOffState_alternatives::switching_on;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::OnOffState";
    static constexpr std::string_view name = "switching_on";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "percent", &rn::detail::OnOffState_alternatives::switching_on::percent, /*offset=*/base::nothing },
    };
  };

  // Reflection info for struct switching_off.
  template<>
  struct traits<rn::detail::OnOffState_alternatives::switching_off> {
    using type = rn::detail::OnOffState_alternatives::switching_off;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::OnOffState";
    static constexpr std::string_view name = "switching_off";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{
      refl::StructField{ "percent", &rn::detail::OnOffState_alternatives::switching_off::percent, /*offset=*/base::nothing },
    };
  };

} // namespace refl

/****************************************************************
*                     Sum Type: OnOffEvent
*****************************************************************/
namespace rn {

  namespace detail {

    namespace OnOffEvent_alternatives {

      struct turn_off {
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct turn_off const& ) const = default;
      };

      struct turn_on {
        // This requires that the types of the member variables
        // also support equality.
        bool operator==( struct turn_on const& ) const = default;
      };

    } // namespace OnOffEvent_alternatives

    using OnOffEventBase = base::variant<
      detail::OnOffEvent_alternatives::turn_off,
      detail::OnOffEvent_alternatives::turn_on
    >;

  } // namespace detail

  struct OnOffEvent : public detail::OnOffEventBase {
    using turn_off = detail::OnOffEvent_alternatives::turn_off;
    using turn_on  = detail::OnOffEvent_alternatives::turn_on;

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

    // Comparison with alternatives.
    bool operator==( turn_off const& rhs ) const {
      return this->template holds<turn_off>() && (this->template get<turn_off>() == rhs);
    }
    bool operator==( turn_on const& rhs ) const {
      return this->template holds<turn_on>() && (this->template get<turn_on>() == rhs);
    }
  };


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
  struct traits<rn::detail::OnOffEvent_alternatives::turn_off> {
    using type = rn::detail::OnOffEvent_alternatives::turn_off;

    static constexpr type_kind kind        = type_kind::struct_kind;
    static constexpr std::string_view ns   = "rn::OnOffEvent";
    static constexpr std::string_view name = "turn_off";
    static constexpr bool is_sumtype_alternative = true;

    using template_types = std::tuple<>;

    static constexpr std::tuple fields{};
  };

  // Reflection info for struct turn_on.
  template<>
  struct traits<rn::detail::OnOffEvent_alternatives::turn_on> {
    using type = rn::detail::OnOffEvent_alternatives::turn_on;

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
    T                                 xxx     = {};
    double                            yyy     = {};
    std::unordered_map<std::string,U> zzz_map = {};

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

// The end.
