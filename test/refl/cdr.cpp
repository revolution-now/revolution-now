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

// rds
#include "rds/testing.rds.hpp"

// cdr
#include "src/cdr/ext-builtin.hpp"
#include "src/cdr/ext-std.hpp"

// base
#include "base/to-str.hpp"
#include "src/base/to-str-ext-std.hpp"
#include "src/base/variant.hpp"

// Must be last.
#include "test/catch-common.hpp"

using namespace ::std;
using namespace ::cdr::literals;

namespace rn {

base::valid_or<std::string> StructWithValidation::validate()
    const {
  if( yyy == double( xxx ) + 1.0 ) return base::valid;
  return "failed validation";
}

// TODO: can remove once reflected structs have automatic to_str
// overloads.
void to_str( StructWithValidation const& o, string& out,
             base::ADL_t ) {
  out += fmt::format( "StructWithValidation{{xxx={},yyy={}}}",
                      o.xxx, o.yyy );
}

} // namespace rn

namespace refl {

using ::cdr::converter;
using ::cdr::list;
using ::cdr::table;
using ::cdr::value;
using ::cdr::testing::conv_from_bt;

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

  base::valid_or<string> validate() const {
    static unordered_set<string> valid_states{ "PA", "VA", "CA",
                                               "MD" };
    RETURN_IF_FALSE( valid_states.contains( state ),
                     "invalid state {}.", state );
    return base::valid;
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
  using template_types = tuple<>;

  static constexpr tuple fields{
      refl::StructField{ "street_number",
                         &my_ns::Address::street_number,
                         offsetof( type, street_number ) },
      refl::StructField{ "state", &my_ns::Address::state,
                         offsetof( type, state ) },
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
  string                    name   = {};
  double                    height = {};
  bool                      male   = {};
  vector<Address>           houses = {};
  unordered_map<e_pet, int> pets   = {};

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
  using template_types = tuple<>;

  static constexpr tuple fields{
      refl::StructField{ "name", &my_ns::Person::name,
                         offsetof( type, name ) },
      refl::StructField{ "height", &my_ns::Person::height,
                         offsetof( type, height ) },
      refl::StructField{ "male", &my_ns::Person::male,
                         offsetof( type, male ) },
      refl::StructField{ "houses", &my_ns::Person::houses,
                         offsetof( type, houses ) },
      refl::StructField{ "pets", &my_ns::Person::pets,
                         offsetof( type, pets ) },
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
  static constexpr string_view refl_ns   = "my_ns";
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
  PersonWrapper                 self     = {};
  string                        updated  = {};
  unordered_map<string, Person> contacts = {};

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
  using template_types = tuple<>;

  static constexpr tuple fields{
      refl::StructField{ "self", &my_ns::Rolodex::self,
                         offsetof( type, self ) },
      refl::StructField{ "updated", &my_ns::Rolodex::updated,
                         offsetof( type, updated ) },
      refl::StructField{ "contacts", &my_ns::Rolodex::contacts,
                         offsetof( type, contacts ) },
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
                table{
                    "cat"_key  = 3,
                    "frog"_key = 6,
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
                        table{
                            "cat"_key = 7,
                            "dog"_key = 8,
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
                        table{
                            "dog"_key = 2,
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
                table{
                    "cat"_key  = 3,
                    "frog"_key = 6,
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
                        table{
                            "cat"_key = 7,
                            "dog"_key = 8,
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
                        table{
                            "dog"_key = 2,
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
                table{
                    "cat"_key  = 3,
                    "frog"_key = 6,
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
                        table{
                            "cat"_key = 7,
                            "dog"_key = 8,
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
                        table{
                            "dog"_key = 2,
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
    "name"_key = "",      "height"_key = 0.0,
    "male"_key = false,   "houses"_key = cdr::list{},
    "pets"_key = table{},
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
        table{
            "cat"_key = 7,
            "dog"_key = 8,
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
        table{
            "frog"_key = 10,
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

value const cdr_address1_invalid_state = table{
    "street_number"_key = 32,
    "state"_key         = "XX",
};

my_ns::Variant1 const variant1              = person1;
my_ns::Variant1 const variant1_default      = my_ns::Variant1{};
my_ns::Variant2 const variant2              = address1;
my_ns::Variant3 const variant3a             = address1;
my_ns::Variant3 const variant3b             = person2;
my_ns::Variant3 const variant3c             = native_rolodex_1;
my_ns::Variant3 const variant3d             = my_ns::Person{};
my_ns::Variant3 const variant3d_fst_default = my_ns::Address{};

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

cdr::table cdr_variant3d_no_default{
    "Person"_key = cdr::table{},
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
      "    \\-std::unordered_map<std::string, "
      "refl::my_ns::Person, std::hash...\n"
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
    // These are important tests: they test that alternatives be-
    // yond the first that have default values still get written
    // in the face of the above options, which they need to be to
    // tell us which alternative is selected. But it also tests
    // that the first alternative still need not be written if it
    // has its default value.
    REQUIRE( conv.to( variant1_default ) == cdr::table{} );
    REQUIRE( conv.to( variant3d ) == cdr_variant3d_no_default );
    REQUIRE( conv.to( variant3d_fst_default ) == cdr::table{} );
  }
  SECTION( "from_canonical" ) {
    REQUIRE(
        conv_from_bt<Variant1>( conv, cdr_variant1_empty ) ==
        variant1_default );
    REQUIRE( conv_from_bt<Variant3>(
                 conv, cdr_variant3d_no_default ) == variant3d );
  }
}

TEST_CASE( "[refl] struct validation" ) {
  using namespace ::refl::my_ns;
  converter conv;
  REQUIRE( conv.from<Address>( cdr_address1_invalid_state ) ==
           conv.err( "invalid state XX." ) );
}

TEST_CASE( "[refl] validation on from_canonical" ) {
  using namespace ::rn;
  SECTION( "has no validation method" ) {
    static_assert( !ValidatableStruct<MyStruct> );
    MyStruct my_struct{
        .xxx = 7,
        .yyy = 5.5,
        .zzz_map =
            {
                { "one", "two" },
            },
    };
    cdr::value cdr_my_struct = cdr::table{
        "xxx"_key = 7,
        "yyy"_key = 5.5,
        "zzz_map"_key =
            cdr::table{
                "one"_key = "two",
            },
    };
    converter conv;
    REQUIRE( conv_from_bt<MyStruct>( conv, cdr_my_struct ) ==
             my_struct );
  }
  SECTION( "has validation method and passes" ) {
    static_assert( ValidatableStruct<StructWithValidation> );
    StructWithValidation swv{
        .xxx = 7,
        .yyy = 8.0,
    };
    cdr::value cdr_swv = cdr::table{
        "xxx"_key = 7,
        "yyy"_key = 8.0,
    };
    converter conv;
    REQUIRE( conv_from_bt<StructWithValidation>(
                 conv, cdr_swv ) == swv );
  }
  SECTION( "has validation method and fails" ) {
    static_assert( ValidatableStruct<StructWithValidation> );
    static_assert( base::Show<StructWithValidation> );
    cdr::value cdr_swv = cdr::table{
        "xxx"_key = 7,
        "yyy"_key = 5.5,
    };
    converter conv;
    REQUIRE( conv.from<StructWithValidation>( cdr_swv ) ==
             conv.err( "failed validation" ) );
  }
}

} // namespace
} // namespace refl
