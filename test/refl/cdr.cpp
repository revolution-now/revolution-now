/****************************************************************
**cdr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-03.
*
* Description: Unit tests for the src/refl/cdr.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/refl/cdr.hpp"

// cdr
#include "src/cdr/ext-builtin.hpp"
#include "src/cdr/ext-std.hpp"

// base
#include "base/to-str.hpp"
#include "src/base/to-str-ext-std.hpp"
#include "src/base/variant.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace refl {
namespace {

using namespace ::std;
using namespace ::cdr::literals;

using ::cdr::converter;
using ::cdr::list;
using ::cdr::table;
using ::cdr::value;
using ::cdr::testing::conv_from_bt;

} // namespace

/****************************************************************
** Address
*****************************************************************/
namespace my_ns {
namespace {

struct Address {
  int    street_number = {};
  string state         = {};

  friend void to_str( Address const& o, string& out,
                      base::ADL_t ) {
    out += fmt::format( "Address{{street_number={},state={}}}",
                        o.street_number, o.state );
  }

  bool operator==( Address const& ) const = default;
};

} // namespace
} // namespace my_ns

template<>
struct traits<my_ns::Address> {
  using type                        = ::refl::my_ns::Address;
  static constexpr type_kind   kind = type_kind::struct_kind;
  static constexpr string_view ns   = "my_ns";
  static constexpr string_view name = "Address";

  // Struct specific.
  static constexpr tuple<> template_types{};

  static constexpr tuple fields{
      refl::ReflectedStructField{
          "street_number", &my_ns::Address::street_number },
      refl::ReflectedStructField{ "state",
                                  &my_ns::Address::state },
  };
};

static_assert( ReflectedStruct<my_ns::Address> );
static_assert( cdr::Canonical<my_ns::Address> );
static_assert( base::Show<my_ns::Address> );

/****************************************************************
** e_pet
*****************************************************************/
namespace my_ns {
namespace {

enum class e_pet { cat, dog, frog };

void to_str( e_pet const& o, string& out, base::ADL_t ) {
  switch( o ) {
    case e_pet::cat: out += "cat"; break;
    case e_pet::dog: out += "dog"; break;
    case e_pet::frog: out += "frog"; break;
  }
}

} // namespace
} // namespace my_ns

template<>
struct traits<my_ns::e_pet> {
  using type                        = my_ns::e_pet;
  static constexpr type_kind   kind = type_kind::enum_kind;
  static constexpr string_view ns   = "my_ns";
  static constexpr string_view name = "e_pet";

  // Enum specific.
  static constexpr array<string_view, 3> value_names{
      "cat",
      "dog",
      "frog",
  };
};

static_assert( ReflectedEnum<my_ns::e_pet> );
static_assert( cdr::Canonical<my_ns::e_pet> );
static_assert( base::Show<my_ns::e_pet> );

/****************************************************************
** Person
*****************************************************************/
namespace my_ns {
namespace {

struct Person {
  string          name   = {};
  double          height = {};
  bool            male   = {};
  vector<Address> houses = {};
  map<e_pet, int> pets   = {};

  friend void to_str( Person const& o, string& out,
                      base::ADL_t ) {
    out += fmt::format(
        "Person{{name={},height={},male={},houses={},pets={}}}",
        o.name, o.height, o.male, o.houses, o.pets );
  }

  bool operator==( Person const& ) const = default;
};

} // namespace
} // namespace my_ns

template<>
struct traits<my_ns::Person> {
  using type                        = ::refl::my_ns::Person;
  static constexpr type_kind   kind = type_kind::struct_kind;
  static constexpr string_view ns   = "my_ns";
  static constexpr string_view name = "Person";

  // Struct specific.
  static constexpr tuple<> template_types{};

  static constexpr tuple fields{
      refl::ReflectedStructField{ "name", &my_ns::Person::name },
      refl::ReflectedStructField{ "height",
                                  &my_ns::Person::height },
      refl::ReflectedStructField{ "male", &my_ns::Person::male },
      refl::ReflectedStructField{ "houses",
                                  &my_ns::Person::houses },
      refl::ReflectedStructField{ "pets", &my_ns::Person::pets },
  };
};

static_assert( ReflectedStruct<my_ns::Person> );
static_assert( cdr::Canonical<my_ns::Person> );
static_assert( base::Show<my_ns::Person> );

/****************************************************************
** PersonWrapper
*****************************************************************/
namespace my_ns {
namespace {

struct PersonWrapper {
  PersonWrapper() = default;

  PersonWrapper( Person person )
    : wrapped( std::move( person ) ) {}

  // Implement refl::WrapsReflected.
  Person const&                refl() const { return wrapped; }
  static constexpr string_view refl_name = "PersonWrapper";

  // Implement base::Show.
  friend void to_str( PersonWrapper const& o, string& out,
                      base::ADL_t ) {
    to_str( o.wrapped, out, base::ADL_t{} );
  }

  bool operator==( PersonWrapper const& ) const = default;

  Person wrapped;
};

} // namespace
} // namespace my_ns

static_assert( WrapsReflected<my_ns::PersonWrapper> );
static_assert( cdr::Canonical<my_ns::PersonWrapper> );
static_assert( base::Show<my_ns::PersonWrapper> );

/****************************************************************
** Rolodex
*****************************************************************/
namespace my_ns {
namespace {

struct Rolodex {
  PersonWrapper       self     = {};
  string              updated  = {};
  map<string, Person> contacts = {};

  friend void to_str( Rolodex const& o, string& out,
                      base::ADL_t ) {
    out +=
        fmt::format( "Rolodex{{self={},updated={},contacts={}}}",
                     o.self, o.updated, o.contacts );
  }

  bool operator==( Rolodex const& ) const = default;
};

} // namespace
} // namespace my_ns

template<>
struct traits<my_ns::Rolodex> {
  using type                        = ::refl::my_ns::Rolodex;
  static constexpr type_kind   kind = type_kind::struct_kind;
  static constexpr string_view ns   = "my_ns";
  static constexpr string_view name = "Rolodex";

  // Struct specific.
  static constexpr tuple<> template_types{};

  static constexpr tuple fields{
      refl::ReflectedStructField{ "self",
                                  &my_ns::Rolodex::self },
      refl::ReflectedStructField{ "updated",
                                  &my_ns::Rolodex::updated },
      refl::ReflectedStructField{ "contacts",
                                  &my_ns::Rolodex::contacts },
  };
};

static_assert( ReflectedStruct<my_ns::Rolodex> );
static_assert( cdr::Canonical<my_ns::Rolodex> );
static_assert( base::Show<my_ns::Rolodex> );

/****************************************************************
** Variants
*****************************************************************/
namespace my_ns {
namespace {

using Variant1 = base::variant<Person>;

using Variant2 = base::variant<Person, Address>;

using Variant3 = base::variant<Address, Rolodex, Person>;

}
} // namespace my_ns

static_assert( cdr::Canonical<my_ns::Variant1> );
static_assert( base::Show<my_ns::Variant1> );

static_assert( cdr::Canonical<my_ns::Variant2> );
static_assert( base::Show<my_ns::Variant2> );

static_assert( cdr::Canonical<my_ns::Variant3> );
static_assert( base::Show<my_ns::Variant3> );

/****************************************************************
** Test Data
*****************************************************************/
value const cdr_rolodex_1 = table{
    "self"_key =
        table{
            "name"_key   = "bob",
            "height"_key = 5.5,
            "male"_key   = true,
            "houses"_key =
                list{
                    table{
                        "street_number"_key = 444,
                        "state"_key         = "CA",
                    },
                    table{
                        "street_number"_key = 555,
                        "state"_key         = "MD",
                    },
                },
            "pets"_key =
                list{
                    table{ { "key", "cat" }, { "val", 3 } },
                    table{ { "key", "frog" }, { "val", 6 } },
                },
        },
    "updated"_key = "1900-02-01",
    "contacts"_key =
        table{
            "joe"_key =
                table{
                    "name"_key   = "joe",
                    "height"_key = 7.5,
                    "male"_key   = false,
                    "houses"_key = list{},
                    "pets"_key =
                        list{
                            table{ { "key", "cat" },
                                   { "val", 7 } },
                            table{ { "key", "dog" },
                                   { "val", 8 } },
                        },
                },
            "moe"_key =
                table{
                    "name"_key   = "moe",
                    "height"_key = 8.5,
                    "male"_key   = true,
                    "houses"_key =
                        list{
                            table{
                                "street_number"_key = 666,
                                "state"_key         = "VA",
                            },
                        },
                    "pets"_key =
                        list{
                            table{ { "key", "dog" },
                                   { "val", 2 } },
                        },
                },
        },
};

value const cdr_rolodex_1_missing_houses = table{
    "self"_key =
        table{
            "name"_key   = "bob",
            "height"_key = 5.5,
            "male"_key   = true,
            "houses"_key =
                list{
                    table{
                        "street_number"_key = 444,
                        "state"_key         = "CA",
                    },
                    table{
                        "street_number"_key = 555,
                        "state"_key         = "MD",
                    },
                },
            "pets"_key =
                list{
                    table{ { "key", "cat" }, { "val", 3 } },
                    table{ { "key", "frog" }, { "val", 6 } },
                },
        },
    "updated"_key = "1900-02-01",
    "contacts"_key =
        table{
            "joe"_key =
                table{
                    "name"_key   = "joe",
                    "height"_key = 7.5,
                    "male"_key   = false,
                    "pets"_key =
                        list{
                            table{ { "key", "cat" },
                                   { "val", 7 } },
                            table{ { "key", "dog" },
                                   { "val", 8 } },
                        },
                },
            "moe"_key =
                table{
                    "name"_key   = "moe",
                    "height"_key = 8.5,
                    "male"_key   = true,
                    "houses"_key =
                        list{
                            table{
                                "street_number"_key = 666,
                                "state"_key         = "VA",
                            },
                        },
                    "pets"_key =
                        list{
                            table{ { "key", "dog" },
                                   { "val", 2 } },
                        },
                },
        },
};

value const cdr_rolodex_1_extra_field = table{
    "self"_key =
        table{
            "name"_key   = "bob",
            "height"_key = 5.5,
            "male"_key   = true,
            "houses"_key =
                list{
                    table{
                        "street_number"_key = 444,
                        "state"_key         = "CA",
                    },
                    table{
                        "street_number"_key = 555,
                        "state"_key         = "MD",
                    },
                },
            "pets"_key =
                list{
                    table{ { "key", "cat" }, { "val", 3 } },
                    table{ { "key", "frog" }, { "val", 6 } },
                },
            "xyz"_key = 5,
        },
    "updated"_key = "1900-02-01",
    "contacts"_key =
        table{
            "joe"_key =
                table{
                    "name"_key   = "joe",
                    "height"_key = 7.5,
                    "male"_key   = false,
                    "houses"_key = list{},
                    "pets"_key =
                        list{
                            table{ { "key", "cat" },
                                   { "val", 7 } },
                            table{ { "key", "dog" },
                                   { "val", 8 } },
                        },
                },
            "moe"_key =
                table{
                    "name"_key   = "moe",
                    "height"_key = 8.5,
                    "male"_key   = true,
                    "houses"_key =
                        list{
                            table{
                                "street_number"_key = 666,
                                "state"_key         = "VA",
                            },
                        },
                    "pets"_key =
                        list{
                            table{ { "key", "dog" },
                                   { "val", 2 } },
                        },
                },
        },
};

my_ns::Rolodex const native_rolodex_1{
    .self    = { {
           .name   = "bob",
           .height = 5.5,
           .male   = true,
           .houses =
            {
                {
                       .street_number = 444,
                       .state         = "CA",
                },
                {
                       .street_number = 555,
                       .state         = "MD",
                },
            },
           .pets =
            {
                { my_ns::e_pet::cat, 3 },
                { my_ns::e_pet::frog, 6 },
            },
    } },
    .updated = "1900-02-01",
    .contacts =
        {
            {
                "joe",
                {
                    .name   = "joe",
                    .height = 7.5,
                    .male   = false,
                    .houses = {},
                    .pets =
                        {
                            { my_ns::e_pet::cat, 7 },
                            { my_ns::e_pet::dog, 8 },
                        },
                },
            },
            {
                "moe",
                {
                    .name   = "moe",
                    .height = 8.5,
                    .male   = true,
                    .houses =
                        {
                            {
                                .street_number = 666,
                                .state         = "VA",
                            },
                        },
                    .pets =
                        {
                            { my_ns::e_pet::dog, 2 },
                        },
                },
            },
        },
};

my_ns::Person const person_default{};

value const cdr_person_default = table{
    "name"_key = "",     "height"_key = 0.0,
    "male"_key = false,  "houses"_key = cdr::list{},
    "pets"_key = list{},
};

my_ns::Person const person1{
    .name   = "joe",
    .height = 7.5,
    .male   = false,
    .houses = {},
    .pets =
        {
            { my_ns::e_pet::cat, 7 },
            { my_ns::e_pet::dog, 8 },
        },
};

value const cdr_person1 = table{
    "name"_key   = "joe",
    "height"_key = 7.5,
    "male"_key   = false,
    "houses"_key = cdr::list{},
    "pets"_key =
        list{
            table{ { "key", "cat" }, { "val", 7 } },
            table{ { "key", "dog" }, { "val", 8 } },
        },
};

my_ns::Person const person2{
    .name   = "moe",
    .height = 8.5,
    .male   = true,
    .houses =
        {
            {
                .street_number = 666,
                .state         = "VA",
            },
        },
    .pets =
        {
            { my_ns::e_pet::frog, 10 },
        },
};

value const cdr_person2 = table{
    "name"_key   = "moe",
    "height"_key = 8.5,
    "male"_key   = true,
    "houses"_key =
        cdr::list{
            table{
                "street_number"_key = 666,
                "state"_key         = "VA",
            },
        },
    "pets"_key =
        list{
            table{ { "key", "frog" }, { "val", 10 } },
        },
};

my_ns::Address const address1{
    .street_number = 32,
    .state         = "PA",
};

value const cdr_address1 = table{
    "street_number"_key = 32,
    "state"_key         = "PA",
};

my_ns::Variant1 const variant1         = person1;
my_ns::Variant1 const variant1_default = my_ns::Variant1{};
my_ns::Variant2 const variant2         = address1;
my_ns::Variant3 const variant3a        = address1;
my_ns::Variant3 const variant3b        = person2;
my_ns::Variant3 const variant3c        = native_rolodex_1;

cdr::table cdr_variant1{
    "Person"_key = cdr_person1,
};

cdr::table cdr_variant1_empty{};

cdr::table cdr_variant1_default{
    "Person"_key = cdr_person_default,
};

cdr::table cdr_variant2{
    "Address"_key = cdr_address1,
};

cdr::table cdr_variant3a{
    "Address"_key = cdr_address1,
};

cdr::table cdr_variant3b{
    "Person"_key = cdr_person2,
};

cdr::table cdr_variant3c{
    "Rolodex"_key = cdr_rolodex_1,
};

cdr::table cdr_variant3_no_fields{};

cdr::list cdr_variant3_non_table{};

cdr::table cdr_variant3_extra_field{
    "Rolodex"_key = cdr_rolodex_1,
    "Person"_key  = cdr_person1,
};

cdr::table cdr_variant3_extra_field_inner{
    "Rolodex"_key = cdr_rolodex_1_extra_field,
};

cdr::table cdr_variant3_unknown_field{
    "abc"_key = cdr_person1,
};

namespace {

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[refl] default options" ) {
  using namespace ::refl::my_ns;
  converter conv;
  REQUIRE( conv_from_bt<Rolodex>( conv, cdr_rolodex_1 ) ==
           native_rolodex_1 );
  REQUIRE( conv.to( native_rolodex_1 ) == cdr_rolodex_1 );
}

TEST_CASE( "[refl] missing field" ) {
  using namespace ::refl::my_ns;
  converter  conv;
  cdr::error expected = conv.err(
      "message: key 'houses' not found in table.\n"
      "frame trace (most recent frame last):\n"
      "---------------------------------------------------\n"
      "refl::my_ns::Rolodex\n"
      " \\-value for key 'contacts'\n"
      "    \\-std::map<std::string, refl::my_ns::Person, "
      "std::less<std::stri...\n"
      "       \\-value for key 'joe'\n"
      "          \\-refl::my_ns::Person\n"
      "             \\-value for key 'houses'\n"
      "---------------------------------------------------" );
  REQUIRE( conv_from_bt<Rolodex>(
               conv, cdr_rolodex_1_missing_houses ) ==
           expected );
}

TEST_CASE( "[refl] extra field" ) {
  using namespace ::refl::my_ns;
  converter  conv;
  cdr::error expected = conv.err( //
      "message: unrecognized key 'xyz' in table.\n"
      "frame trace (most recent frame last):\n"
      "---------------------------------------------------\n"
      "refl::my_ns::Rolodex\n"
      " \\-value for key 'self'\n"
      "    \\-refl::my_ns::PersonWrapper\n"
      "       \\-refl::my_ns::Person\n"
      "---------------------------------------------------" );
  REQUIRE( conv_from_bt<Rolodex>(
               conv, cdr_rolodex_1_extra_field ) == expected );
}

TEST_CASE( "[refl] enum" ) {
  using namespace ::refl::my_ns;
  converter conv;
  SECTION( "to_canonical" ) {
    REQUIRE( conv.to( e_pet::dog ) == "dog" );
    REQUIRE( conv.to( e_pet::cat ) == "cat" );
    REQUIRE( conv.to( e_pet::frog ) == "frog" );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( conv_from_bt<e_pet>( conv, "dog" ) == e_pet::dog );
    REQUIRE( conv_from_bt<e_pet>( conv, "cat" ) == e_pet::cat );
    REQUIRE( conv_from_bt<e_pet>( conv, "frog" ) ==
             e_pet::frog );
    REQUIRE( conv.from<string>( 5 ) ==
             conv.err( "expected type string, instead found "
                       "type integer." ) );
  }
}

TEST_CASE( "[refl] variant" ) {
  using namespace ::refl::my_ns;
  converter conv;
  SECTION( "to_canonical" ) {
    REQUIRE( conv.to( variant1 ) == cdr_variant1 );
    REQUIRE( conv.to( variant2 ) == cdr_variant2 );
    REQUIRE( conv.to( variant3a ) == cdr_variant3a );
    REQUIRE( conv.to( variant3b ) == cdr_variant3b );
    REQUIRE( conv.to( variant3c ) == cdr_variant3c );
    REQUIRE( conv.to( variant1_default ) ==
             cdr_variant1_default );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( conv_from_bt<Variant1>( conv, cdr_variant1 ) ==
             variant1 );
    REQUIRE( conv_from_bt<Variant2>( conv, cdr_variant2 ) ==
             variant2 );
    REQUIRE( conv_from_bt<Variant3>( conv, cdr_variant3a ) ==
             variant3a );
    REQUIRE( conv_from_bt<Variant3>( conv, cdr_variant3b ) ==
             variant3b );
    REQUIRE( conv_from_bt<Variant3>( conv, cdr_variant3c ) ==
             variant3c );
    REQUIRE( conv.from<Variant1>( cdr_variant1_empty ) ==
             conv.err( "expected table with 1 key(s), instead "
                       "found 0 key(s)." ) );

    cdr::error expected1 = conv.err( //
        "message: unrecognized key 'xyz' in table.\n"
        "frame trace (most recent frame last):\n"
        "---------------------------------------------------\n"
        "base::variant<refl::my_ns::Address, "
        "refl::my_ns::Rolodex, refl...\n"
        " \\-value for key 'Rolodex'\n"
        "    \\-refl::my_ns::Rolodex\n"
        "       \\-value for key 'self'\n"
        "          \\-refl::my_ns::PersonWrapper\n"
        "             \\-refl::my_ns::Person\n"
        "---------------------------------------------------" );
    REQUIRE( conv_from_bt<Variant3>(
                 conv, cdr_variant3_extra_field_inner ) ==
             expected1 );
    cdr::error expected2 = conv.err( //
        "message: unrecognized variant alternative 'abc'.\n"
        "frame trace (most recent frame last):\n"
        "---------------------------------------------------\n"
        "base::variant<refl::my_ns::Address, "
        "refl::my_ns::Rolodex, refl...\n"
        "---------------------------------------------------" );
    REQUIRE( conv_from_bt<Variant3>(
                 conv, cdr_variant3_unknown_field ) ==
             expected2 );
    cdr::error expected3 = conv.err( //
        "message: expected table with 1 key(s), instead found 2 "
        "key(s).\n"
        "frame trace (most recent frame last):\n"
        "---------------------------------------------------\n"
        "base::variant<refl::my_ns::Address, "
        "refl::my_ns::Rolodex, refl...\n"
        "---------------------------------------------------" );
    REQUIRE( conv_from_bt<Variant3>(
                 conv, cdr_variant3_extra_field ) == expected3 );
    cdr::error expected4 = conv.err( //
        "message: expected table with 1 key(s), instead found 0 "
        "key(s).\n"
        "frame trace (most recent frame last):\n"
        "---------------------------------------------------\n"
        "base::variant<refl::my_ns::Address, "
        "refl::my_ns::Rolodex, refl...\n"
        "---------------------------------------------------" );
    REQUIRE( conv_from_bt<Variant3>(
                 conv, cdr_variant3_no_fields ) == expected4 );
    REQUIRE(
        conv.from<Variant3>( cdr_variant3_non_table ) ==
        conv.err(
            "expected type table, instead found type list." ) );
  }
}

TEST_CASE( "[refl] variant/defaults" ) {
  using namespace ::refl::my_ns;
  converter conv{ {
      .write_fields_with_default_value  = false,
      .default_construct_missing_fields = true,
  } };
  SECTION( "to_canonical" ) {
    REQUIRE( conv.to( variant1_default ) == cdr_variant1_empty );
  }
  SECTION( "from_canonical" ) {
    REQUIRE(
        conv_from_bt<Variant1>( conv, cdr_variant1_empty ) ==
        variant1_default );
  }
}

} // namespace
} // namespace refl
