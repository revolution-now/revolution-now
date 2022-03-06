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

TEST_CASE( "[image] empty_image" ) {
  image img = empty_image( size{ .w = 7, .h = 5 } );

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

}

} // namespace
} // namespace gfx
