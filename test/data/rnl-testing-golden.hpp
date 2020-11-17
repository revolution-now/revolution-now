// Auto-Generated file, do not modify! (testing).
#pragma once

/****************************************************************
*                           Includes
*****************************************************************/
// Includes specified in rnl file.
#include <string>
#include <variant>
#include <vector>
#include "fb/testing_generated.h"

// Revolution Now
#include "core-config.hpp"
#include "cc-specific.hpp"
#include "rnl/helper/sumtype-helper.hpp"
#include "fmt-helper.hpp"
#include "errors.hpp"
#include "fb.hpp"

// base-util
#include "base-util/mp.hpp"

// {fmt}
#include "fmt/format.h"

// C++ standard library
#include <string_view>
#include <variant>

/****************************************************************
*                          Global Vars
*****************************************************************/
namespace rn {

  // This will be the naem of this header, not the file that it
  // is include in.
  inline constexpr std::string_view rnl_testing_genfile = __FILE__;

} // namespace rn

/****************************************************************
*                        Sum Type: Maybe
*****************************************************************/
namespace rnltest {

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
  using Maybe_t = std::variant<
    Maybe::nothing<T>,
    Maybe::just<T>
  >;
  NOTHROW_MOVE( Maybe_t<int> );

} // namespace rnltest

// This gives us the enum to use in a switch statement.
template<typename T>
struct rn::SumtypeToEnum<rnltest::Maybe_t<T>> {
  using type = rnltest::Maybe::e;
};

// rnltest::Maybe::nothing
template<typename T>
struct fmt::formatter<rnltest::Maybe::nothing<T>>
  : formatter_base {
  template<typename Context>
  auto format( rnltest::Maybe::nothing<T> const&, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "Maybe::nothing<{}>"
      , ::rn::type_list_to_names<T>() ), ctx );
  }
};

// rnltest::Maybe::just
template<typename T>
struct fmt::formatter<rnltest::Maybe::just<T>>
  : formatter_base {
  template<typename Context>
  auto format( rnltest::Maybe::just<T> const& o, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "Maybe::just<{}>{{"
        "val={}"
      "}}"
      , ::rn::type_list_to_names<T>(), o.val ), ctx );
  }
};

/****************************************************************
*                     Sum Type: MyVariant0
*****************************************************************/
namespace rnltest {

  using MyVariant0_t = std::monostate;

} // namespace rnltest

/****************************************************************
*                     Sum Type: MyVariant1
*****************************************************************/
namespace rnltest {

  namespace MyVariant1 {

    struct happy {
      std::pair<char,int> p;
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

  using MyVariant1_t = std::variant<
    MyVariant1::happy,
    MyVariant1::sad,
    MyVariant1::excited
  >;
  NOTHROW_MOVE( MyVariant1_t );

} // namespace rnltest

// This gives us the enum to use in a switch statement.
template<>
struct rn::SumtypeToEnum<rnltest::MyVariant1_t> {
  using type = rnltest::MyVariant1::e;
};

/****************************************************************
*                     Sum Type: MyVariant2
*****************************************************************/
namespace rnltest {

  namespace MyVariant2 {

    struct first {
      std::string name;
      bool        b;
      using fb_target_t = fb::MyVariant2::first;

      static std::string fb_root_type_name() {
        return "fb.MyVariant2.first";
      }

      FBOffset<fb::MyVariant2::first> serialize_table(
          FBBuilder& builder ) const {
        (void)fb_root_type_name;
        using ::rn::serial::serialize;
          
        auto s_name = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().name() )>>(
            builder, name, ::rn::serial::ADL{} );

        auto s_b = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().b() )>>(
            builder, b, ::rn::serial::ADL{} );

        return fb::MyVariant2::Createfirst( builder
            , s_name.get(), s_b.get()
        );
      }

      static ::rn::expect<> deserialize_table(
          fb::MyVariant2::first const& src,
          first* dst ) {
        (void)src;
        (void)dst;
        DCHECK( dst );
        using ::rn::serial::deserialize;
          
        XP_OR_RETURN_( deserialize(
            ::rn::serial::detail::to_const_ptr( src.name() ),
            &dst->name, ::rn::serial::ADL{} ) );

        XP_OR_RETURN_( deserialize(
            ::rn::serial::detail::to_const_ptr( src.b() ),
            &dst->b, ::rn::serial::ADL{} ) );

        return ::rn::xp_success_t{};
      }

      template<typename ParentBuilderT>
      void builder_add_me( ParentBuilderT& builder,
                           FBOffset<void>  offset ) const {
        FBOffset<::fb::MyVariant2::first> typed_offset(
            offset.o );
        builder.add_first( typed_offset );
      }

      template<typename ParentSrcT, typename ParentDstT>
      static ::rn::expect<bool> try_deserialize_me(
          ParentSrcT const* src,
          ParentDstT* dst ) {
        if( src->first() == nullptr ) return false;
        *dst = first{};
        XP_OR_RETURN_( deserialize(
            src->first(), std::get_if<first>( dst ),
            ::rn::serial::ADL{} ) );
        return true;
      }

      ::rn::expect<> check_invariants_safe() const {
        return ::rn::xp_success_t{};
      }

    };

    struct second {
      bool flag1;
      bool flag2;
      using fb_target_t = fb::MyVariant2::second;

      static std::string fb_root_type_name() {
        return "fb.MyVariant2.second";
      }

      FBOffset<fb::MyVariant2::second> serialize_table(
          FBBuilder& builder ) const {
        (void)fb_root_type_name;
        using ::rn::serial::serialize;
          
        auto s_flag1 = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().flag1() )>>(
            builder, flag1, ::rn::serial::ADL{} );

        auto s_flag2 = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().flag2() )>>(
            builder, flag2, ::rn::serial::ADL{} );

        return fb::MyVariant2::Createsecond( builder
            , s_flag1.get(), s_flag2.get()
        );
      }

      static ::rn::expect<> deserialize_table(
          fb::MyVariant2::second const& src,
          second* dst ) {
        (void)src;
        (void)dst;
        DCHECK( dst );
        using ::rn::serial::deserialize;
          
        XP_OR_RETURN_( deserialize(
            ::rn::serial::detail::to_const_ptr( src.flag1() ),
            &dst->flag1, ::rn::serial::ADL{} ) );

        XP_OR_RETURN_( deserialize(
            ::rn::serial::detail::to_const_ptr( src.flag2() ),
            &dst->flag2, ::rn::serial::ADL{} ) );

        return ::rn::xp_success_t{};
      }

      template<typename ParentBuilderT>
      void builder_add_me( ParentBuilderT& builder,
                           FBOffset<void>  offset ) const {
        FBOffset<::fb::MyVariant2::second> typed_offset(
            offset.o );
        builder.add_second( typed_offset );
      }

      template<typename ParentSrcT, typename ParentDstT>
      static ::rn::expect<bool> try_deserialize_me(
          ParentSrcT const* src,
          ParentDstT* dst ) {
        if( src->second() == nullptr ) return false;
        *dst = second{};
        XP_OR_RETURN_( deserialize(
            src->second(), std::get_if<second>( dst ),
            ::rn::serial::ADL{} ) );
        return true;
      }

      ::rn::expect<> check_invariants_safe() const {
        return ::rn::xp_success_t{};
      }

    };

    struct third {
      int cost;
      using fb_target_t = fb::MyVariant2::third;

      static std::string fb_root_type_name() {
        return "fb.MyVariant2.third";
      }

      FBOffset<fb::MyVariant2::third> serialize_table(
          FBBuilder& builder ) const {
        (void)fb_root_type_name;
        using ::rn::serial::serialize;
          
        auto s_cost = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().cost() )>>(
            builder, cost, ::rn::serial::ADL{} );

        return fb::MyVariant2::Createthird( builder
            , s_cost.get()
        );
      }

      static ::rn::expect<> deserialize_table(
          fb::MyVariant2::third const& src,
          third* dst ) {
        (void)src;
        (void)dst;
        DCHECK( dst );
        using ::rn::serial::deserialize;
          
        XP_OR_RETURN_( deserialize(
            ::rn::serial::detail::to_const_ptr( src.cost() ),
            &dst->cost, ::rn::serial::ADL{} ) );

        return ::rn::xp_success_t{};
      }

      template<typename ParentBuilderT>
      void builder_add_me( ParentBuilderT& builder,
                           FBOffset<void>  offset ) const {
        FBOffset<::fb::MyVariant2::third> typed_offset(
            offset.o );
        builder.add_third( typed_offset );
      }

      template<typename ParentSrcT, typename ParentDstT>
      static ::rn::expect<bool> try_deserialize_me(
          ParentSrcT const* src,
          ParentDstT* dst ) {
        if( src->third() == nullptr ) return false;
        *dst = third{};
        XP_OR_RETURN_( deserialize(
            src->third(), std::get_if<third>( dst ),
            ::rn::serial::ADL{} ) );
        return true;
      }

      ::rn::expect<> check_invariants_safe() const {
        return ::rn::xp_success_t{};
      }

    };

    enum class e {
      first,
      second,
      third,
    };

  } // namespace MyVariant2

  using MyVariant2_t = std::variant<
    MyVariant2::first,
    MyVariant2::second,
    MyVariant2::third
  >;
  NOTHROW_MOVE( MyVariant2_t );

} // namespace rnltest

// This gives us the enum to use in a switch statement.
template<>
struct rn::SumtypeToEnum<rnltest::MyVariant2_t> {
  using type = rnltest::MyVariant2::e;
};

// rnltest::MyVariant2::first
template<>
struct fmt::formatter<rnltest::MyVariant2::first>
  : formatter_base {
  template<typename Context>
  auto format( rnltest::MyVariant2::first const& o, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "MyVariant2::first{{"
        "name={},"
        "b={}"
      "}}"
      , o.name, o.b ), ctx );
  }
};

// rnltest::MyVariant2::second
template<>
struct fmt::formatter<rnltest::MyVariant2::second>
  : formatter_base {
  template<typename Context>
  auto format( rnltest::MyVariant2::second const& o, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "MyVariant2::second{{"
        "flag1={},"
        "flag2={}"
      "}}"
      , o.flag1, o.flag2 ), ctx );
  }
};

// rnltest::MyVariant2::third
template<>
struct fmt::formatter<rnltest::MyVariant2::third>
  : formatter_base {
  template<typename Context>
  auto format( rnltest::MyVariant2::third const& o, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "MyVariant2::third{{"
        "cost={}"
      "}}"
      , o.cost ), ctx );
  }
};

namespace rn::serial {

  template<typename Hint>
  auto serialize( FBBuilder& fbb, ::rnltest::MyVariant2_t const& o,
                  ::rn::serial::ADL ) {
    auto offset  = util::visit( o, [&]( auto const& v ) {
      // Call Union() to make the offset templated on type `void`
      // instead of the type of this variant member so that we have
      // a consistent return type.
      return v.serialize_table( fbb ).Union();
    } );
    auto builder = fb::MyVariant2_tBuilder( fbb );
    util::visit( o, [&]( auto const& v ) {
      v.builder_add_me( builder, offset );
    } );
    return ::rn::serial::ReturnValue{ builder.Finish() };
  }

  expect<> inline deserialize( fb::MyVariant2_t const* src,
                               ::rnltest::MyVariant2_t* dst,
                               ::rn::serial::ADL ) {
    if( src == nullptr ) return ::rn::xp_success_t{};
    int            count  = 0;
    ::rn::expect<> result = ::rn::xp_success_t{};
    ::rn::try_deserialize_variant_types<::rnltest::MyVariant2_t>(
        [&]( auto const* p ) {
          if( !result ) return;
          using type =
              std::decay_t<std::remove_pointer_t<decltype( p )>>;
          auto xp = type::try_deserialize_me( src, dst );
          if( !xp ) {
            result = UNEXPECTED( "{}", xp.error().what );
            return;
          }
          auto deserialized = *xp;
          if( deserialized ) ++count;
        } );
    if( !result ) return result;
    if( count != 1 )
      return UNEXPECTED(
          "failed to deserialized precisely one variant element "
          "(found {})",
          count );
    return ::rn::xp_success_t{};
  }

} // namespace rn::serial

/****************************************************************
*                     Sum Type: MyVariant3
*****************************************************************/
namespace rnltest::inner {

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

  using MyVariant3_t = std::variant<
    MyVariant3::a1,
    MyVariant3::a2,
    MyVariant3::a3
  >;
  NOTHROW_MOVE( MyVariant3_t );

} // namespace rnltest::inner

// This gives us the enum to use in a switch statement.
template<>
struct rn::SumtypeToEnum<rnltest::inner::MyVariant3_t> {
  using type = rnltest::inner::MyVariant3::e;
};

// rnltest::inner::MyVariant3::a1
template<>
struct fmt::formatter<rnltest::inner::MyVariant3::a1>
  : formatter_base {
  template<typename Context>
  auto format( rnltest::inner::MyVariant3::a1 const& o, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "MyVariant3::a1{{"
        "var0={}"
      "}}"
      , o.var0 ), ctx );
  }
};

// rnltest::inner::MyVariant3::a2
template<>
struct fmt::formatter<rnltest::inner::MyVariant3::a2>
  : formatter_base {
  template<typename Context>
  auto format( rnltest::inner::MyVariant3::a2 const& o, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "MyVariant3::a2{{"
        "var1={},"
        "var2={}"
      "}}"
      , o.var1, o.var2 ), ctx );
  }
};

// rnltest::inner::MyVariant3::a3
template<>
struct fmt::formatter<rnltest::inner::MyVariant3::a3>
  : formatter_base {
  template<typename Context>
  auto format( rnltest::inner::MyVariant3::a3 const& o, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "MyVariant3::a3{{"
        "c={}"
      "}}"
      , o.c ), ctx );
  }
};

/****************************************************************
*                     Sum Type: MyVariant4
*****************************************************************/
namespace rnltest::inner {

  namespace MyVariant4 {

    struct first {
      int                     i;
      char                    c;
      bool                    b;
      std::optional<uint32_t> op;
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

  using MyVariant4_t = std::variant<
    MyVariant4::first,
    MyVariant4::_2nd,
    MyVariant4::third
  >;
  NOTHROW_MOVE( MyVariant4_t );

} // namespace rnltest::inner

// This gives us the enum to use in a switch statement.
template<>
struct rn::SumtypeToEnum<rnltest::inner::MyVariant4_t> {
  using type = rnltest::inner::MyVariant4::e;
};

// rnltest::inner::MyVariant4::first
template<>
struct fmt::formatter<rnltest::inner::MyVariant4::first>
  : formatter_base {
  template<typename Context>
  auto format( rnltest::inner::MyVariant4::first const& o, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "MyVariant4::first{{"
        "i={},"
        "c={},"
        "b={},"
        "op={}"
      "}}"
      , o.i, o.c, o.b, o.op ), ctx );
  }
};

// rnltest::inner::MyVariant4::_2nd
template<>
struct fmt::formatter<rnltest::inner::MyVariant4::_2nd>
  : formatter_base {
  template<typename Context>
  auto format( rnltest::inner::MyVariant4::_2nd const&, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "MyVariant4::_2nd"
       ), ctx );
  }
};

// rnltest::inner::MyVariant4::third
template<>
struct fmt::formatter<rnltest::inner::MyVariant4::third>
  : formatter_base {
  template<typename Context>
  auto format( rnltest::inner::MyVariant4::third const& o, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "MyVariant4::third{{"
        "s={},"
        "var3={}"
      "}}"
      , o.s, o.var3 ), ctx );
  }
};

/****************************************************************
*                  Sum Type: TemplateTwoParams
*****************************************************************/
namespace rnltest::inner {

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
  using TemplateTwoParams_t = std::variant<
    TemplateTwoParams::first_alternative<T, U>,
    TemplateTwoParams::second_alternative<T, U>,
    TemplateTwoParams::third_alternative<T, U>
  >;
  NOTHROW_MOVE( TemplateTwoParams_t<int, int> );

} // namespace rnltest::inner

// This gives us the enum to use in a switch statement.
template<typename T, typename U>
struct rn::SumtypeToEnum<rnltest::inner::TemplateTwoParams_t<T, U>> {
  using type = rnltest::inner::TemplateTwoParams::e;
};

// rnltest::inner::TemplateTwoParams::first_alternative
template<typename T, typename U>
struct fmt::formatter<rnltest::inner::TemplateTwoParams::first_alternative<T, U>>
  : formatter_base {
  template<typename Context>
  auto format( rnltest::inner::TemplateTwoParams::first_alternative<T, U> const& o, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "TemplateTwoParams::first_alternative<{}>{{"
        "t={},"
        "c={}"
      "}}"
      , ::rn::type_list_to_names<T, U>(), o.t, o.c ), ctx );
  }
};

// rnltest::inner::TemplateTwoParams::second_alternative
template<typename T, typename U>
struct fmt::formatter<rnltest::inner::TemplateTwoParams::second_alternative<T, U>>
  : formatter_base {
  template<typename Context>
  auto format( rnltest::inner::TemplateTwoParams::second_alternative<T, U> const&, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "TemplateTwoParams::second_alternative<{}>"
      , ::rn::type_list_to_names<T, U>() ), ctx );
  }
};

// rnltest::inner::TemplateTwoParams::third_alternative
template<typename T, typename U>
struct fmt::formatter<rnltest::inner::TemplateTwoParams::third_alternative<T, U>>
  : formatter_base {
  template<typename Context>
  auto format( rnltest::inner::TemplateTwoParams::third_alternative<T, U> const& o, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "TemplateTwoParams::third_alternative<{}>{{"
        "hello={},"
        "u={}"
      "}}"
      , ::rn::type_list_to_names<T, U>(), o.hello, o.u ), ctx );
  }
};

/****************************************************************
*                Sum Type: CompositeTemplateTwo
*****************************************************************/
namespace rnltest::inner {

  namespace CompositeTemplateTwo {

    template<typename T, typename U>
    struct first {
      rnltest::inner::TemplateTwoParams_t<T,U> ttp;
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
  using CompositeTemplateTwo_t = std::variant<
    CompositeTemplateTwo::first<T, U>,
    CompositeTemplateTwo::second<T, U>
  >;
  NOTHROW_MOVE( CompositeTemplateTwo_t<int, int> );

} // namespace rnltest::inner

// This gives us the enum to use in a switch statement.
template<typename T, typename U>
struct rn::SumtypeToEnum<rnltest::inner::CompositeTemplateTwo_t<T, U>> {
  using type = rnltest::inner::CompositeTemplateTwo::e;
};

// rnltest::inner::CompositeTemplateTwo::first
template<typename T, typename U>
struct fmt::formatter<rnltest::inner::CompositeTemplateTwo::first<T, U>>
  : formatter_base {
  template<typename Context>
  auto format( rnltest::inner::CompositeTemplateTwo::first<T, U> const& o, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "CompositeTemplateTwo::first<{}>{{"
        "ttp={}"
      "}}"
      , ::rn::type_list_to_names<T, U>(), o.ttp ), ctx );
  }
};

// rnltest::inner::CompositeTemplateTwo::second
template<typename T, typename U>
struct fmt::formatter<rnltest::inner::CompositeTemplateTwo::second<T, U>>
  : formatter_base {
  template<typename Context>
  auto format( rnltest::inner::CompositeTemplateTwo::second<T, U> const&, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "CompositeTemplateTwo::second<{}>"
      , ::rn::type_list_to_names<T, U>() ), ctx );
  }
};

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

      static std::string fb_root_type_name() {
        return "fb.MySumtype.none";
      }

      FBOffset<fb::MySumtype::none> serialize_table(
          FBBuilder& builder ) const {
        (void)fb_root_type_name;
        using ::rn::serial::serialize;
          
        return fb::MySumtype::Createnone( builder
              
        );
      }

      static ::rn::expect<> deserialize_table(
          fb::MySumtype::none const& src,
          none* dst ) {
        (void)src;
        (void)dst;
        DCHECK( dst );
        using ::rn::serial::deserialize;
          
        return ::rn::xp_success_t{};
      }

      template<typename ParentBuilderT>
      void builder_add_me( ParentBuilderT& builder,
                           FBOffset<void>  offset ) const {
        FBOffset<::fb::MySumtype::none> typed_offset(
            offset.o );
        builder.add_none( typed_offset );
      }

      template<typename ParentSrcT, typename ParentDstT>
      static ::rn::expect<bool> try_deserialize_me(
          ParentSrcT const* src,
          ParentDstT* dst ) {
        if( src->none() == nullptr ) return false;
        *dst = none{};
        XP_OR_RETURN_( deserialize(
            src->none(), std::get_if<none>( dst ),
            ::rn::serial::ADL{} ) );
        return true;
      }

      ::rn::expect<> check_invariants_safe() const {
        return ::rn::xp_success_t{};
      }

    };

    struct some {
      std::string s;
      int         y;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct some const& ) const = default;
      bool operator!=( struct some const& ) const = default;
      using fb_target_t = fb::MySumtype::some;

      static std::string fb_root_type_name() {
        return "fb.MySumtype.some";
      }

      FBOffset<fb::MySumtype::some> serialize_table(
          FBBuilder& builder ) const {
        (void)fb_root_type_name;
        using ::rn::serial::serialize;
          
        auto s_s = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().s() )>>(
            builder, s, ::rn::serial::ADL{} );

        auto s_y = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().y() )>>(
            builder, y, ::rn::serial::ADL{} );

        return fb::MySumtype::Createsome( builder
            , s_s.get(), s_y.get()
        );
      }

      static ::rn::expect<> deserialize_table(
          fb::MySumtype::some const& src,
          some* dst ) {
        (void)src;
        (void)dst;
        DCHECK( dst );
        using ::rn::serial::deserialize;
          
        XP_OR_RETURN_( deserialize(
            ::rn::serial::detail::to_const_ptr( src.s() ),
            &dst->s, ::rn::serial::ADL{} ) );

        XP_OR_RETURN_( deserialize(
            ::rn::serial::detail::to_const_ptr( src.y() ),
            &dst->y, ::rn::serial::ADL{} ) );

        return ::rn::xp_success_t{};
      }

      template<typename ParentBuilderT>
      void builder_add_me( ParentBuilderT& builder,
                           FBOffset<void>  offset ) const {
        FBOffset<::fb::MySumtype::some> typed_offset(
            offset.o );
        builder.add_some( typed_offset );
      }

      template<typename ParentSrcT, typename ParentDstT>
      static ::rn::expect<bool> try_deserialize_me(
          ParentSrcT const* src,
          ParentDstT* dst ) {
        if( src->some() == nullptr ) return false;
        *dst = some{};
        XP_OR_RETURN_( deserialize(
            src->some(), std::get_if<some>( dst ),
            ::rn::serial::ADL{} ) );
        return true;
      }

      ::rn::expect<> check_invariants_safe() const {
        return ::rn::xp_success_t{};
      }

    };

    struct more {
      double d;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct more const& ) const = default;
      bool operator!=( struct more const& ) const = default;
      using fb_target_t = fb::MySumtype::more;

      static std::string fb_root_type_name() {
        return "fb.MySumtype.more";
      }

      FBOffset<fb::MySumtype::more> serialize_table(
          FBBuilder& builder ) const {
        (void)fb_root_type_name;
        using ::rn::serial::serialize;
          
        auto s_d = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().d() )>>(
            builder, d, ::rn::serial::ADL{} );

        return fb::MySumtype::Createmore( builder
            , s_d.get()
        );
      }

      static ::rn::expect<> deserialize_table(
          fb::MySumtype::more const& src,
          more* dst ) {
        (void)src;
        (void)dst;
        DCHECK( dst );
        using ::rn::serial::deserialize;
          
        XP_OR_RETURN_( deserialize(
            ::rn::serial::detail::to_const_ptr( src.d() ),
            &dst->d, ::rn::serial::ADL{} ) );

        return ::rn::xp_success_t{};
      }

      template<typename ParentBuilderT>
      void builder_add_me( ParentBuilderT& builder,
                           FBOffset<void>  offset ) const {
        FBOffset<::fb::MySumtype::more> typed_offset(
            offset.o );
        builder.add_more( typed_offset );
      }

      template<typename ParentSrcT, typename ParentDstT>
      static ::rn::expect<bool> try_deserialize_me(
          ParentSrcT const* src,
          ParentDstT* dst ) {
        if( src->more() == nullptr ) return false;
        *dst = more{};
        XP_OR_RETURN_( deserialize(
            src->more(), std::get_if<more>( dst ),
            ::rn::serial::ADL{} ) );
        return true;
      }

      ::rn::expect<> check_invariants_safe() const {
        return ::rn::xp_success_t{};
      }

    };

    enum class e {
      none,
      some,
      more,
    };

  } // namespace MySumtype

  using MySumtype_t = std::variant<
    MySumtype::none,
    MySumtype::some,
    MySumtype::more
  >;
  NOTHROW_MOVE( MySumtype_t );

} // namespace rn

// This gives us the enum to use in a switch statement.
template<>
struct rn::SumtypeToEnum<rn::MySumtype_t> {
  using type = rn::MySumtype::e;
};

// rn::MySumtype::none
template<>
struct fmt::formatter<rn::MySumtype::none>
  : formatter_base {
  template<typename Context>
  auto format( rn::MySumtype::none const&, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "MySumtype::none"
       ), ctx );
  }
};

// rn::MySumtype::some
template<>
struct fmt::formatter<rn::MySumtype::some>
  : formatter_base {
  template<typename Context>
  auto format( rn::MySumtype::some const& o, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "MySumtype::some{{"
        "s={},"
        "y={}"
      "}}"
      , o.s, o.y ), ctx );
  }
};

// rn::MySumtype::more
template<>
struct fmt::formatter<rn::MySumtype::more>
  : formatter_base {
  template<typename Context>
  auto format( rn::MySumtype::more const& o, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "MySumtype::more{{"
        "d={}"
      "}}"
      , o.d ), ctx );
  }
};

namespace rn::serial {

  template<typename Hint>
  auto serialize( FBBuilder& fbb, ::rn::MySumtype_t const& o,
                  ::rn::serial::ADL ) {
    auto offset  = util::visit( o, [&]( auto const& v ) {
      // Call Union() to make the offset templated on type `void`
      // instead of the type of this variant member so that we have
      // a consistent return type.
      return v.serialize_table( fbb ).Union();
    } );
    auto builder = fb::MySumtype_tBuilder( fbb );
    util::visit( o, [&]( auto const& v ) {
      v.builder_add_me( builder, offset );
    } );
    return ::rn::serial::ReturnValue{ builder.Finish() };
  }

  expect<> inline deserialize( fb::MySumtype_t const* src,
                               ::rn::MySumtype_t* dst,
                               ::rn::serial::ADL ) {
    if( src == nullptr ) return ::rn::xp_success_t{};
    int            count  = 0;
    ::rn::expect<> result = ::rn::xp_success_t{};
    ::rn::try_deserialize_variant_types<::rn::MySumtype_t>(
        [&]( auto const* p ) {
          if( !result ) return;
          using type =
              std::decay_t<std::remove_pointer_t<decltype( p )>>;
          auto xp = type::try_deserialize_me( src, dst );
          if( !xp ) {
            result = UNEXPECTED( "{}", xp.error().what );
            return;
          }
          auto deserialized = *xp;
          if( deserialized ) ++count;
        } );
    if( !result ) return result;
    if( count != 1 )
      return UNEXPECTED(
          "failed to deserialized precisely one variant element "
          "(found {})",
          count );
    return ::rn::xp_success_t{};
  }

} // namespace rn::serial

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

      static std::string fb_root_type_name() {
        return "fb.OnOffState.off";
      }

      FBOffset<fb::OnOffState::off> serialize_table(
          FBBuilder& builder ) const {
        (void)fb_root_type_name;
        using ::rn::serial::serialize;
          
        return fb::OnOffState::Createoff( builder
              
        );
      }

      static ::rn::expect<> deserialize_table(
          fb::OnOffState::off const& src,
          off* dst ) {
        (void)src;
        (void)dst;
        DCHECK( dst );
        using ::rn::serial::deserialize;
          
        return ::rn::xp_success_t{};
      }

      template<typename ParentBuilderT>
      void builder_add_me( ParentBuilderT& builder,
                           FBOffset<void>  offset ) const {
        FBOffset<::fb::OnOffState::off> typed_offset(
            offset.o );
        builder.add_off( typed_offset );
      }

      template<typename ParentSrcT, typename ParentDstT>
      static ::rn::expect<bool> try_deserialize_me(
          ParentSrcT const* src,
          ParentDstT* dst ) {
        if( src->off() == nullptr ) return false;
        *dst = off{};
        XP_OR_RETURN_( deserialize(
            src->off(), std::get_if<off>( dst ),
            ::rn::serial::ADL{} ) );
        return true;
      }

      ::rn::expect<> check_invariants_safe() const {
        return ::rn::xp_success_t{};
      }

    };

    struct on {
      std::string user;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct on const& ) const = default;
      bool operator!=( struct on const& ) const = default;
      using fb_target_t = fb::OnOffState::on;

      static std::string fb_root_type_name() {
        return "fb.OnOffState.on";
      }

      FBOffset<fb::OnOffState::on> serialize_table(
          FBBuilder& builder ) const {
        (void)fb_root_type_name;
        using ::rn::serial::serialize;
          
        auto s_user = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().user() )>>(
            builder, user, ::rn::serial::ADL{} );

        return fb::OnOffState::Createon( builder
            , s_user.get()
        );
      }

      static ::rn::expect<> deserialize_table(
          fb::OnOffState::on const& src,
          on* dst ) {
        (void)src;
        (void)dst;
        DCHECK( dst );
        using ::rn::serial::deserialize;
          
        XP_OR_RETURN_( deserialize(
            ::rn::serial::detail::to_const_ptr( src.user() ),
            &dst->user, ::rn::serial::ADL{} ) );

        return ::rn::xp_success_t{};
      }

      template<typename ParentBuilderT>
      void builder_add_me( ParentBuilderT& builder,
                           FBOffset<void>  offset ) const {
        FBOffset<::fb::OnOffState::on> typed_offset(
            offset.o );
        builder.add_on( typed_offset );
      }

      template<typename ParentSrcT, typename ParentDstT>
      static ::rn::expect<bool> try_deserialize_me(
          ParentSrcT const* src,
          ParentDstT* dst ) {
        if( src->on() == nullptr ) return false;
        *dst = on{};
        XP_OR_RETURN_( deserialize(
            src->on(), std::get_if<on>( dst ),
            ::rn::serial::ADL{} ) );
        return true;
      }

      ::rn::expect<> check_invariants_safe() const {
        return ::rn::xp_success_t{};
      }

    };

    struct switching_on {
      double percent;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct switching_on const& ) const = default;
      bool operator!=( struct switching_on const& ) const = default;
      using fb_target_t = fb::OnOffState::switching_on;

      static std::string fb_root_type_name() {
        return "fb.OnOffState.switching_on";
      }

      FBOffset<fb::OnOffState::switching_on> serialize_table(
          FBBuilder& builder ) const {
        (void)fb_root_type_name;
        using ::rn::serial::serialize;
          
        auto s_percent = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().percent() )>>(
            builder, percent, ::rn::serial::ADL{} );

        return fb::OnOffState::Createswitching_on( builder
            , s_percent.get()
        );
      }

      static ::rn::expect<> deserialize_table(
          fb::OnOffState::switching_on const& src,
          switching_on* dst ) {
        (void)src;
        (void)dst;
        DCHECK( dst );
        using ::rn::serial::deserialize;
          
        XP_OR_RETURN_( deserialize(
            ::rn::serial::detail::to_const_ptr( src.percent() ),
            &dst->percent, ::rn::serial::ADL{} ) );

        return ::rn::xp_success_t{};
      }

      template<typename ParentBuilderT>
      void builder_add_me( ParentBuilderT& builder,
                           FBOffset<void>  offset ) const {
        FBOffset<::fb::OnOffState::switching_on> typed_offset(
            offset.o );
        builder.add_switching_on( typed_offset );
      }

      template<typename ParentSrcT, typename ParentDstT>
      static ::rn::expect<bool> try_deserialize_me(
          ParentSrcT const* src,
          ParentDstT* dst ) {
        if( src->switching_on() == nullptr ) return false;
        *dst = switching_on{};
        XP_OR_RETURN_( deserialize(
            src->switching_on(), std::get_if<switching_on>( dst ),
            ::rn::serial::ADL{} ) );
        return true;
      }

      ::rn::expect<> check_invariants_safe() const {
        return ::rn::xp_success_t{};
      }

    };

    struct switching_off {
      double percent;
      // This requires that the types of the member variables
      // also support equality.
      bool operator==( struct switching_off const& ) const = default;
      bool operator!=( struct switching_off const& ) const = default;
      using fb_target_t = fb::OnOffState::switching_off;

      static std::string fb_root_type_name() {
        return "fb.OnOffState.switching_off";
      }

      FBOffset<fb::OnOffState::switching_off> serialize_table(
          FBBuilder& builder ) const {
        (void)fb_root_type_name;
        using ::rn::serial::serialize;
          
        auto s_percent = serialize<::rn::serial::fb_serialize_hint_t<
            decltype( std::declval<fb_target_t>().percent() )>>(
            builder, percent, ::rn::serial::ADL{} );

        return fb::OnOffState::Createswitching_off( builder
            , s_percent.get()
        );
      }

      static ::rn::expect<> deserialize_table(
          fb::OnOffState::switching_off const& src,
          switching_off* dst ) {
        (void)src;
        (void)dst;
        DCHECK( dst );
        using ::rn::serial::deserialize;
          
        XP_OR_RETURN_( deserialize(
            ::rn::serial::detail::to_const_ptr( src.percent() ),
            &dst->percent, ::rn::serial::ADL{} ) );

        return ::rn::xp_success_t{};
      }

      template<typename ParentBuilderT>
      void builder_add_me( ParentBuilderT& builder,
                           FBOffset<void>  offset ) const {
        FBOffset<::fb::OnOffState::switching_off> typed_offset(
            offset.o );
        builder.add_switching_off( typed_offset );
      }

      template<typename ParentSrcT, typename ParentDstT>
      static ::rn::expect<bool> try_deserialize_me(
          ParentSrcT const* src,
          ParentDstT* dst ) {
        if( src->switching_off() == nullptr ) return false;
        *dst = switching_off{};
        XP_OR_RETURN_( deserialize(
            src->switching_off(), std::get_if<switching_off>( dst ),
            ::rn::serial::ADL{} ) );
        return true;
      }

      ::rn::expect<> check_invariants_safe() const {
        return ::rn::xp_success_t{};
      }

    };

    enum class e {
      off,
      on,
      switching_on,
      switching_off,
    };

  } // namespace OnOffState

  using OnOffState_t = std::variant<
    OnOffState::off,
    OnOffState::on,
    OnOffState::switching_on,
    OnOffState::switching_off
  >;
  NOTHROW_MOVE( OnOffState_t );

} // namespace rn

// This gives us the enum to use in a switch statement.
template<>
struct rn::SumtypeToEnum<rn::OnOffState_t> {
  using type = rn::OnOffState::e;
};

// rn::OnOffState::off
template<>
struct fmt::formatter<rn::OnOffState::off>
  : formatter_base {
  template<typename Context>
  auto format( rn::OnOffState::off const&, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "OnOffState::off"
       ), ctx );
  }
};

// rn::OnOffState::on
template<>
struct fmt::formatter<rn::OnOffState::on>
  : formatter_base {
  template<typename Context>
  auto format( rn::OnOffState::on const& o, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "OnOffState::on{{"
        "user={}"
      "}}"
      , o.user ), ctx );
  }
};

// rn::OnOffState::switching_on
template<>
struct fmt::formatter<rn::OnOffState::switching_on>
  : formatter_base {
  template<typename Context>
  auto format( rn::OnOffState::switching_on const& o, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "OnOffState::switching_on{{"
        "percent={}"
      "}}"
      , o.percent ), ctx );
  }
};

// rn::OnOffState::switching_off
template<>
struct fmt::formatter<rn::OnOffState::switching_off>
  : formatter_base {
  template<typename Context>
  auto format( rn::OnOffState::switching_off const& o, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "OnOffState::switching_off{{"
        "percent={}"
      "}}"
      , o.percent ), ctx );
  }
};

namespace rn::serial {

  template<typename Hint>
  auto serialize( FBBuilder& fbb, ::rn::OnOffState_t const& o,
                  ::rn::serial::ADL ) {
    auto offset  = util::visit( o, [&]( auto const& v ) {
      // Call Union() to make the offset templated on type `void`
      // instead of the type of this variant member so that we have
      // a consistent return type.
      return v.serialize_table( fbb ).Union();
    } );
    auto builder = fb::OnOffState_tBuilder( fbb );
    util::visit( o, [&]( auto const& v ) {
      v.builder_add_me( builder, offset );
    } );
    return ::rn::serial::ReturnValue{ builder.Finish() };
  }

  expect<> inline deserialize( fb::OnOffState_t const* src,
                               ::rn::OnOffState_t* dst,
                               ::rn::serial::ADL ) {
    if( src == nullptr ) return ::rn::xp_success_t{};
    int            count  = 0;
    ::rn::expect<> result = ::rn::xp_success_t{};
    ::rn::try_deserialize_variant_types<::rn::OnOffState_t>(
        [&]( auto const* p ) {
          if( !result ) return;
          using type =
              std::decay_t<std::remove_pointer_t<decltype( p )>>;
          auto xp = type::try_deserialize_me( src, dst );
          if( !xp ) {
            result = UNEXPECTED( "{}", xp.error().what );
            return;
          }
          auto deserialized = *xp;
          if( deserialized ) ++count;
        } );
    if( !result ) return result;
    if( count != 1 )
      return UNEXPECTED(
          "failed to deserialized precisely one variant element "
          "(found {})",
          count );
    return ::rn::xp_success_t{};
  }

} // namespace rn::serial

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

  using OnOffEvent_t = std::variant<
    OnOffEvent::turn_off,
    OnOffEvent::turn_on
  >;
  NOTHROW_MOVE( OnOffEvent_t );

} // namespace rn

// This gives us the enum to use in a switch statement.
template<>
struct rn::SumtypeToEnum<rn::OnOffEvent_t> {
  using type = rn::OnOffEvent::e;
};

// rn::OnOffEvent::turn_off
template<>
struct fmt::formatter<rn::OnOffEvent::turn_off>
  : formatter_base {
  template<typename Context>
  auto format( rn::OnOffEvent::turn_off const&, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "OnOffEvent::turn_off"
       ), ctx );
  }
};

// rn::OnOffEvent::turn_on
template<>
struct fmt::formatter<rn::OnOffEvent::turn_on>
  : formatter_base {
  template<typename Context>
  auto format( rn::OnOffEvent::turn_on const&, Context& ctx ) {
    return formatter_base::format( fmt::format(
      "OnOffEvent::turn_on"
       ), ctx );
  }
};
