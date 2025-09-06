/****************************************************************
**image-analysis-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-08-17.
*
* Description: Unit tests for the gfx/image-analysis module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/gfx/image-analysis.hpp"

// gfx
#include "src/gfx/image.hpp"
#include "src/gfx/pixel.hpp"

// C++ standard library
#include <cstring>

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace gfx {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[gfx/image-analysis] find_trimmed_bounds_in" ) {
  static pixel const R =
      pixel{ .r = 255, .g = 0, .b = 0, .a = 255 };
  static pixel const B =
      pixel{ .r = 0, .g = 0, .b = 0, .a = 255 };
  static pixel const x = pixel{ .r = 0, .g = 0, .b = 0, .a = 0 };

  rect expected, bounds;
  vector<pixel> pixels;
  size img_sz;

  auto make_img = [&] {
    BASE_CHECK( img_sz.area() == int( pixels.size() ) );

    int const buf_sz = sizeof( pixel ) * pixels.size();
    unsigned char* const buf =
        (unsigned char*)::malloc( buf_sz );
    ::memcpy( buf, pixels.data(), buf_sz );
    return image( img_sz, buf );
  };

  SECTION( "small empty" ) {
    pixels = {
      // clang-format off
      // 0
         x, // 0
      // clang-format on
    };
    img_sz = { .w = 1, .h = 1 };
    BASE_CHECK( img_sz.area() == int( pixels.size() ) );
    image const img = make_img();

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = img_sz };
    expected = { .origin = { .x = 0, .y = 0 },
                 .size   = { .w = 0, .h = 0 } };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = {} };
    expected = { .origin = { .x = 0, .y = 0 }, .size = {} };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );
  }

  SECTION( "small full" ) {
    pixels = {
      // clang-format off
      // 0
         B, // 0
      // clang-format on
    };
    img_sz = { .w = 1, .h = 1 };
    BASE_CHECK( img_sz.area() == int( pixels.size() ) );
    image const img = make_img();

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = img_sz };
    expected = { .origin = { .x = 0, .y = 0 },
                 .size   = { .w = 1, .h = 1 } };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = {} };
    expected = { .origin = { .x = 0, .y = 0 }, .size = {} };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );
  }

  SECTION( "empty" ) {
    pixels = {
      // clang-format off
      // 0  1  2  3  4  5  6  7  8  9  a  b
         x, x, x, x, x, x, x, x, x, x, x, x, // 0
         x, x, x, x, x, x, x, x, x, x, x, x, // 1
         x, x, x, x, x, x, x, x, x, x, x, x, // 2
         x, x, x, x, x, x, x, x, x, x, x, x, // 3
         x, x, x, x, x, x, x, x, x, x, x, x, // 4
         x, x, x, x, x, x, x, x, x, x, x, x, // 5
         x, x, x, x, x, x, x, x, x, x, x, x, // 6
         x, x, x, x, x, x, x, x, x, x, x, x, // 7
         x, x, x, x, x, x, x, x, x, x, x, x, // 8
         x, x, x, x, x, x, x, x, x, x, x, x, // 9
      // clang-format on
    };
    img_sz = { .w = 12, .h = 10 };
    BASE_CHECK( img_sz.area() == int( pixels.size() ) );
    image const img = make_img();

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = img_sz };
    expected = { .origin = { .x = 6, .y = 5 },
                 .size   = { .w = 0, .h = 0 } };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = {} };
    expected = { .origin = { .x = 0, .y = 0 }, .size = {} };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 1, .y = 3 }, .size = {} };
    expected = { .origin = { .x = 1, .y = 3 }, .size = {} };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 1, .y = 3 },
                 .size   = { .w = 2, .h = 3 } };
    expected = { .origin = { .x = 2, .y = 4 }, .size = {} };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 6, .y = 4 },
                 .size   = { .w = 4, .h = 5 } };
    expected = { .origin = { .x = 8, .y = 6 }, .size = {} };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = img_sz };
    expected = { .origin = { .x = 6, .y = 5 }, .size = {} };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );
  }

  SECTION( "one" ) {
    pixels = {
      // clang-format off
      // 0  1  2  3  4  5  6  7  8  9  a  b
         x, x, x, x, x, x, x, x, x, x, x, x, // 0
         x, x, x, x, x, x, x, x, x, x, x, x, // 1
         x, x, x, x, x, x, x, x, x, x, x, x, // 2
         x, x, x, x, x, x, x, x, x, x, x, x, // 3
         x, x, x, x, x, x, x, x, x, x, x, x, // 4
         x, x, x, x, x, x, x, R, x, x, x, x, // 5
         x, x, x, x, x, x, x, x, x, x, x, x, // 6
         x, x, x, x, x, x, x, x, x, x, x, x, // 7
         x, x, x, x, x, x, x, x, x, x, x, x, // 8
         x, x, x, x, x, x, x, x, x, x, x, x, // 9
      // clang-format on
    };
    img_sz = { .w = 12, .h = 10 };
    BASE_CHECK( img_sz.area() == int( pixels.size() ) );
    image const img = make_img();

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = img_sz };
    expected = { .origin = { .x = 7, .y = 5 },
                 .size   = { .w = 1, .h = 1 } };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = {} };
    expected = { .origin = { .x = 0, .y = 0 }, .size = {} };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 1, .y = 3 }, .size = {} };
    expected = { .origin = { .x = 1, .y = 3 }, .size = {} };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 1, .y = 3 },
                 .size   = { .w = 2, .h = 3 } };
    expected = { .origin = { .x = 2, .y = 4 }, .size = {} };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 6, .y = 4 },
                 .size   = { .w = 4, .h = 5 } };
    expected = { .origin = { .x = 7, .y = 5 },
                 .size   = { .w = 1, .h = 1 } };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );
  }

  SECTION( "two" ) {
    pixels = {
      // clang-format off
      // 0  1  2  3  4  5  6  7  8  9  a  b
         x, x, x, x, x, x, x, x, x, x, x, x, // 0
         x, x, x, x, x, x, x, x, x, x, x, x, // 1
         x, x, x, x, x, x, x, x, x, x, x, x, // 2
         x, x, x, x, x, x, x, x, x, x, x, x, // 3
         x, x, x, x, R, x, x, x, x, x, x, x, // 4
         x, x, x, x, x, x, x, R, x, x, x, x, // 5
         x, x, x, x, x, x, x, x, x, x, x, x, // 6
         x, x, x, x, x, x, x, x, x, x, x, x, // 7
         x, x, x, x, x, x, x, x, x, x, x, x, // 8
         x, x, x, x, x, x, x, x, x, x, x, x, // 9
      // clang-format on
    };
    img_sz = { .w = 12, .h = 10 };
    BASE_CHECK( img_sz.area() == int( pixels.size() ) );
    image const img = make_img();

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = img_sz };
    expected = { .origin = { .x = 4, .y = 4 },
                 .size   = { .w = 4, .h = 2 } };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = {} };
    expected = { .origin = { .x = 0, .y = 0 }, .size = {} };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 1, .y = 3 }, .size = {} };
    expected = { .origin = { .x = 1, .y = 3 }, .size = {} };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 1, .y = 3 },
                 .size   = { .w = 2, .h = 3 } };
    expected = { .origin = { .x = 2, .y = 4 }, .size = {} };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 6, .y = 4 },
                 .size   = { .w = 4, .h = 5 } };
    expected = { .origin = { .x = 7, .y = 5 },
                 .size   = { .w = 1, .h = 1 } };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 4, .y = 4 },
                 .size   = { .w = 4, .h = 2 } };
    expected = { .origin = { .x = 4, .y = 4 },
                 .size   = { .w = 4, .h = 2 } };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 3, .y = 3 },
                 .size   = { .w = 6, .h = 4 } };
    expected = { .origin = { .x = 4, .y = 4 },
                 .size   = { .w = 4, .h = 2 } };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );
  }

  SECTION( "three" ) {
    pixels = {
      // clang-format off
      // 0  1  2  3  4  5  6  7  8  9  a  b
         x, x, x, x, x, x, x, x, x, x, x, x, // 0
         x, x, x, x, x, x, x, x, x, x, x, x, // 1
         x, x, x, x, x, x, x, x, x, x, x, x, // 2
         x, x, x, x, x, x, x, x, x, x, x, x, // 3
         x, x, x, x, R, x, x, x, x, x, x, x, // 4
         x, x, x, x, x, x, x, R, x, x, x, x, // 5
         x, x, x, x, x, x, x, x, x, x, x, x, // 6
         x, x, x, x, x, x, x, x, x, x, x, x, // 7
         x, x, x, x, x, x, x, x, x, x, x, x, // 8
         x, x, x, x, x, x, x, x, x, x, x, R, // 9
      // clang-format on
    };
    img_sz = { .w = 12, .h = 10 };
    BASE_CHECK( img_sz.area() == int( pixels.size() ) );
    image const img = make_img();

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = img_sz };
    expected = { .origin = { .x = 4, .y = 4 },
                 .size   = { .w = 8, .h = 6 } };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = {} };
    expected = { .origin = { .x = 0, .y = 0 }, .size = {} };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 1, .y = 3 }, .size = {} };
    expected = { .origin = { .x = 1, .y = 3 }, .size = {} };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 1, .y = 3 },
                 .size   = { .w = 2, .h = 3 } };
    expected = { .origin = { .x = 2, .y = 4 }, .size = {} };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 6, .y = 4 },
                 .size   = { .w = 4, .h = 5 } };
    expected = { .origin = { .x = 7, .y = 5 },
                 .size   = { .w = 1, .h = 1 } };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 4, .y = 4 },
                 .size   = { .w = 4, .h = 2 } };
    expected = { .origin = { .x = 4, .y = 4 },
                 .size   = { .w = 4, .h = 2 } };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 3, .y = 3 },
                 .size   = { .w = 6, .h = 4 } };
    expected = { .origin = { .x = 4, .y = 4 },
                 .size   = { .w = 4, .h = 2 } };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );
  }

  SECTION( "many" ) {
    pixels = {
      // clang-format off
      // 0  1  2  3  4  5  6  7  8  9  a  b
         x, x, x, x, x, x, x, x, x, x, x, x, // 0
         x, x, x, x, x, x, x, x, x, x, x, x, // 1
         x, x, x, R, R, R, R, R, R, x, x, x, // 2
         x, x, x, R, R, R, R, R, R, x, x, x, // 3
         x, x, x, R, R, R, R, R, R, x, x, x, // 4
         x, x, x, R, R, R, R, R, R, x, x, x, // 5
         x, x, x, R, R, R, R, R, R, x, x, x, // 6
         x, x, x, x, x, x, x, x, x, x, x, x, // 7
         x, x, x, x, x, x, x, x, x, x, x, x, // 8
         x, x, x, x, x, x, x, x, x, x, x, x, // 9
      // clang-format on
    };
    img_sz = { .w = 12, .h = 10 };
    BASE_CHECK( img_sz.area() == int( pixels.size() ) );
    image const img = make_img();

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = img_sz };
    expected = { .origin = { .x = 3, .y = 2 },
                 .size   = { .w = 6, .h = 5 } };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = {} };
    expected = { .origin = { .x = 0, .y = 0 }, .size = {} };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 1, .y = 3 }, .size = {} };
    expected = { .origin = { .x = 1, .y = 3 }, .size = {} };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 1, .y = 3 },
                 .size   = { .w = 2, .h = 3 } };
    expected = { .origin = { .x = 2, .y = 4 }, .size = {} };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 6, .y = 4 },
                 .size   = { .w = 4, .h = 5 } };
    expected = { .origin = { .x = 6, .y = 4 },
                 .size   = { .w = 3, .h = 3 } };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 4, .y = 4 },
                 .size   = { .w = 4, .h = 2 } };
    expected = { .origin = { .x = 4, .y = 4 },
                 .size   = { .w = 4, .h = 2 } };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 3, .y = 3 },
                 .size   = { .w = 6, .h = 4 } };
    expected = { .origin = { .x = 3, .y = 3 },
                 .size   = { .w = 6, .h = 4 } };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );

    bounds   = { .origin = { .x = 3, .y = 3 },
                 .size   = { .w = 7, .h = 5 } };
    expected = { .origin = { .x = 3, .y = 3 },
                 .size   = { .w = 6, .h = 4 } };
    REQUIRE( find_trimmed_bounds_in( img, bounds ) == expected );
  }
}

TEST_CASE( "[gfx/image-analysis] compute_burrowed_sprites" ) {
  static pixel const _ = pixel{ .r = 0, .g = 0, .b = 0, .a = 0 };
  static pixel const w =
      pixel{ .r = 255, .g = 255, .b = 255, .a = 255 };
  static pixel const y =
      pixel{ .r = 255, .g = 255, .b = 255, .a = 0 };

  // The three output stages.
  static pixel const a =
      pixel{ .r = 160, .g = 0, .b = 0, .a = 255 };
  static pixel const b =
      pixel{ .r = 130, .g = 0, .b = 0, .a = 255 };
  static pixel const c =
      pixel{ .r = 100, .g = 0, .b = 0, .a = 255 };

  vector<pixel> pixels;
  size img_sz;

  vector<rect> sprites;
  auto const f = [&] [[nodiscard]] [[clang::noinline]] (
                     image const& input ) {
    return compute_burrowed_sprites( input, sprites );
  };

  auto const make_img = [&] {
    BASE_CHECK( img_sz.area() == int( pixels.size() ) );
    int const buf_sz = sizeof( pixel ) * pixels.size();
    unsigned char* const buf =
        (unsigned char*)::malloc( buf_sz );
    ::memcpy( buf, pixels.data(), buf_sz );
    return image( img_sz, buf );
  };

  auto const print_img = [&]( image const& img ) {
    for( int y = 0; y < img_sz.h; ++y ) {
      for( int x = 0; x < img_sz.w; ++x ) {
        fmt::print( "{:<4} ", img[{ .x = x, .y = y }].r );
      }
      fmt::println( "" );
    }
  };

  pixels = {
    // clang-format off
    // 0  1  2  3  4  5  6  7  8  9  a  b
       _, w, w, w, _, _, _, _, w, w, w, w, // 0
       _, w, w, w, _, w, _, _, w, w, w, w, // 1
       _, w, w, w, _, _, _, _, w, w, w, w, // 2
       _, w, w, w, _, _, _, _, w, _, _, w, // 3
       _, w, w, w, _, _, _, _, _, _, _, _, // 4
       _, w, w, w, _, _, w, w, w, w, _, _, // 5
       _, _, _, _, _, _, w, _, _, w, _, _, // 6
       _, _, _, _, _, _, w, _, w, w, _, _, // 7
       _, y, y, _, _, _, w, w, w, w, _, _, // 8
       _, y, y, _, _, _, _, _, _, _, _, _, // 9
    // 0  1  2  3  4  5  6  7  8  9  a  b
    // clang-format on
  };
  img_sz                = { .w = 12, .h = 10 };
  image const input_img = make_img();

  sprites = {
    rect{ .origin = { .x = 1, .y = 0 },
          .size   = { .w = 3, .h = 10 } },
    rect{ .origin = { .x = 5, .y = 1 },
          .size   = { .w = 1, .h = 1 } },
    rect{ .origin = { .x = 5, .y = 2 },
          .size   = { .w = 1, .h = 1 } },
    rect{ .origin = { .x = 6, .y = 5 },
          .size   = { .w = 4, .h = 4 } },
    rect{ .origin = { .x = 8, .y = 0 },
          .size   = { .w = 4, .h = 4 } },
  };

  pixels = {
    // clang-format off
    // 0  1  2  3  4  5  6  7  8  9  a  b
       _, _, _, _, _, _, _, _, _, c, c, _, // 0
       _, _, _, _, _, a, _, _, c, b, b, c, // 1
       _, _, _, _, _, _, _, _, b, a, a, b, // 2
       _, c, c, c, _, _, _, _, a, _, _, a, // 3
       _, b, b, b, _, _, _, _, _, _, _, _, // 4
       _, a, a, a, _, _, _, _, _, _, _, _, // 5
       _, _, _, _, _, _, c, _, _, c, _, _, // 6
       _, _, _, _, _, _, b, _, b, b, _, _, // 7
       _, _, _, _, _, _, a, a, a, a, _, _, // 8
       _, _, _, _, _, _, _, _, _, _, _, _, // 9
    // 0  1  2  3  4  5  6  7  8  9  a  b
    // clang-format on
  };
  image const expected_img = make_img();

  image const output_img = f( input_img );

  if constexpr( false ) {
    fmt::println( "\nInput image:\n\n" );
    print_img( input_img );

    fmt::println( "\nExpected image:\n\n" );
    print_img( expected_img );

    fmt::println( "\nOutput image:\n\n" );
    print_img( output_img );
  }

  REQUIRE( output_img != input_img );
  REQUIRE( output_img == expected_img );
}

} // namespace
} // namespace gfx
