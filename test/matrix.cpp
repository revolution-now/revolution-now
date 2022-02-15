/****************************************************************
**matrix.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-14.
*
* Description: Unit tests for the src/matrix.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/matrix.hpp"

// refl
#include "refl/cdr.hpp"
#include "refl/to-str.hpp"

// cdr
#include "src/cdr/ext-builtin.hpp"
#include "src/cdr/ext-std.hpp"

// base
// #include "base/to-str.hpp"
// #include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace ::std;
using namespace ::cdr::literals;

using ::cdr::list;
using ::cdr::table;
using ::cdr::value;
using ::cdr::testing::conv_from_bt;

Matrix<int> m_empty( Delta{} );

value cdr_empty = table{
    "size"_key =
        table{
            "h"_key = 0,
            "w"_key = 0,
        },
    "data"_key = cdr::list{},
};

value cdr_no_data = table{
    "size"_key =
        table{
            "h"_key = 1,
            "w"_key = 1,
        },
    "data"_key = cdr::list{},
};

value cdr_inconsistent_size = table{
    "size"_key =
        table{
            "h"_key = 0,
            "w"_key = 0,
        },
    "data"_key =
        cdr::list{
            // clang-format off
            table{ "key"_key=table{ "x"_key=0, "y"_key=0, }, "val"_key=1 },
            // clang-format on
        },
};

value const cdr_2x4_missing_default_elem = table{
    "size"_key =
        table{
            "h"_key = 2,
            "w"_key = 4,
        },
    "data"_key =
        cdr::list{
            // clang-format off
        table{                                           "val"_key=1 },
        table{ "key"_key=table{ "x"_key=1,            }, "val"_key=2 },
        table{ "key"_key=table{ "x"_key=2,            }, "val"_key=3 },
        table{ "key"_key=table{ "x"_key=3,            }, "val"_key=4 },
        table{ "key"_key=table{            "y"_key=1, }, "val"_key=5 },
        // Intentially leave out x=1,y=1.
        table{ "key"_key=table{ "x"_key=2, "y"_key=1, }, "val"_key=7 },
        table{ "key"_key=table{ "x"_key=3, "y"_key=1, }, "val"_key=8 },
            // clang-format on
        },
};

value const cdr_2x4_with_default_elem = table{
    "size"_key =
        table{
            "h"_key = 2,
            "w"_key = 4,
        },
    "data"_key =
        cdr::list{
            // clang-format off
        table{ "key"_key=table{ "x"_key=0, "y"_key=0, }, "val"_key=1 },
        table{ "key"_key=table{ "x"_key=1, "y"_key=0, }, "val"_key=2 },
        table{ "key"_key=table{ "x"_key=2, "y"_key=0, }, "val"_key=3 },
        table{ "key"_key=table{ "x"_key=3, "y"_key=0, }, "val"_key=4 },
        table{ "key"_key=table{ "x"_key=0, "y"_key=1, }, "val"_key=5 },
        table{ "key"_key=table{ "x"_key=1, "y"_key=1, }, "val"_key=0 },
        table{ "key"_key=table{ "x"_key=2, "y"_key=1, }, "val"_key=7 },
        table{ "key"_key=table{ "x"_key=3, "y"_key=1, }, "val"_key=8 },
            // clang-format on
        },
};

Matrix<int> make_m_2x4() {
  Matrix<int> m_2x4( Delta{ 4_w, 2_h } );
  m_2x4[0_y][0_x] = 1;
  m_2x4[0_y][1_x] = 2;
  m_2x4[0_y][2_x] = 3;
  m_2x4[0_y][3_x] = 4;
  m_2x4[1_y][0_x] = 5;
  m_2x4[1_y][1_x] = 0;
  m_2x4[1_y][2_x] = 7;
  m_2x4[1_y][3_x] = 8;
  return m_2x4;
}

TEST_CASE( "[src/matrix] cdr/empty matrix" ) {
  cdr::converter conv;
  Matrix<int>    m_empty( Delta{} );
  SECTION( "to_canonical" ) {
    REQUIRE( conv.to( m_empty ) == cdr_empty );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( conv_from_bt<Matrix<int>>( conv, cdr_empty ) ==
             m_empty );
  }
}

TEST_CASE( "[src/matrix] cdr/include-defaults" ) {
  cdr::converter conv;
  Matrix<int>    m_2x4 = make_m_2x4();
  SECTION( "to_canonical" ) {
    REQUIRE( conv.to( m_2x4 ) == cdr_2x4_with_default_elem );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( conv_from_bt<Matrix<int>>(
                 conv, cdr_2x4_with_default_elem ) == m_2x4 );
  }
}

TEST_CASE( "[src/matrix] cdr/no-include-defaults" ) {
  cdr::converter conv( cdr::converter::options{
      .write_fields_with_default_value  = false,
      .default_construct_missing_fields = true,
  } );
  Matrix<int>    m_2x4 = make_m_2x4();
  SECTION( "to_canonical" ) {
    REQUIRE( conv.to( m_2x4 ) == cdr_2x4_missing_default_elem );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( conv_from_bt<Matrix<int>>(
                 conv, cdr_2x4_missing_default_elem ) == m_2x4 );
  }
}

TEST_CASE( "[src/matrix] cdr/no data" ) {
  cdr::converter conv;
  REQUIRE(
      conv.from<Matrix<int>>( cdr_no_data ) ==
      conv.err(
          "inconsistent sizes between 'size' field and 'data' "
          "field ('size' implies 1 while 'data' implies 0)." ) );
}

TEST_CASE( "[src/matrix] cdr/inconsistent data" ) {
  cdr::converter conv;
  REQUIRE(
      conv.from<Matrix<int>>( cdr_inconsistent_size ) ==
      conv.err(
          "serialized matrix has more coordinates in 'data' (1) "
          "then are allowed by the 'size' (0)." ) );
}

} // namespace
} // namespace rn
