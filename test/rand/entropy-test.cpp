/****************************************************************
**entropy-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-01-01.
*
* Description: Unit tests for the rand/entropy module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rand/entropy.hpp"

// Testing
#include "test/luapp/common.hpp"

// cdr
#include "src/cdr/converter.hpp"

// luapp
#include "types.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rng {
namespace {

using namespace std;

using ::base::nothing;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[rand/entropy] from_random_device" ) {
  auto const source1 = entropy::from_random_device();
  auto const source2 = entropy::from_random_device();

  // In theory these could be violated if by chance they happened
  // to be the same and/or zero, but the probability of that
  // should be extremely miniscule (1/2^128) if they are truly
  // random.
  REQUIRE( source1 != entropy{} );
  REQUIRE( source2 != entropy{} );
  REQUIRE( source1 != source2 );
}

TEST_CASE( "[rand/entropy] to_str" ) {
  entropy const s1{
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x4df00076,
  };
  REQUIRE( base::to_str( s1 ) ==
           "4df00076fbe4a2766da636d66151c187" );

  // Starts with zeroes.
  entropy const s2{
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x00f00076,
  };
  REQUIRE( base::to_str( s2 ) ==
           "00f00076fbe4a2766da636d66151c187" );

  // Starts with all zeroes.
  entropy const s3{
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x00000000,
  };
  REQUIRE( base::to_str( s3 ) ==
           "00000000fbe4a2766da636d66151c187" );

  // All zeroes.
  entropy const s4{
    .e1 = 0,
    .e2 = 0,
    .e3 = 0,
    .e4 = 0,
  };
  REQUIRE( base::to_str( s4 ) ==
           "00000000000000000000000000000000" );
}

TEST_CASE( "[rand/entropy] entropy::from_string" ) {
  auto const f =
      [] [[clang::noinline]] ( string_view const sv ) {
        return entropy::from_string( sv );
      };

  REQUIRE( f( "4df00076fbe4a2766da636d66151c187" ) ==
           entropy{
             .e1 = 0x6151c187,
             .e2 = 0x6da636d6,
             .e3 = 0xfbe4a276,
             .e4 = 0x4df00076,
           } );

  REQUIRE( f( "31c1981050f9b08e8fe3fda12fcb0d0c" ) ==
           entropy{
             .e1 = 0x2fcb0d0c,
             .e2 = 0x8fe3fda1,
             .e3 = 0x50f9b08e,
             .e4 = 0x31c19810,
           } );

  REQUIRE( f( "7ab4375984fb5c6291898d52e46ac2cd" ) ==
           entropy{
             .e1 = 0xe46ac2cd,
             .e2 = 0x91898d52,
             .e3 = 0x84fb5c62,
             .e4 = 0x7ab43759,
           } );

  REQUIRE( f( "b26bf80619002f58ddb33904f2bdab07" ) ==
           entropy{
             .e1 = 0xf2bdab07,
             .e2 = 0xddb33904,
             .e3 = 0x19002f58,
             .e4 = 0xb26bf806,
           } );

  REQUIRE( f( "df61d386b9660235b4a723c187140475" ) ==
           entropy{
             .e1 = 0x87140475,
             .e2 = 0xb4a723c1,
             .e3 = 0xb9660235,
             .e4 = 0xdf61d386,
           } );

  REQUIRE( f( "cc8846dfffedb6d0edd7e828bbc38bf2" ) ==
           entropy{
             .e1 = 0xbbc38bf2,
             .e2 = 0xedd7e828,
             .e3 = 0xffedb6d0,
             .e4 = 0xcc8846df,
           } );

  REQUIRE( f( "4bb3f9459059ff13fbbe120892de3c09" ) ==
           entropy{
             .e1 = 0x92de3c09,
             .e2 = 0xfbbe1208,
             .e3 = 0x9059ff13,
             .e4 = 0x4bb3f945,
           } );

  REQUIRE( f( "320e55050bc0f5673652472c4114dbe2" ) ==
           entropy{
             .e1 = 0x4114dbe2,
             .e2 = 0x3652472c,
             .e3 = 0x0bc0f567,
             .e4 = 0x320e5505,
           } );

  // No 0x prefix allowed.
  REQUIRE( f( "0x320e55050bc0f5673652472c4114dbe2" ) ==
           nothing );

  // No spaces at start allowed.
  REQUIRE( f( " 320e55050bc0f5673652472c4114dbe2" ) == nothing );
  // No spaces at end allowed.
  REQUIRE( f( "320e55050bc0f5673652472c4114dbe2 " ) == nothing );
  // No spaces in middle allowed.
  REQUIRE( f( "320e55050bc0f5673 652472c4114dbe2" ) == nothing );

  // Tricky... correct length but with space.
  REQUIRE( f( " 320e55050bc0f5673652472c4114dbe" ) == nothing );
  REQUIRE( f( "320e55050bc0f5673652472c4114dbe " ) == nothing );

  // Too long.
  REQUIRE( f( "320e55050bc0f5673652472c4114dbe21" ) == nothing );
  REQUIRE( f( "320e55050bc0f5673652472c4114dbe212" ) ==
           nothing );
  REQUIRE( f( "320e55050bc0f5673652472c4114dbe2123" ) ==
           nothing );

  // Too short.
  REQUIRE( f( "320e55050bc0f5673652472c4114dbe" ) == nothing );
  REQUIRE( f( "320e55050bc0f56" ) == nothing );
  REQUIRE( f( "320e550" ) == nothing );
  REQUIRE( f( "320" ) == nothing );
  REQUIRE( f( "30" ) == nothing );
  REQUIRE( f( "1" ) == nothing );
  REQUIRE( f( "0" ) == nothing );
  REQUIRE( f( "" ) == nothing );

  // Zero.
  REQUIRE( f( "00000000000000000000000000000000" ) == entropy{
                                                        .e1 = 0,
                                                        .e2 = 0,
                                                        .e3 = 0,
                                                        .e4 = 0,
                                                      } );

  // Capitals.
  REQUIRE( f( "320E55050BC0F5673652472C4114DBE2" ) ==
           entropy{
             .e1 = 0x4114dbe2,
             .e2 = 0x3652472c,
             .e3 = 0x0bc0f567,
             .e4 = 0x320e5505,
           } );

  // Mixed case.
  REQUIRE( f( "320E55050Bc0f5673652472C4114dBe2" ) ==
           entropy{
             .e1 = 0x4114dbe2,
             .e2 = 0x3652472c,
             .e3 = 0x0bc0f567,
             .e4 = 0x320e5505,
           } );

  // Non-hex char.
  REQUIRE( f( "320e55050bc0f56x3652472c4114dbe2" ) == nothing );
}

TEST_CASE( "[rand/entropy] mix" ) {
  entropy s1{
    .e1 = 0x4114dbe2,
    .e2 = 0x3652472c,
    .e3 = 0x0bc0f567,
    .e4 = 0x320e5505,
  };
  entropy s2 = s1;
  entropy s3 = s1;
  ++s3.e3;

  REQUIRE( base::to_str( s1 ) ==
           "320e55050bc0f5673652472c4114dbe2" );
  REQUIRE( base::to_str( s2 ) ==
           "320e55050bc0f5673652472c4114dbe2" );
  REQUIRE( base::to_str( s3 ) ==
           "320e55050bc0f5683652472c4114dbe2" );

  s1.mix();
  s2.mix();
  s3.mix();

  REQUIRE( base::to_str( s1 ) ==
           "0eb3a1c7894a1f11993c206b7efc5bca" );
  REQUIRE( base::to_str( s2 ) ==
           "0eb3a1c7894a1f11993c206b7efc5bca" );
  REQUIRE( base::to_str( s3 ) ==
           "c0bd1da88d8c8f6e53f40c7bb0f2e7a5" );

  s1.mix();
  s2.mix();
  s3.mix();

  REQUIRE( base::to_str( s1 ) ==
           "e87fcaef1f1620934ce5e74140a109a3" );
  REQUIRE( base::to_str( s2 ) ==
           "e87fcaef1f1620934ce5e74140a109a3" );
  REQUIRE( base::to_str( s3 ) ==
           "d3478ea9bf7a8b0cc8f4e27cb33f8ef5" );

  ++s1.e1;
  ++s2.e1;
  ++s3.e1;
  s1.mix();
  s2.mix();
  s3.mix();

  REQUIRE( base::to_str( s1 ) ==
           "492e461f3f6b755bec6192865ffcb46a" );
  REQUIRE( base::to_str( s2 ) ==
           "492e461f3f6b755bec6192865ffcb46a" );
  REQUIRE( base::to_str( s3 ) ==
           "6f2bd75473be42c4f283160cae49c348" );

  ++s2.e2;
  s1.mix();
  s2.mix();
  s3.mix();

  REQUIRE( base::to_str( s1 ) ==
           "d3b52594f480dd5a5a061560d0afbf37" );
  REQUIRE( base::to_str( s2 ) ==
           "501b7200b69f0357a9125f17111e36ae" );
  REQUIRE( base::to_str( s3 ) ==
           "b2e1e4f71a4c6822847fa499bb3dae53" );
}

TEST_CASE( "[rand/entropy] mixed" ) {
  entropy const s1{
    .e1 = 0x4114dbe2,
    .e2 = 0x3652472c,
    .e3 = 0x0bc0f567,
    .e4 = 0x320e5505,
  };

  REQUIRE( base::to_str( s1 ) ==
           "320e55050bc0f5673652472c4114dbe2" );

  entropy const s2 = s1.mixed();

  REQUIRE( base::to_str( s1 ) ==
           "320e55050bc0f5673652472c4114dbe2" );
  REQUIRE( base::to_str( s2 ) ==
           "0eb3a1c7894a1f11993c206b7efc5bca" );
}

TEST_CASE( "[rand/entropy] equality" ) {
  entropy const s1{
    .e1 = 0x4114dbe2,
    .e2 = 0x3652472c,
    .e3 = 0x0bc0f567,
    .e4 = 0x320e5505,
  };
  entropy const s2 = s1;
  entropy s3       = s1;
  ++s3.e3;

  REQUIRE( s1 == s1 );
  REQUIRE( s1 == s2 );
  REQUIRE( s1 != s3 );

  REQUIRE( s2 == s2 );
  REQUIRE( s2 != s3 );

  REQUIRE( s3 == s3 );
}

TEST_CASE( "[rand/entropy] to_canonical" ) {
  cdr::converter conv;

  entropy const s1{
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x4df00076,
  };
  REQUIRE( conv.to( s1 ) == "4df00076fbe4a2766da636d66151c187" );

  // Starts with zeroes.
  entropy const s2{
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x00f00076,
  };
  REQUIRE( conv.to( s2 ) == "00f00076fbe4a2766da636d66151c187" );

  // Starts with all zeroes.
  entropy const s3{
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x00000000,
  };
  REQUIRE( conv.to( s3 ) == "00000000fbe4a2766da636d66151c187" );

  // All zeroes.
  entropy const s4{
    .e1 = 0,
    .e2 = 0,
    .e3 = 0,
    .e4 = 0,
  };
  REQUIRE( conv.to( s4 ) == "00000000000000000000000000000000" );
}

TEST_CASE( "[rand/entropy] from_canonical" ) {
  cdr::converter conv;

  REQUIRE(
      conv.from<entropy>( cdr::table{} ) ==
      conv.err(
          "expected type string, instead found type table." ) );

  REQUIRE( conv.from<entropy>( "" ) ==
           conv.err( "hash/entropy strings must be 32-character "
                     "hex strings." ) );

  REQUIRE(
      conv.from<entropy>( "00000000000000000000000000000000" ) ==
      entropy{} );

  REQUIRE(
      conv.from<entropy>( "0000000000000000000000000000000x" ) ==
      conv.err( "hash/entropy strings must be 32-character hex "
                "strings with characters 0-9, a-f, A-F." ) );

  entropy const s1{
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x00f00076,
  };
  REQUIRE( conv.from<entropy>(
               "00f00076fbe4a2766da636d66151c187" ) == s1 );
}

TEST_CASE( "[rand/entropy] rotate_right_n_bytes" ) {
  entropy const e{
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x00f00076,
  };

  auto const f = [&] [[clang::noinline]] ( int const n ) {
    auto cpy = e;
    cpy.rotate_right_n_bytes( n );
    return cpy;
  };

  entropy expected;

  expected = e;
  REQUIRE( f( 0 ) == expected );

  expected = {
    .e1 = 0xd66151c1,
    .e2 = 0x766da636,
    .e3 = 0x76fbe4a2,
    .e4 = 0x8700f000,
  };
  REQUIRE( f( 1 ) == expected );

  expected = {
    .e1 = 0x36d66151,
    .e2 = 0xa2766da6,
    .e3 = 0x0076fbe4,
    .e4 = 0xc18700f0,
  };
  REQUIRE( f( 2 ) == expected );

  expected = {
    .e1 = 0xa636d661,
    .e2 = 0xe4a2766d,
    .e3 = 0xf00076fb,
    .e4 = 0x51c18700,
  };
  REQUIRE( f( 3 ) == expected );

  expected = {
    .e1 = 0x6da636d6,
    .e2 = 0xfbe4a276,
    .e3 = 0x00f00076,
    .e4 = 0x6151c187,
  };
  REQUIRE( f( 4 ) == expected );

  expected = {
    .e1 = 0xe4a2766d,
    .e2 = 0xf00076fb,
    .e3 = 0x51c18700,
    .e4 = 0xa636d661,
  };
  REQUIRE( f( 7 ) == expected );

  expected = {
    .e1 = 0x0076fbe4,
    .e2 = 0xc18700f0,
    .e3 = 0x36d66151,
    .e4 = 0xa2766da6,
  };
  REQUIRE( f( 10 ) == expected );

  expected = {
    .e1 = 0x8700f000,
    .e2 = 0xd66151c1,
    .e3 = 0x766da636,
    .e4 = 0x76fbe4a2,
  };
  REQUIRE( f( 13 ) == expected );

  expected = e;
  REQUIRE( f( 16 ) == expected );
}

TEST_CASE( "[rand/entropy] consume" ) {
  entropy const e{
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x00f00076,
  };

  entropy w = e;

  auto const f = [&]<typename T> [[clang::noinline]] (
                     T* const ) { return w.consume<T>(); };

  entropy expected_w;

  expected_w = {
    .e1 = 0xd66151c1,
    .e2 = 0x766da636,
    .e3 = 0x76fbe4a2,
    .e4 = 0x8700f000,
  };
  REQUIRE( f( (uint8_t*){} ) == 0x87 );
  REQUIRE( w == expected_w );

  expected_w = {
    .e1 = 0x76fbe4a2,
    .e2 = 0x8700f000,
    .e3 = 0xd66151c1,
    .e4 = 0x766da636,
  };
  REQUIRE( f( (uint64_t*){} ) == 0x766da636d66151c1 );
  REQUIRE( w == expected_w );

  expected_w = {
    .e1 = 0xf00076fb,
    .e2 = 0x51c18700,
    .e3 = 0xa636d661,
    .e4 = 0xe4a2766d,
  };
  REQUIRE( f( (uint16_t*){} ) == 0xe4a2 );
  REQUIRE( w == expected_w );
}

LUA_TEST_CASE( "[rand/entropy] traverse" ) {
  vector<uint32_t> v1;
  vector<string> v2;
  v1.reserve( 4 );
  v2.reserve( 4 );

  entropy const e{
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x00f00076,
  };

  auto const fn = [&]( int const& o, string_view const sv ) {
    v1.push_back( o );
    v2.push_back( string( sv ) );
  };

  traverse( e, fn, trv::tag<entropy> );

  REQUIRE( v1 == vector<uint32_t>{ 0x6151c187, 0x6da636d6,
                                   0xfbe4a276, 0x00f00076 } );
  REQUIRE( v2 == vector<string>{ "e1", "e2", "e3", "e4" } );
}

LUA_TEST_CASE( "[rand/entropy] lua bindings" ) {
  entropy const e{
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x00f00076,
  };

  lua::push( L, e );
  lua::push( L, 5 );
  lua::push( L, "hello" );

  REQUIRE( lua::get<entropy>( L, -1 ) ==
           lua::unexpected{ .msg = "failed to convert string "
                                   "'hello' to entropy type" } );

  REQUIRE( lua::get<entropy>( L, -2 ) ==
           lua::unexpected{
             .msg = "expected string but found type number" } );

  REQUIRE( lua::get<entropy>( L, -3 ) == e );

  lua::pop_stack( L, 3 );
}

} // namespace
} // namespace rng
