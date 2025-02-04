/****************************************************************
**image.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-11.
*
* Description: Unit tests for the src/gfx/image.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/gfx/image.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace gfx {
namespace {

using namespace std;

TEST_CASE( "[image] creation" ) {
  unsigned char* data = (unsigned char*)::malloc( 7 * 5 * 4 );
  ::memset( data, 0, 7 * 5 * 4 );
  data[7 * 1 * 4 + 0] = 1; // red
  data[7 * 1 * 4 + 1] = 2; // green
  data[7 * 1 * 4 + 2] = 3; // blue
  data[7 * 1 * 4 + 3] = 4; // alpha

  image img( size{ .w = 7, .h = 5 }, data );

  REQUIRE( img.size_pixels() == size{ .w = 7, .h = 5 } );
  REQUIRE( img.height_pixels() == 5 );
  REQUIRE( img.width_pixels() == 7 );
  REQUIRE( img.size_bytes() == 140 );
  REQUIRE( img.total_pixels() == 35 );

  static_assert( image::kBytesPerPixel == 4 );

  REQUIRE( img.data() == data );

  REQUIRE( img.at( point{ .x = 0, .y = 0 } ) ==
           pixel{ 0, 0, 0, 0 } );
  REQUIRE( img.at( point{ .x = 0, .y = 1 } ) ==
           pixel{ 1, 2, 3, 4 } );

  span<byte const> sb = img;
  REQUIRE( int( sb.size() ) == img.size_bytes() );
  REQUIRE( to_integer<int>( sb[0] ) == 0 );
  REQUIRE( to_integer<int>( sb[1] ) == 0 );
  REQUIRE( to_integer<int>( sb[2] ) == 0 );
  REQUIRE( to_integer<int>( sb[3] ) == 0 );
  REQUIRE( to_integer<int>( sb[7 * 1 * 4 + 0] ) == 1 );
  REQUIRE( to_integer<int>( sb[7 * 1 * 4 + 1] ) == 2 );
  REQUIRE( to_integer<int>( sb[7 * 1 * 4 + 2] ) == 3 );
  REQUIRE( to_integer<int>( sb[7 * 1 * 4 + 3] ) == 4 );

  span<char const> sc = img;
  REQUIRE( int( sc.size() ) == img.size_bytes() );
  REQUIRE( sc[0] == 0 );
  REQUIRE( sc[1] == 0 );
  REQUIRE( sc[2] == 0 );
  REQUIRE( sc[3] == 0 );
  REQUIRE( sc[7 * 1 * 4 + 0] == 1 );
  REQUIRE( sc[7 * 1 * 4 + 1] == 2 );
  REQUIRE( sc[7 * 1 * 4 + 2] == 3 );
  REQUIRE( sc[7 * 1 * 4 + 3] == 4 );

  span<unsigned char const> suc = img;
  REQUIRE( int( suc.size() ) == img.size_bytes() );
  REQUIRE( suc[0] == 0 );
  REQUIRE( suc[1] == 0 );
  REQUIRE( suc[2] == 0 );
  REQUIRE( suc[3] == 0 );
  REQUIRE( suc[7 * 1 * 4 + 0] == 1 );
  REQUIRE( suc[7 * 1 * 4 + 1] == 2 );
  REQUIRE( suc[7 * 1 * 4 + 2] == 3 );
  REQUIRE( suc[7 * 1 * 4 + 3] == 4 );

  span<pixel const> sp = img;
  REQUIRE( int( sp.size() ) == img.total_pixels() );
  REQUIRE( sp[0] == pixel{ 0, 0, 0, 0 } );
  REQUIRE( sp[7 * 1] == pixel{ 1, 2, 3, 4 } );
  REQUIRE( sp[7 * 1 + 1] == pixel{ 0, 0, 0, 0 } );
}

TEST_CASE( "[image] new_empty_image" ) {
  image img = new_empty_image( size{ .w = 7, .h = 5 } );

  REQUIRE( img.height_pixels() == 5 );
  REQUIRE( img.width_pixels() == 7 );
  REQUIRE( img.size_bytes() == 140 );
  REQUIRE( img.total_pixels() == 35 );

  REQUIRE( img.at( point{ .x = 0, .y = 0 } ) ==
           pixel{ 0, 0, 0, 0 } );
  REQUIRE( img.at( point{ .x = 0, .y = 1 } ) ==
           pixel{ 0, 0, 0, 0 } );

  span<byte const> sb = img;
  REQUIRE( int( sb.size() ) == img.size_bytes() );
  REQUIRE( to_integer<int>( sb[0] ) == 0 );
  REQUIRE( to_integer<int>( sb[1] ) == 0 );
  REQUIRE( to_integer<int>( sb[2] ) == 0 );
  REQUIRE( to_integer<int>( sb[3] ) == 0 );
  REQUIRE( to_integer<int>( sb[7 * 1 * 4 + 0] ) == 0 );
  REQUIRE( to_integer<int>( sb[7 * 1 * 4 + 1] ) == 0 );
  REQUIRE( to_integer<int>( sb[7 * 1 * 4 + 2] ) == 0 );
  REQUIRE( to_integer<int>( sb[7 * 1 * 4 + 3] ) == 0 );

  span<char const> sc = img;
  REQUIRE( int( sc.size() ) == img.size_bytes() );
  REQUIRE( sc[0] == 0 );
  REQUIRE( sc[1] == 0 );
  REQUIRE( sc[2] == 0 );
  REQUIRE( sc[3] == 0 );
  REQUIRE( sc[7 * 1 * 4 + 0] == 0 );
  REQUIRE( sc[7 * 1 * 4 + 1] == 0 );
  REQUIRE( sc[7 * 1 * 4 + 2] == 0 );
  REQUIRE( sc[7 * 1 * 4 + 3] == 0 );

  span<unsigned char const> suc = img;
  REQUIRE( int( suc.size() ) == img.size_bytes() );
  REQUIRE( suc[0] == 0 );
  REQUIRE( suc[1] == 0 );
  REQUIRE( suc[2] == 0 );
  REQUIRE( suc[3] == 0 );
  REQUIRE( suc[7 * 1 * 4 + 0] == 0 );
  REQUIRE( suc[7 * 1 * 4 + 1] == 0 );
  REQUIRE( suc[7 * 1 * 4 + 2] == 0 );
  REQUIRE( suc[7 * 1 * 4 + 3] == 0 );

  span<pixel const> sp = img;
  REQUIRE( int( sp.size() ) == img.total_pixels() );
  REQUIRE( sp[0] == pixel{ 0, 0, 0, 0 } );
  REQUIRE( sp[7 * 1] == pixel{ 0, 0, 0, 0 } );
  REQUIRE( sp[7 * 1 + 1] == pixel{ 0, 0, 0, 0 } );
}

TEST_CASE( "[image] new_filled_image" ) {
  static pixel const G =
      pixel{ .r = 0, .g = 255, .b = 0, .a = 255 };
  static pixel const O =
      pixel{ .r = 0, .g = 0, .b = 0, .a = 255 };

  image img1 = new_filled_image( { .w = 4, .h = 4 }, O );

  vector<pixel> expected_img1 = {
    O, O, O, O, //
    O, O, O, O, //
    O, O, O, O, //
    O, O, O, O, //
  };
  REQUIRE( testing::image_equals( img1, expected_img1 ) );

  image img2 = new_filled_image( { .w = 5, .h = 7 }, G );
  vector<pixel> expected_img2 = {
    G, G, G, G, G, //
    G, G, G, G, G, //
    G, G, G, G, G, //
    G, G, G, G, G, //
    G, G, G, G, G, //
    G, G, G, G, G, //
    G, G, G, G, G, //
  };
  REQUIRE( testing::image_equals( img2, expected_img2 ) );
}

TEST_CASE( "[image] blit_from" ) {
  static pixel const R =
      pixel{ .r = 255, .g = 0, .b = 0, .a = 255 };
  static pixel const G =
      pixel{ .r = 0, .g = 255, .b = 0, .a = 255 };
  static pixel const O =
      pixel{ .r = 0, .g = 0, .b = 0, .a = 255 };

  image dst = new_filled_image( { .w = 4, .h = 4 }, O );
  image src = new_filled_image( { .w = 5, .h = 7 }, G );

  vector<pixel> expected_dst, expected_src;

  src[{ .x = 2, .y = 1 }] = R;
  src[{ .x = 3, .y = 1 }] = R;
  src[{ .x = 4, .y = 1 }] = R;
  src[{ .x = 2, .y = 2 }] = R;
  src[{ .x = 3, .y = 2 }] = R;
  src[{ .x = 4, .y = 2 }] = R;
  src[{ .x = 2, .y = 3 }] = R;
  src[{ .x = 3, .y = 3 }] = R;
  src[{ .x = 4, .y = 3 }] = R;
  src[{ .x = 2, .y = 4 }] = R;
  src[{ .x = 3, .y = 4 }] = R;
  src[{ .x = 4, .y = 4 }] = R;

  expected_src = {
    G, G, G, G, G, //
    G, G, R, R, R, //
    G, G, R, R, R, //
    G, G, R, R, R, //
    G, G, R, R, R, //
    G, G, G, G, G, //
    G, G, G, G, G, //
  };
  REQUIRE( testing::image_equals( src, expected_src ) );

  dst.blit_from( src,
                 rect{ .origin = { .x = 2, .y = 1 },
                       .size   = { .w = 4, .h = 3 } },
                 point{ .x = -1, .y = 2 } );
  expected_dst = {
    O, O, O, O, //
    O, O, O, O, //
    R, R, O, O, //
    R, R, O, O, //
  };
  REQUIRE( testing::image_equals( dst, expected_dst ) );
}

TEST_CASE( "[image] find_trimmed_bounds_in" ) {
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
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = {} };
    expected = { .origin = { .x = 0, .y = 0 }, .size = {} };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );
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
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = {} };
    expected = { .origin = { .x = 0, .y = 0 }, .size = {} };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );
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
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = {} };
    expected = { .origin = { .x = 0, .y = 0 }, .size = {} };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 1, .y = 3 }, .size = {} };
    expected = { .origin = { .x = 1, .y = 3 }, .size = {} };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 1, .y = 3 },
                 .size   = { .w = 2, .h = 3 } };
    expected = { .origin = { .x = 2, .y = 4 }, .size = {} };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 6, .y = 4 },
                 .size   = { .w = 4, .h = 5 } };
    expected = { .origin = { .x = 8, .y = 6 }, .size = {} };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = img_sz };
    expected = { .origin = { .x = 6, .y = 5 }, .size = {} };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );
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
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = {} };
    expected = { .origin = { .x = 0, .y = 0 }, .size = {} };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 1, .y = 3 }, .size = {} };
    expected = { .origin = { .x = 1, .y = 3 }, .size = {} };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 1, .y = 3 },
                 .size   = { .w = 2, .h = 3 } };
    expected = { .origin = { .x = 2, .y = 4 }, .size = {} };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 6, .y = 4 },
                 .size   = { .w = 4, .h = 5 } };
    expected = { .origin = { .x = 7, .y = 5 },
                 .size   = { .w = 1, .h = 1 } };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );
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
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = {} };
    expected = { .origin = { .x = 0, .y = 0 }, .size = {} };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 1, .y = 3 }, .size = {} };
    expected = { .origin = { .x = 1, .y = 3 }, .size = {} };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 1, .y = 3 },
                 .size   = { .w = 2, .h = 3 } };
    expected = { .origin = { .x = 2, .y = 4 }, .size = {} };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 6, .y = 4 },
                 .size   = { .w = 4, .h = 5 } };
    expected = { .origin = { .x = 7, .y = 5 },
                 .size   = { .w = 1, .h = 1 } };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 4, .y = 4 },
                 .size   = { .w = 4, .h = 2 } };
    expected = { .origin = { .x = 4, .y = 4 },
                 .size   = { .w = 4, .h = 2 } };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 3, .y = 3 },
                 .size   = { .w = 6, .h = 4 } };
    expected = { .origin = { .x = 4, .y = 4 },
                 .size   = { .w = 4, .h = 2 } };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );
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
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = {} };
    expected = { .origin = { .x = 0, .y = 0 }, .size = {} };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 1, .y = 3 }, .size = {} };
    expected = { .origin = { .x = 1, .y = 3 }, .size = {} };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 1, .y = 3 },
                 .size   = { .w = 2, .h = 3 } };
    expected = { .origin = { .x = 2, .y = 4 }, .size = {} };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 6, .y = 4 },
                 .size   = { .w = 4, .h = 5 } };
    expected = { .origin = { .x = 7, .y = 5 },
                 .size   = { .w = 1, .h = 1 } };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 4, .y = 4 },
                 .size   = { .w = 4, .h = 2 } };
    expected = { .origin = { .x = 4, .y = 4 },
                 .size   = { .w = 4, .h = 2 } };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 3, .y = 3 },
                 .size   = { .w = 6, .h = 4 } };
    expected = { .origin = { .x = 4, .y = 4 },
                 .size   = { .w = 4, .h = 2 } };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );
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
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 0, .y = 0 }, .size = {} };
    expected = { .origin = { .x = 0, .y = 0 }, .size = {} };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 1, .y = 3 }, .size = {} };
    expected = { .origin = { .x = 1, .y = 3 }, .size = {} };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 1, .y = 3 },
                 .size   = { .w = 2, .h = 3 } };
    expected = { .origin = { .x = 2, .y = 4 }, .size = {} };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 6, .y = 4 },
                 .size   = { .w = 4, .h = 5 } };
    expected = { .origin = { .x = 6, .y = 4 },
                 .size   = { .w = 3, .h = 3 } };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 4, .y = 4 },
                 .size   = { .w = 4, .h = 2 } };
    expected = { .origin = { .x = 4, .y = 4 },
                 .size   = { .w = 4, .h = 2 } };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 3, .y = 3 },
                 .size   = { .w = 6, .h = 4 } };
    expected = { .origin = { .x = 3, .y = 3 },
                 .size   = { .w = 6, .h = 4 } };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );

    bounds   = { .origin = { .x = 3, .y = 3 },
                 .size   = { .w = 7, .h = 5 } };
    expected = { .origin = { .x = 3, .y = 3 },
                 .size   = { .w = 6, .h = 4 } };
    REQUIRE( img.find_trimmed_bounds_in( bounds ) == expected );
  }
}

} // namespace
} // namespace gfx
