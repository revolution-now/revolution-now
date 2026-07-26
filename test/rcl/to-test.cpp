/****************************************************************
**to-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-01-15.
*
* Description: Unit tests for the rcl/to module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rcl/to.hpp"

// Testing
#include "test/rds/testing.rds.hpp"

// refl
#include "src/refl/cdr.hpp"

// cdr
#include "src/cdr/ext-base.hpp"
#include "src/cdr/ext-builtin.hpp"
#include "src/cdr/ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rcl {
namespace {

using namespace std;

/****************************************************************
** Data.
*****************************************************************/
test::Top const kObject1;

test::Top const kObject2{
  .top1 =
      {
        test::Middle{
          .mid1 = true,
          .mid2 = {},
          .mid3 =
              {
                { test::e_numbers::two, "TWO" },
                { test::e_numbers::one, "ONE" },
                { test::e_numbers::four, "444" },
              },
        },
        test::Middle{
          .mid1 = false,
          .mid2 =
              {
                { "key1",
                  test::Leaf{
                    .leaf2 = 99.9,
                  } },
                { "key0",
                  test::Leaf{
                    .leaf3 = test::e_numbers::two,
                  } },
              },
          .mid3 =
              {
                { test::e_numbers::three, "THREE" },
              },
        },
      },
  .top2 =
      test::ThisOrThat::that{
        .leaf =
            test::Leaf{
              .leaf1 = 42,
              .leaf2 = 79.3,
              .leaf3 = test::e_numbers::four,
            },
        .name = "my name",
      },
  .top3 =
      test::Leaf{
        .leaf1 = 43,
        .leaf2 = 79.4,
        .leaf3 = test::e_numbers::one,
      },
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[rcl/to] to_rcl" ) {
  test::Top o;
  string expected;

  auto const f = [&] [[clang::noinline]] { return to_rcl( o ); };

  o        = kObject1;
  expected = R"(top1: []

top2.th1s {
  x: 0
  y: 0
}

top3: null
)";
  REQUIRE( f() == expected );

  o        = kObject2;
  expected = R"(top1: [
  {
    mid1: true
    mid2 {}
    mid3: [
      {
        key: two
        val: TWO
      },
      {
        key: one
        val: ONE
      },
      {
        key: four
        val: "444"
      },
    ]
  },
  {
    mid1: false
    mid2 {
      key0 {
        leaf1: 0
        leaf2: 0
        leaf3: two
      }
      key1 {
        leaf1: 0
        leaf2: 99.9
        leaf3: one
      }
    }
    mid3: [
      {
        key: three
        val: THREE
      },
    ]
  },
]

top2.that {
  leaf {
    leaf1: 42
    leaf2: 79.3
    leaf3: four
  }
  name: my name
}

top3 {
  leaf1: 43
  leaf2: 79.4
  leaf3: one
}
)";
  REQUIRE( f() == expected );
}

TEST_CASE( "[rcl/to] to_json" ) {
  test::Top o;
  string expected;

  auto const f = [&] [[clang::noinline]] {
    return to_json( o );
  };

  o        = kObject1;
  expected = R"({
  "top1": [],
  "top2": {
    "th1s": {
      "x": 0,
      "y": 0
    }
  },
  "top3": null
})";
  REQUIRE( f() == expected );

  o        = kObject2;
  expected = R"({
  "top1": [
    {
      "mid1": true,
      "mid2": {},
      "mid3": [
        {
          "key": "two",
          "val": "TWO"
        },
        {
          "key": "one",
          "val": "ONE"
        },
        {
          "key": "four",
          "val": "444"
        }
      ]
    },
    {
      "mid1": false,
      "mid2": {
        "key0": {
          "leaf1": 0,
          "leaf2": 0,
          "leaf3": "two"
        },
        "key1": {
          "leaf1": 0,
          "leaf2": 99.9,
          "leaf3": "one"
        }
      },
      "mid3": [
        {
          "key": "three",
          "val": "THREE"
        }
      ]
    }
  ],
  "top2": {
    "that": {
      "leaf": {
        "leaf1": 42,
        "leaf2": 79.3,
        "leaf3": "four"
      },
      "name": "my name"
    }
  },
  "top3": {
    "leaf1": 43,
    "leaf2": 79.4,
    "leaf3": "one"
  }
})";
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rcl
