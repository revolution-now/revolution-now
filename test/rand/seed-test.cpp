/****************************************************************
**seed-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-01-01.
*
* Description: Unit tests for the rand/seed module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rand/seed.hpp"

// cdr
#include "src/cdr/converter.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rng {
namespace {

using namespace std;

using ::base::nothing;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[rand/seed] to_str" ) {
  seed const s1{
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x4df00076,
  };
  REQUIRE( base::to_str( s1 ) ==
           "4df00076fbe4a2766da636d66151c187" );

  // Starts with zeroes.
  seed const s2{
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x00f00076,
  };
  REQUIRE( base::to_str( s2 ) ==
           "00f00076fbe4a2766da636d66151c187" );

  // Starts with all zeroes.
  seed const s3{
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x00000000,
  };
  REQUIRE( base::to_str( s3 ) ==
           "00000000fbe4a2766da636d66151c187" );

  // All zeroes.
  seed const s4{
    .e1 = 0,
    .e2 = 0,
    .e3 = 0,
    .e4 = 0,
  };
  REQUIRE( base::to_str( s4 ) ==
           "00000000000000000000000000000000" );
}

TEST_CASE( "[rand/seed] seed::from_string" ) {
  auto const f =
      [] [[clang::noinline]] ( string_view const sv ) {
        return seed::from_string( sv );
      };

  REQUIRE( f( "4df00076fbe4a2766da636d66151c187" ) ==
           seed{
             .e1 = 0x6151c187,
             .e2 = 0x6da636d6,
             .e3 = 0xfbe4a276,
             .e4 = 0x4df00076,
           } );

  REQUIRE( f( "31c1981050f9b08e8fe3fda12fcb0d0c" ) ==
           seed{
             .e1 = 0x2fcb0d0c,
             .e2 = 0x8fe3fda1,
             .e3 = 0x50f9b08e,
             .e4 = 0x31c19810,
           } );

  REQUIRE( f( "7ab4375984fb5c6291898d52e46ac2cd" ) ==
           seed{
             .e1 = 0xe46ac2cd,
             .e2 = 0x91898d52,
             .e3 = 0x84fb5c62,
             .e4 = 0x7ab43759,
           } );

  REQUIRE( f( "b26bf80619002f58ddb33904f2bdab07" ) ==
           seed{
             .e1 = 0xf2bdab07,
             .e2 = 0xddb33904,
             .e3 = 0x19002f58,
             .e4 = 0xb26bf806,
           } );

  REQUIRE( f( "df61d386b9660235b4a723c187140475" ) ==
           seed{
             .e1 = 0x87140475,
             .e2 = 0xb4a723c1,
             .e3 = 0xb9660235,
             .e4 = 0xdf61d386,
           } );

  REQUIRE( f( "cc8846dfffedb6d0edd7e828bbc38bf2" ) ==
           seed{
             .e1 = 0xbbc38bf2,
             .e2 = 0xedd7e828,
             .e3 = 0xffedb6d0,
             .e4 = 0xcc8846df,
           } );

  REQUIRE( f( "4bb3f9459059ff13fbbe120892de3c09" ) ==
           seed{
             .e1 = 0x92de3c09,
             .e2 = 0xfbbe1208,
             .e3 = 0x9059ff13,
             .e4 = 0x4bb3f945,
           } );

  REQUIRE( f( "320e55050bc0f5673652472c4114dbe2" ) ==
           seed{
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
  REQUIRE( f( "00000000000000000000000000000000" ) == seed{
                                                        .e1 = 0,
                                                        .e2 = 0,
                                                        .e3 = 0,
                                                        .e4 = 0,
                                                      } );

  // Capitals.
  REQUIRE( f( "320E55050BC0F5673652472C4114DBE2" ) ==
           seed{
             .e1 = 0x4114dbe2,
             .e2 = 0x3652472c,
             .e3 = 0x0bc0f567,
             .e4 = 0x320e5505,
           } );

  // Mixed case.
  REQUIRE( f( "320E55050Bc0f5673652472C4114dBe2" ) ==
           seed{
             .e1 = 0x4114dbe2,
             .e2 = 0x3652472c,
             .e3 = 0x0bc0f567,
             .e4 = 0x320e5505,
           } );

  // Non-hex char.
  REQUIRE( f( "320e55050bc0f56x3652472c4114dbe2" ) == nothing );
}

TEST_CASE( "[rand/seed] mix" ) {
  seed s1{
    .e1 = 0x4114dbe2,
    .e2 = 0x3652472c,
    .e3 = 0x0bc0f567,
    .e4 = 0x320e5505,
  };
  seed s2 = s1;
  seed s3 = s1;
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

TEST_CASE( "[rand/seed] equality" ) {
  seed const s1{
    .e1 = 0x4114dbe2,
    .e2 = 0x3652472c,
    .e3 = 0x0bc0f567,
    .e4 = 0x320e5505,
  };
  seed const s2 = s1;
  seed s3       = s1;
  ++s3.e3;

  REQUIRE( s1 == s1 );
  REQUIRE( s1 == s2 );
  REQUIRE( s1 != s3 );

  REQUIRE( s2 == s2 );
  REQUIRE( s2 != s3 );

  REQUIRE( s3 == s3 );
}

TEST_CASE( "[rand/seed] to_canonical" ) {
  cdr::converter conv;

  seed const s1{
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x4df00076,
  };
  REQUIRE( conv.to( s1 ) == "4df00076fbe4a2766da636d66151c187" );

  // Starts with zeroes.
  seed const s2{
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x00f00076,
  };
  REQUIRE( conv.to( s2 ) == "00f00076fbe4a2766da636d66151c187" );

  // Starts with all zeroes.
  seed const s3{
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x00000000,
  };
  REQUIRE( conv.to( s3 ) == "00000000fbe4a2766da636d66151c187" );

  // All zeroes.
  seed const s4{
    .e1 = 0,
    .e2 = 0,
    .e3 = 0,
    .e4 = 0,
  };
  REQUIRE( conv.to( s4 ) == "00000000000000000000000000000000" );
}

TEST_CASE( "[rand/seed] from_canonical" ) {
  cdr::converter conv;

  REQUIRE(
      conv.from<seed>( cdr::table{} ) ==
      conv.err(
          "expected type string, instead found type table." ) );

  REQUIRE(
      conv.from<seed>( "" ) ==
      conv.err(
          "seed strings must be 32-character hex strings." ) );

  REQUIRE( conv.from<seed>(
               "00000000000000000000000000000000" ) == seed{} );

  REQUIRE(
      conv.from<seed>( "0000000000000000000000000000000x" ) ==
      conv.err( "seed strings must be 32-character hex strings "
                "with characters 0-9, a-f, A-F." ) );

  seed const s1{
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x00f00076,
  };
  REQUIRE( conv.from<seed>(
               "00f00076fbe4a2766da636d66151c187" ) == s1 );
}

} // namespace
} // namespace rng
