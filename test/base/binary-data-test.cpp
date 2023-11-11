/****************************************************************
**binary-data-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-03.
*
* Description: Unit tests for the base/binary-data module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/binary-data.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace base {
namespace {

using namespace std;

/****************************************************************
** Helpers.
*****************************************************************/
fs::path output_folder() {
  error_code ec  = {};
  fs::path   res = fs::temp_directory_path( ec );
  BASE_CHECK( ec.value() == 0,
              "failed to get temp folder path." );
  return res;
}

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[base/binary-data] IBinaryIO [write] [builtin]" ) {
  array<unsigned char, 16> buffer   = {};
  array<unsigned char, 16> expected = {};

  MemBufferBinaryIO b( buffer );
  REQUIRE( b.size() == 16 );

  uint8_t const  i1 = 0x05;
  uint16_t const i2 = 0x80ff;
  uint32_t const i3 = 0x10002000;
  uint64_t const i4 = 0x1234567890987654;

  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 16 );
  REQUIRE( b.pos() == 0 );
  expected = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  REQUIRE( buffer == expected );

  REQUIRE( write_binary( b, i1 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 15 );
  REQUIRE( b.pos() == 1 );
  expected = { 0x05, 0, 0, 0, 0, 0, 0, 0,
               0,    0, 0, 0, 0, 0, 0, 0 };
  REQUIRE( buffer == expected );

  REQUIRE( write_binary( b, i2 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 13 );
  REQUIRE( b.pos() == 3 );
  expected = { 0x05, 0xff, 0x80, 0, 0, 0, 0, 0,
               0,    0,    0,    0, 0, 0, 0, 0 };
  REQUIRE( buffer == expected );

  REQUIRE( write_binary( b, i3 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 9 );
  REQUIRE( b.pos() == 7 );
  expected = { 0x05, 0xff, 0x80, 0, 0x20, 0, 0x10, 0,
               0,    0,    0,    0, 0,    0, 0,    0 };
  REQUIRE( buffer == expected );

  REQUIRE( write_binary( b, i4 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 1 );
  REQUIRE( b.pos() == 15 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( write_binary( b, i4 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 1 );
  REQUIRE( b.pos() == 15 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( write_binary( b, i2 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 1 );
  REQUIRE( b.pos() == 15 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0 };
  REQUIRE( buffer == expected );

  REQUIRE( write_binary( b, i1 ) );
  REQUIRE( b.eof() );
  REQUIRE_FALSE( b.remaining() == 1 );
  REQUIRE( b.pos() == 16 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0x05 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( write_binary( b, i1 ) );
  REQUIRE( b.eof() );
  REQUIRE_FALSE( b.remaining() == 1 );
  REQUIRE( b.pos() == 16 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0x05 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( write_binary( b, i4 ) );
  REQUIRE( b.eof() );
  REQUIRE_FALSE( b.remaining() == 1 );
  REQUIRE( b.pos() == 16 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0x05 };
  REQUIRE( buffer == expected );
}

TEST_CASE( "[base/binary-data] IBinaryIO [write_bytes]" ) {
  array<unsigned char, 16> buffer   = {};
  array<unsigned char, 16> expected = {};

  MemBufferBinaryIO b( buffer );
  REQUIRE( b.size() == 16 );

  uint8_t const  i1 = 0x05;
  uint16_t const i2 = 0x80ff;
  uint32_t const i3 = 0x10002000;
  uint64_t const i4 = 0x1234567890987654;

  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 16 );
  REQUIRE( b.pos() == 0 );
  expected = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  REQUIRE( buffer == expected );

  REQUIRE( b.write_bytes<1>( i1 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 15 );
  REQUIRE( b.pos() == 1 );
  expected = { 0x05, 0, 0, 0, 0, 0, 0, 0,
               0,    0, 0, 0, 0, 0, 0, 0 };
  REQUIRE( buffer == expected );

  REQUIRE( b.write_bytes<2>( i2 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 13 );
  REQUIRE( b.pos() == 3 );
  expected = { 0x05, 0xff, 0x80, 0, 0, 0, 0, 0,
               0,    0,    0,    0, 0, 0, 0, 0 };
  REQUIRE( buffer == expected );

  REQUIRE( b.write_bytes<2>( i3 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 11 );
  REQUIRE( b.pos() == 5 );
  expected = { 0x05, 0xff, 0x80, 0, 0x20, 0, 0, 0,
               0,    0,    0,    0, 0,    0, 0, 0 };
  REQUIRE( buffer == expected );

  REQUIRE( b.write_bytes<2>( i3 >> 16 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 9 );
  REQUIRE( b.pos() == 7 );
  expected = { 0x05, 0xff, 0x80, 0, 0x20, 0, 0x10, 0,
               0,    0,    0,    0, 0,    0, 0,    0 };
  REQUIRE( buffer == expected );

  REQUIRE( b.write_bytes<7>( i4 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 2 );
  REQUIRE( b.pos() == 14 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0,    0 };
  REQUIRE( buffer == expected );

  REQUIRE( b.write_bytes<1>( i4 >> 56 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 1 );
  REQUIRE( b.pos() == 15 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( b.write_bytes<8>( i4 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 1 );
  REQUIRE( b.pos() == 15 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( b.write_bytes<2>( i2 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 1 );
  REQUIRE( b.pos() == 15 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0 };
  REQUIRE( buffer == expected );

  REQUIRE( b.write_bytes<1>( i1 ) );
  REQUIRE( b.eof() );
  REQUIRE_FALSE( b.remaining() == 1 );
  REQUIRE( b.pos() == 16 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0x05 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( b.write_bytes<1>( i1 ) );
  REQUIRE( b.eof() );
  REQUIRE_FALSE( b.remaining() == 1 );
  REQUIRE( b.pos() == 16 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0x05 };
  REQUIRE( buffer == expected );

  REQUIRE( b.write_bytes<0>( i1 ) );
  REQUIRE( b.eof() );
  REQUIRE_FALSE( b.remaining() == 1 );
  REQUIRE( b.pos() == 16 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0x05 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( b.write_bytes<8>( i4 ) );
  REQUIRE( b.eof() );
  REQUIRE_FALSE( b.remaining() == 1 );
  REQUIRE( b.pos() == 16 );
  expected = { 0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
               0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0x05 };
  REQUIRE( buffer == expected );
}

TEST_CASE(
    "[base/binary-data] IBinaryIO [write] [std::array]" ) {
  // These are not the arrays under test; we just happen to use a
  // std::array to represent the underlying binary buffer because
  // it supports operator==.
  array<unsigned char, 16 + 15> buffer   = {};
  array<unsigned char, 16 + 15> expected = {};

  MemBufferBinaryIO b( buffer );
  REQUIRE( b.size() == 31 );

  array<uint16_t, 8> i0 = { 1, 2, 3, 4, 5, 6, 7, 8 };
  array<uint64_t, 2> i1 = { 0x1234567801234567,
                            0xaaaaaaaaaaaaaaaa };
  array<uint32_t, 3> i2 = { 0xabcdef01, 0xbcdef987, 0xbbbbbbbb };
  array<uint8_t, 2>  i3 = { 0x34, 0x56 };

  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 31 );
  REQUIRE( b.pos() == 0 );
  expected = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  REQUIRE( buffer == expected );

  REQUIRE( write_binary( b, i0 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 15 );
  REQUIRE( b.pos() == 16 );
  expected = { 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0,
               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( write_binary( b, i1 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 7 );
  REQUIRE( b.pos() == 24 );
  expected = { 1,    0,    2,    0,    3,    0,    4,    0,
               5,    0,    6,    0,    7,    0,    8,    0,
               0x67, 0x45, 0x23, 0x01, 0x78, 0x56, 0x34, 0x12,
               0,    0,    0,    0,    0,    0,    0 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( write_binary( b, i2 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 3 );
  REQUIRE( b.pos() == 28 );
  expected = { 1,    0,    2,    0,    3,    0,    4,    0,
               5,    0,    6,    0,    7,    0,    8,    0,
               0x67, 0x45, 0x23, 0x01, 0x78, 0x56, 0x34, 0x12,
               0x01, 0xef, 0xcd, 0xab, 0,    0,    0 };
  REQUIRE( buffer == expected );

  REQUIRE( write_binary( b, i3 ) );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 1 );
  REQUIRE( b.pos() == 30 );
  expected = { 1,    0,    2,    0,    3,    0,    4,    0,
               5,    0,    6,    0,    7,    0,    8,    0,
               0x67, 0x45, 0x23, 0x01, 0x78, 0x56, 0x34, 0x12,
               0x01, 0xef, 0xcd, 0xab, 0x34, 0x56, 0 };
  REQUIRE( buffer == expected );

  REQUIRE_FALSE( write_binary( b, i3 ) );
  REQUIRE( b.eof() );
  REQUIRE( b.remaining() == 0 );
  REQUIRE( b.pos() == 31 );
  expected = { 1,    0,    2,    0,    3,    0,    4,    0,
               5,    0,    6,    0,    7,    0,    8,    0,
               0x67, 0x45, 0x23, 0x01, 0x78, 0x56, 0x34, 0x12,
               0x01, 0xef, 0xcd, 0xab, 0x34, 0x56, 0x34 };
  REQUIRE( buffer == expected );
}

TEST_CASE( "[base/binary-data] IBinaryIO [read] [builtin]" ) {
  array<unsigned char, 16> buffer = {
      0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
      0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0x06 };

  MemBufferBinaryIO b( buffer );
  REQUIRE( b.size() == 16 );

  uint8_t const  expected_i1 = 0x05;
  uint16_t const expected_i2 = 0x80ff;
  uint32_t const expected_i3 = 0x10002000;
  uint64_t const expected_i4 = 0x1234567890987654;

  uint8_t  i1 = 0;
  uint16_t i2 = 0;
  uint32_t i3 = 0;
  uint64_t i4 = 0;

  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 16 );
  REQUIRE( b.pos() == 0 );

  REQUIRE( i1 == 0 );
  REQUIRE( read_binary( b, i1 ) );
  REQUIRE( i1 == expected_i1 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 15 );
  REQUIRE( b.pos() == 1 );

  REQUIRE( i2 == 0 );
  REQUIRE( read_binary( b, i2 ) );
  REQUIRE( i2 == expected_i2 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 13 );
  REQUIRE( b.pos() == 3 );

  REQUIRE( i3 == 0 );
  REQUIRE( read_binary( b, i3 ) );
  REQUIRE( i3 == expected_i3 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 9 );
  REQUIRE( b.pos() == 7 );

  REQUIRE( i4 == 0 );
  REQUIRE( read_binary( b, i4 ) );
  REQUIRE( i4 == expected_i4 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 1 );
  REQUIRE( b.pos() == 15 );

  REQUIRE_FALSE( read_binary( b, i4 ) );
  REQUIRE( i4 == expected_i4 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 1 );
  REQUIRE( b.pos() == 15 );

  REQUIRE_FALSE( read_binary( b, i2 ) );
  REQUIRE( i2 == expected_i2 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 1 );
  REQUIRE( b.pos() == 15 );

  REQUIRE( i1 == expected_i1 );
  REQUIRE( read_binary( b, i1 ) );
  REQUIRE( i1 == 0x06 );
  REQUIRE( b.eof() );
  REQUIRE_FALSE( b.remaining() == 1 );
  REQUIRE( b.pos() == 16 );

  REQUIRE_FALSE( read_binary( b, i1 ) );
  REQUIRE( i1 == 0x06 );
  REQUIRE( b.eof() );
  REQUIRE_FALSE( b.remaining() == 1 );
  REQUIRE( b.pos() == 16 );

  REQUIRE_FALSE( read_binary( b, i4 ) );
  REQUIRE( i1 == 0x06 );
  REQUIRE( b.eof() );
  REQUIRE( b.pos() == 16 );
}

TEST_CASE( "[base/binary-data] IBinaryIO [read_bytes]" ) {
  array<unsigned char, 16> buffer = {
      0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
      0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0x06 };

  MemBufferBinaryIO b( buffer );
  REQUIRE( b.size() == 16 );

  uint8_t const  expected_i1 = 0x05;
  uint64_t const expected_i4 = 0x1234567890987654;

  uint8_t  i1 = 0;
  uint16_t i2 = 0;
  uint32_t i3 = 0;
  uint64_t i4 = 0;

  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 16 );
  REQUIRE( b.pos() == 0 );

  REQUIRE( i1 == 0 );
  REQUIRE( b.read_bytes<0>( i1 ) );
  REQUIRE( i1 == 0 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 16 );
  REQUIRE( b.pos() == 0 );

  REQUIRE( i1 == 0 );
  REQUIRE( b.read_bytes<1>( i1 ) );
  REQUIRE( i1 == expected_i1 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 15 );
  REQUIRE( b.pos() == 1 );

  REQUIRE( i2 == 0 );
  REQUIRE( b.read_bytes<1>( i2 ) );
  REQUIRE( i2 == 0xff );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 14 );
  REQUIRE( b.pos() == 2 );

  REQUIRE( b.read_bytes<1>( i2 ) );
  REQUIRE( i2 == 0x80 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 13 );
  REQUIRE( b.pos() == 3 );

  REQUIRE( i3 == 0 );
  REQUIRE( b.read_bytes<3>( i3 ) );
  REQUIRE( i3 == 0x00002000 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 10 );
  REQUIRE( b.pos() == 6 );

  REQUIRE( b.read_bytes<1>( i3 ) );
  REQUIRE( i3 == 0x00000010 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 9 );
  REQUIRE( b.pos() == 7 );

  REQUIRE( i4 == 0 );
  REQUIRE( b.read_bytes<8>( i4 ) );
  REQUIRE( i4 == expected_i4 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 1 );
  REQUIRE( b.pos() == 15 );

  REQUIRE_FALSE( b.read_bytes<8>( i4 ) );
  REQUIRE( i4 == 0 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 1 );
  REQUIRE( b.pos() == 15 );

  REQUIRE_FALSE( b.read_bytes<2>( i2 ) );
  REQUIRE( i2 == 0 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 1 );
  REQUIRE( b.pos() == 15 );

  REQUIRE( i1 == expected_i1 );
  REQUIRE( b.read_bytes<1>( i1 ) );
  REQUIRE( i1 == 0x06 );
  REQUIRE( b.eof() );
  REQUIRE_FALSE( b.remaining() == 1 );
  REQUIRE( b.pos() == 16 );

  REQUIRE_FALSE( b.read_bytes<1>( i1 ) );
  REQUIRE( i1 == 0 );
  REQUIRE( b.eof() );
  REQUIRE_FALSE( b.remaining() == 1 );
  REQUIRE( b.pos() == 16 );

  REQUIRE_FALSE( b.read_bytes<1>( i4 ) );
  REQUIRE( i1 == 0 );
  REQUIRE( b.eof() );
  REQUIRE_FALSE( b.remaining() == 1 );
  REQUIRE( b.pos() == 16 );
}

TEST_CASE( "[base/binary-data] IBinaryIO [read] [std::array]" ) {
  // These are not the arrays under test; we just happen to use a
  // std::array to represent the underlying binary buffer because
  // it supports operator==.
  array<unsigned char, 16> buffer = {
      0x05, 0xff, 0x80, 0,    0x20, 0,    0x10, 0x54,
      0x76, 0x98, 0x90, 0x78, 0x56, 0x34, 0x12, 0x06 };

  MemBufferBinaryIO b( buffer );
  REQUIRE( b.size() == 16 );

  array<uint16_t, 3> const expected_i0 = { 0xff05, 0x0080,
                                           0x0020 };
  array<uint32_t, 2> const expected_i1 = { 0x98765410,
                                           0x34567890 };
  array<uint8_t, 2> const  expected_i2 = { 0x12, 0x06 };

  array<uint16_t, 3> i0 = { 0, 0, 0 };
  array<uint32_t, 2> i1 = { 0, 0 };
  array<uint8_t, 2>  i2 = { 0, 0 };

  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 16 );
  REQUIRE( b.pos() == 0 );

  REQUIRE( read_binary( b, i0 ) );
  REQUIRE( i0 == expected_i0 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 10 );
  REQUIRE( b.pos() == 6 );

  REQUIRE( read_binary( b, i1 ) );
  REQUIRE( i1 == expected_i1 );
  REQUIRE_FALSE( b.eof() );
  REQUIRE( b.remaining() == 2 );
  REQUIRE( b.pos() == 14 );

  REQUIRE( read_binary( b, i2 ) );
  REQUIRE( i2 == expected_i2 );
  REQUIRE( b.eof() );
  REQUIRE( b.remaining() == 0 );
  REQUIRE( b.pos() == 16 );

  REQUIRE_FALSE( read_binary( b, i2 ) );
  REQUIRE( i2 == expected_i2 );
  REQUIRE( b.eof() );
  REQUIRE( b.remaining() == 0 );
  REQUIRE( b.pos() == 16 );
}

TEST_CASE(
    "[base/binary-data] IBinaryIO [read/write] [signed]" ) {
  array<unsigned char, 8> buffer = { 0xff, 0xff, 0x05, 0x80,
                                     0xff, 0xff, 0xff, 0xff };
  array<unsigned char, 8> expected;

  MemBufferBinaryIO b( buffer );
  REQUIRE( b.size() == 8 );

  SECTION( "read" ) {
    int16_t x1 = 0;
    REQUIRE( read_binary( b, x1 ) );
    REQUIRE( x1 == -1 );

    int8_t x2 = 0;
    REQUIRE( read_binary( b, x2 ) );
    REQUIRE( x2 == 5 );

    REQUIRE( read_binary( b, x2 ) );
    REQUIRE( x2 == -128 );

    int32_t x3 = 0;
    REQUIRE( read_binary( b, x3 ) );
    REQUIRE( x3 == -1 );
  }

  SECTION( "write" ) {
    int16_t x1 = -32768;
    REQUIRE( write_binary( b, x1 ) );
    expected = { 0x00, 0x80, 0x05, 0x80,
                 0xff, 0xff, 0xff, 0xff };

    int8_t x2 = -1;
    REQUIRE( write_binary( b, x2 ) );
    expected = { 0xff, 0xff, 0xff, 0x80,
                 0xff, 0xff, 0xff, 0xff };

    x2 = 1;
    REQUIRE( write_binary( b, x2 ) );
    expected = { 0xff, 0xff, 0xff, 0x01,
                 0xff, 0xff, 0xff, 0xff };

    int32_t x3 = -234567788;
    REQUIRE( write_binary( b, x3 ) );
    expected = { 0xff, 0xff, 0xff, 0x01,
                 0x94, 0xc7, 0x04, 0xf2 };
  }
}

TEST_CASE( "[base/binary-data] FileBinaryIO read" ) {
  expect<FileBinaryIO, string> const nonexistent =
      FileBinaryIO::open_for_rw_fail_on_nonexist(
          "does-not-exist-j89j9j" );
  REQUIRE(
      nonexistent ==
      "failed to open file \"does-not-exist-j89j9j\" for reading."s );

  auto bin_files = testing::data_dir() / "binary-files";

  UNWRAP_CHECK( five_numbers,
                FileBinaryIO::open_for_rw_fail_on_nonexist(
                    bin_files / "five-numbers.bin" ) );
  REQUIRE( !five_numbers.eof() );
  REQUIRE( five_numbers.pos() == 0 );
  REQUIRE( five_numbers.size() == 5 );
  REQUIRE( five_numbers.remaining() == 5 );

  uint16_t two_bytes = 0;
  REQUIRE( five_numbers.read( two_bytes ) );
  REQUIRE( two_bytes == 0x0301 );
  REQUIRE( !five_numbers.eof() );
  REQUIRE( five_numbers.pos() == 2 );
  REQUIRE( five_numbers.size() == 5 );
  REQUIRE( five_numbers.remaining() == 3 );

  vector<unsigned char> const v = five_numbers.read_remainder();
  REQUIRE( v == vector<unsigned char>{ 2, 4, 7 } );
  REQUIRE( five_numbers.eof() );
  REQUIRE( five_numbers.pos() == 5 );
  REQUIRE( five_numbers.size() == 5 );
  REQUIRE( five_numbers.remaining() == 0 );

  int8_t one_byte = 111;
  REQUIRE( !five_numbers.read( one_byte ) );
  REQUIRE( one_byte == 111 ); // unchanged due to failure.
  REQUIRE( five_numbers.eof() );
  REQUIRE( five_numbers.pos() == 5 );
  REQUIRE( five_numbers.size() == 5 );
  REQUIRE( five_numbers.remaining() == 0 );

  vector<unsigned char> const v2 = five_numbers.read_remainder();
  REQUIRE( v2.empty() );
  REQUIRE( five_numbers.eof() );
  REQUIRE( five_numbers.pos() == 5 );
  REQUIRE( five_numbers.size() == 5 );
  REQUIRE( five_numbers.remaining() == 0 );
}

TEST_CASE( "[base/binary-data] FileBinaryIO write" ) {
  fs::path const tmp  = output_folder();
  fs::path const file = tmp / "two-numbers.bin";

  {
    UNWRAP_CHECK(
        two_numbers,
        FileBinaryIO::open_for_rw_and_truncate( file ) );
    REQUIRE( two_numbers.eof() );
    REQUIRE( two_numbers.pos() == 0 );
    REQUIRE( two_numbers.size() == 0 );
    REQUIRE( two_numbers.remaining() == 0 );

    uint16_t const two_bytes = 0x3355;
    REQUIRE( two_numbers.write( two_bytes ) );
    REQUIRE( two_numbers.eof() );
    REQUIRE( two_numbers.pos() == 2 );
    REQUIRE( two_numbers.size() == 2 );
    REQUIRE( two_numbers.remaining() == 0 );
  }

  UNWRAP_CHECK(
      two_numbers,
      FileBinaryIO::open_for_rw_fail_on_nonexist( file ) );
  REQUIRE( !two_numbers.eof() );
  REQUIRE( two_numbers.pos() == 0 );
  REQUIRE( two_numbers.size() == 2 );
  REQUIRE( two_numbers.remaining() == 2 );

  uint16_t two_bytes = 0;
  REQUIRE( two_numbers.read( two_bytes ) );
  REQUIRE( two_bytes == 0x3355 );
  REQUIRE( two_numbers.eof() );
  REQUIRE( two_numbers.pos() == 2 );
  REQUIRE( two_numbers.size() == 2 );
  REQUIRE( two_numbers.remaining() == 0 );

  vector<unsigned char> const v = two_numbers.read_remainder();
  REQUIRE( v == vector<unsigned char>{} );
  REQUIRE( two_numbers.eof() );
  REQUIRE( two_numbers.pos() == 2 );
  REQUIRE( two_numbers.size() == 2 );
  REQUIRE( two_numbers.remaining() == 0 );
}

} // namespace
} // namespace base
