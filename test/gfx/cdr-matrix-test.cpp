/****************************************************************
**cdr-matrix.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-22.
*
* Description: Unit tests for the src/gfx/cdr-matrix.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/gfx/cdr-matrix.hpp"

// refl
#include "src/refl/cdr.hpp"

// cdr
#include "src/cdr/ext-builtin.hpp"
#include "src/cdr/ext-std.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace gfx {
namespace {

using namespace ::std;
using namespace ::cdr::literals;

using ::cdr::list;
using ::cdr::table;
using ::cdr::value;
using ::cdr::testing::conv_from_bt;

/****************************************************************
** Data.
*****************************************************************/
value cdr_empty = table{
    "has_coords"_key = false,
    "size"_key =
        table{
            "h"_key = 0,
            "w"_key = 0,
        },
    "data"_key = cdr::list{},
};

value cdr_no_data = table{
    "has_coords"_key = false,
    "size"_key =
        table{
            "h"_key = 1,
            "w"_key = 1,
        },
    "data"_key = cdr::list{},
};

value cdr_inconsistent_size = table{
    "has_coords"_key = true,
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
    "has_coords"_key = true,
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
    "has_coords"_key = false,
    "size"_key =
        table{
            "h"_key = 2,
            "w"_key = 4,
        },
    "data"_key = cdr::list{ 1, 2, 3, 4, 5, 0, 7, 8 },
};

Matrix<int> m_empty( rn::Delta{} );

Matrix<int> make_m_2x4() {
  Matrix<int> m_2x4( rn::Delta{ .w = 4, .h = 2 } );
  m_2x4[0][0] = 1;
  m_2x4[0][1] = 2;
  m_2x4[0][2] = 3;
  m_2x4[0][3] = 4;
  m_2x4[1][0] = 5;
  m_2x4[1][1] = 0;
  m_2x4[1][2] = 7;
  m_2x4[1][3] = 8;
  return m_2x4;
}

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[gfx/cdr-matrix] cdr/empty matrix" ) {
  cdr::converter conv;
  Matrix<int>    m_empty( rn::Delta{} );
  SECTION( "to_canonical" ) {
    REQUIRE( conv.to( m_empty ) == cdr_empty );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( conv_from_bt<Matrix<int>>( conv, cdr_empty ) ==
             m_empty );
  }
}

TEST_CASE( "[gfx/cdr-matrix] cdr/include-defaults" ) {
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

TEST_CASE( "[gfx/cdr-matrix] cdr/no-include-defaults" ) {
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

TEST_CASE( "[gfx/cdr-matrix] cdr/no data" ) {
  cdr::converter conv;
  REQUIRE(
      conv.from<Matrix<int>>( cdr_no_data ) ==
      conv.err(
          "inconsistent sizes between 'size' field and 'data' "
          "field ('size' implies 1 while 'data' implies 0)." ) );
}

TEST_CASE( "[gfx/cdr-matrix] cdr/inconsistent data" ) {
  cdr::converter conv;
  REQUIRE(
      conv.from<Matrix<int>>( cdr_inconsistent_size ) ==
      conv.err(
          "serialized matrix has more coordinates in 'data' (1) "
          "then are allowed by the 'size' (0)." ) );
}

} // namespace
} // namespace gfx
