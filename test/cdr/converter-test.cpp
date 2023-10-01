/****************************************************************
**converter.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-29.
*
* Description: Unit tests for the src/cdr/converter.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/cdr/converter.hpp"

// cdr
#include "src/cdr/ext-builtin.hpp"
#include "src/cdr/ext-std.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace cdr {
namespace {

using namespace std;
using namespace ::cdr::literals;

using ::cdr::testing::conv_from_bt;

#define CONV_TO_FIELD( name ) conv.to_field( tbl, #name, o.name )

#define CONV_FROM_FIELD( name )                             \
  UNWRAP_RETURN(                                            \
      name, conv.from_field<                                \
                std::remove_cvref_t<decltype( res.name )>>( \
                tbl, #name, used_keys ) );                  \
  res.name = std::move( name )

/****************************************************************
** Address
*****************************************************************/
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

value to_canonical( converter& conv, Address const& o,
                    tag_t<Address> ) {
  table tbl;
  CONV_TO_FIELD( street_number );
  CONV_TO_FIELD( state );
  return tbl;
}

result<Address> from_canonical( converter& conv, value const& v,
                                tag_t<Address> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<table>( v ) );
  Address     res{};
  set<string> used_keys;
  CONV_FROM_FIELD( street_number );
  CONV_FROM_FIELD( state );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

static_assert( Canonical<Address> );
static_assert( base::Show<Address> );

/****************************************************************
** e_pet
*****************************************************************/
enum class e_pet { cat, dog, frog };

value to_canonical( converter&, e_pet const& o, tag_t<e_pet> ) {
  switch( o ) {
    case e_pet::cat:
      return "cat";
    case e_pet::dog:
      return "dog";
    case e_pet::frog:
      return "frog";
  }
}

result<e_pet> from_canonical( converter& conv, value const& v,
                              tag_t<e_pet> ) {
  UNWRAP_RETURN( str, conv.ensure_type<string>( v ) );
  if( str == "cat" ) return e_pet::cat;
  if( str == "dog" ) return e_pet::dog;
  if( str == "frog" ) return e_pet::frog;
  return conv.err( "unrecognized value for enum e_pet: {}",
                   str );
}

void to_str( e_pet const& o, string& out, base::ADL_t ) {
  switch( o ) {
    case e_pet::cat:
      out += "cat";
      break;
    case e_pet::dog:
      out += "dog";
      break;
    case e_pet::frog:
      out += "frog";
      break;
  }
}

static_assert( Canonical<e_pet> );
static_assert( base::Show<e_pet> );

/****************************************************************
** Person
*****************************************************************/
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

value to_canonical( converter& conv, Person const& o,
                    tag_t<Person> ) {
  table tbl;
  CONV_TO_FIELD( name );
  CONV_TO_FIELD( height );
  CONV_TO_FIELD( male );
  CONV_TO_FIELD( houses );
  CONV_TO_FIELD( pets );
  return tbl;
}

result<Person> from_canonical( converter& conv, value const& v,
                               tag_t<Person> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<table>( v ) );
  Person      res{};
  set<string> used_keys;
  CONV_FROM_FIELD( name );
  CONV_FROM_FIELD( height );
  CONV_FROM_FIELD( male );
  CONV_FROM_FIELD( houses );
  CONV_FROM_FIELD( pets );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

static_assert( Canonical<Person> );
static_assert( base::Show<Person> );

/****************************************************************
** Rolodex
*****************************************************************/
struct Rolodex {
  Person                        self     = {};
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

value to_canonical( converter& conv, Rolodex const& o,
                    tag_t<Rolodex> ) {
  table tbl;
  CONV_TO_FIELD( self );
  CONV_TO_FIELD( updated );
  CONV_TO_FIELD( contacts );
  return tbl;
}

result<Rolodex> from_canonical( converter& conv, value const& v,
                                tag_t<Rolodex> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<table>( v ) );
  Rolodex     res{};
  set<string> used_keys;
  CONV_FROM_FIELD( self );
  CONV_FROM_FIELD( updated );
  CONV_FROM_FIELD( contacts );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

static_assert( Canonical<Rolodex> );
static_assert( base::Show<Rolodex> );

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

// This one has no field values set if they assume their
// default-constructed values.
value const cdr_rolodex_1_no_def_fields = table{
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

// This one has some invalid/unrecognized fields.
value const cdr_rolodex_1_with_unrecognized = table{
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
                        "xyz"_key           = 1,
                    },
                },
            "pets"_key =
                table{
                    "cat"_key  = 3,
                    "frog"_key = 6,
                },
        },
    "updated"_key = "1900-02-01",
    "???"_key     = "hello",
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
                                   { "val", 7 },
                                   { "abc", 9 } },
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

Rolodex const native_rolodex_1{
    .self =
        {
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
                    { e_pet::cat, 3 },
                    { e_pet::frog, 6 },
                },
        },
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
                            { e_pet::cat, 7 },
                            { e_pet::dog, 8 },
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
                            { e_pet::dog, 2 },
                        },
                },
            },
        },
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[cdr/converter] default options" ) {
  converter::options opts{
      .write_fields_with_default_value  = true,
      .allow_unrecognized_fields        = false,
      .default_construct_missing_fields = false };
  // Sanity check to make sure the above are actually the de-
  // fault.
  REQUIRE( opts == converter::options{} );
  // Test roundtrip with default options.
  converter conv;
  REQUIRE( conv_from_bt<Rolodex>( conv, cdr_rolodex_1 ) ==
           native_rolodex_1 );
  REQUIRE( conv.from<Rolodex>( cdr_rolodex_1_no_def_fields ) ==
           conv.err( "key 'male' not found in table." ) );
  REQUIRE(
      conv.from<Rolodex>( cdr_rolodex_1_with_unrecognized ) ==
      conv.err( "unrecognized key 'xyz' in table." ) );
  REQUIRE( conv.to( native_rolodex_1 ) == cdr_rolodex_1 );
}

TEST_CASE( "[cdr/converter] no write def values" ) {
  converter::options opts{
      .write_fields_with_default_value  = false,
      .allow_unrecognized_fields        = false,
      .default_construct_missing_fields = false };
  converter conv( opts );
  REQUIRE( conv_from_bt<Rolodex>( conv, cdr_rolodex_1 ) ==
           native_rolodex_1 );
  REQUIRE( conv.from<Rolodex>( cdr_rolodex_1_no_def_fields ) ==
           conv.err( "key 'male' not found in table." ) );
  REQUIRE(
      conv.from<Rolodex>( cdr_rolodex_1_with_unrecognized ) ==
      conv.err( "unrecognized key 'xyz' in table." ) );
  REQUIRE( conv.to( native_rolodex_1 ) ==
           cdr_rolodex_1_no_def_fields );
}

TEST_CASE(
    "[cdr/converter] no write def values & default construct "
    "missing" ) {
  converter::options opts{
      .write_fields_with_default_value  = false,
      .allow_unrecognized_fields        = false,
      .default_construct_missing_fields = true };
  converter conv( opts );
  REQUIRE( conv_from_bt<Rolodex>( conv, cdr_rolodex_1 ) ==
           native_rolodex_1 );
  REQUIRE( conv_from_bt<Rolodex>(
               conv, cdr_rolodex_1_no_def_fields ) ==
           native_rolodex_1 );
  REQUIRE(
      conv.from<Rolodex>( cdr_rolodex_1_with_unrecognized ) ==
      conv.err( "unrecognized key 'xyz' in table." ) );
  REQUIRE( conv.to( native_rolodex_1 ) ==
           cdr_rolodex_1_no_def_fields );
}

TEST_CASE(
    "[cdr/converter] write def values & default construct "
    "missing" ) {
  converter::options opts{
      .write_fields_with_default_value  = true,
      .allow_unrecognized_fields        = false,
      .default_construct_missing_fields = true };
  converter conv( opts );
  REQUIRE( conv_from_bt<Rolodex>( conv, cdr_rolodex_1 ) ==
           native_rolodex_1 );
  REQUIRE( conv_from_bt<Rolodex>(
               conv, cdr_rolodex_1_no_def_fields ) ==
           native_rolodex_1 );
  REQUIRE(
      conv.from<Rolodex>( cdr_rolodex_1_with_unrecognized ) ==
      conv.err( "unrecognized key 'xyz' in table." ) );
  REQUIRE( conv.to( native_rolodex_1 ) == cdr_rolodex_1 );
}

TEST_CASE( "[cdr/converter] allow_unrecognized_fields" ) {
  converter::options opts{ .allow_unrecognized_fields = true };
  converter          conv( opts );
  REQUIRE( conv_from_bt<Rolodex>( conv, cdr_rolodex_1 ) ==
           native_rolodex_1 );
  REQUIRE( conv.from<Rolodex>( cdr_rolodex_1_no_def_fields ) ==
           conv.err( "key 'male' not found in table." ) );
  REQUIRE( conv_from_bt<Rolodex>(
               conv, cdr_rolodex_1_with_unrecognized ) ==
           native_rolodex_1 );
  REQUIRE( conv.to( native_rolodex_1 ) == cdr_rolodex_1 );
}

TEST_CASE( "[cdr/converter] backtrace" ) {
  converter conv;
  using M  = unordered_map<string, int>;
  value  v = list{ table{ { "key", "one" }, { "val", 1 } },
                  table{ { "key", "two" }, { "val", "2" } } };
  string expected =
      "failed to convert value of type string to int.\n"
      "frame trace (most recent frame last):\n"
      "---------------------------------------------------\n"
      "std::unordered_map<std::string, int, "
      "std::hash<std::string>, s...\n"
      " \\-index 1\n"
      "   \\-std::pair<std::string const, int>\n"
      "     \\-value for key 'val'\n"
      "       \\-int\n"
      "---------------------------------------------------";
  REQUIRE( run_conversion_from_canonical<M>( v ) ==
           conv.err( expected ) );
}

} // namespace
} // namespace cdr