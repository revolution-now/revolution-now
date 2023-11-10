/****************************************************************
**emit.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-08.
*
* Description: Unit tests for the src/rcl/emit.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rcl/emit.hpp"

// rcl
#include "src/rcl/parse.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rcl {
namespace {

using namespace std;

static string const input = R"(
  a {
    c {
      f = 5
      g = true
      h = truedat
    }
  }

  z = {}
  zz.yy = {}

  b {
    s = 5
    t = 3.5
  }

  c {
    d.e {
      f {
        g {
          h {
            i = 9
            j = 10
          }
          yes=no
        }
      }
      unit: 1
    }
  }

  file: /this/is/a/file/path
  url: "http://domain.com?x=y"

  tbl1: { x=1, y: 2, z=3, "hello yo"="world", yes=no }
  tbl2: { x=1, y: "2 3", z=3, hello="world wide", yes=     x  }

  one {
    " two\a\b\" xxx" = [
       1,
       2,
       {
         one {
           two {
             three = 3
             four = 4
             hello=1
             world=2
           }
         }
       },
    ]
  }

  subtype {
    "this is.a test[]{}".a = [
      abc,
      5,
      -.03,
      table,
      {
        a {
          b.c=1
          d=2
        }
      },
    ]
    x = 9
  }

  aaa.b.c {
    a.b.c [
      {
        a {
          b.c {
            f {
              g {
                h {
                  i: 5
                  j: 6
                  k: 5
                  l: 6
                }
              }
            }
          }
        }
      },
    ]
  }

  "list" [
    one
    two
    3
    "false"
    four,
    null
  ]

  null_val: null

  nonnull_val: "null"
)";

TEST_CASE( "[emit] emit no flatten keys" ) {
  // We are not testing this here as it is tested in the parser
  // module. We are just using it for convenience.
  auto doc = parse( "fake-file", input );
  REQUIRE( doc );

  string emitted = emit( *doc, { .flatten_keys = false } );

  static string const expected = R"(a {
  c {
    f: 5
    g: true
    h: "truedat"
  }
}

aaa {
  b {
    c {
      a {
        b {
          c: [
            {
              a {
                b {
                  c {
                    f {
                      g {
                        h {
                          i: 5
                          j: 6
                          k: 5
                          l: 6
                        }
                      }
                    }
                  }
                }
              }
            },
          ]
        }
      }
    }
  }
}

b {
  s: 5
  t: 3.5
}

c {
  d {
    e {
      f {
        g {
          h {
            i: 9
            j: 10
          }
          yes: no
        }
      }
      unit: 1
    }
  }
}

file: /this/is/a/file/path

list: [
  one,
  two,
  3,
  "false",
  four,
  null,
]

nonnull_val: "null"

null_val: null

one {
  " two\\a\\b\" xxx": [
    1,
    2,
    {
      one {
        two {
          four: 4
          hello: 1
          three: 3
          world: 2
        }
      }
    },
  ]
}

subtype {
  "this is.a test[]{}" {
    a: [
      abc,
      5,
      -0.03,
      table,
      {
        a {
          b {
            c: 1
          }
          d: 2
        }
      },
    ]
  }
  x: 9
}

tbl1 {
  "hello yo": world
  x: 1
  y: 2
  yes: no
  z: 3
}

tbl2 {
  hello: world wide
  x: 1
  y: "2 3"
  yes: x
  z: 3
}

url: "http://domain.com?x=y"

z {}

zz {
  yy {}
}
)";

  REQUIRE( emitted == expected );

  // Let's make sure that Rcl can parse what it emits.
  auto doc2 = parse( "fake-file", emitted );
  REQUIRE( doc2 );

  // Now a round trip.
  string emitted2 = emit( *doc2, { .flatten_keys = false } );
  REQUIRE( emitted == emitted2 );
}

TEST_CASE( "[emit] emit flatten keys" ) {
  // We are not testing this here as it is tested in the parser
  // module. We are just using it for convenience.
  auto doc = parse( "fake-file", input );
  REQUIRE( doc );

  string emitted = emit( *doc );

  static string const expected = R"(a.c {
  f: 5
  g: true
  h: "truedat"
}

aaa.b.c.a.b.c: [
  .a.b.c.f.g.h {
    i: 5
    j: 6
    k: 5
    l: 6
  },
]

b {
  s: 5
  t: 3.5
}

c.d.e {
  f.g {
    h {
      i: 9
      j: 10
    }
    yes: no
  }
  unit: 1
}

file: /this/is/a/file/path

list: [
  one,
  two,
  3,
  "false",
  four,
  null,
]

nonnull_val: "null"

null_val: null

one." two\\a\\b\" xxx": [
  1,
  2,
  .one.two {
    four: 4
    hello: 1
    three: 3
    world: 2
  },
]

subtype {
  "this is.a test[]{}".a: [
    abc,
    5,
    -0.03,
    table,
    .a {
      b.c: 1
      d: 2
    },
  ]
  x: 9
}

tbl1 {
  "hello yo": world
  x: 1
  y: 2
  yes: no
  z: 3
}

tbl2 {
  hello: world wide
  x: 1
  y: "2 3"
  yes: x
  z: 3
}

url: "http://domain.com?x=y"

z {}

zz.yy {}
)";

  REQUIRE( emitted == expected );

  // Let's make sure that Rcl can parse what it emits.
  auto doc2 = parse( "fake-file", emitted );
  REQUIRE( doc2 );

  // Now a round trip.
  string emitted2 = emit( *doc2 );
  REQUIRE( emitted == emitted2 );
}

TEST_CASE( "[emit] emit json" ) {
  // We are not testing this here as it is tested in the parser
  // module. We are just using it for convenience.
  auto const doc = parse( "fake-file", input );
  REQUIRE( doc );

  string const emitted = emit_json( *doc );

  static string const expected = R"({
  "a": {
    "c": {
      "f": 5,
      "g": true,
      "h": "truedat"
    }
  },
  "aaa": {
    "b": {
      "c": {
        "a": {
          "b": {
            "c": [
              {
                "a": {
                  "b": {
                    "c": {
                      "f": {
                        "g": {
                          "h": {
                            "i": 5,
                            "j": 6,
                            "k": 5,
                            "l": 6
                          }
                        }
                      }
                    }
                  }
                }
              }
            ]
          }
        }
      }
    }
  },
  "b": {
    "s": 5,
    "t": 3.5
  },
  "c": {
    "d": {
      "e": {
        "f": {
          "g": {
            "h": {
              "i": 9,
              "j": 10
            },
            "yes": "no"
          }
        },
        "unit": 1
      }
    }
  },
  "file": "/this/is/a/file/path",
  "list": [
    "one",
    "two",
    3,
    "false",
    "four",
    null
  ],
  "nonnull_val": "null",
  "null_val": null,
  "one": {
    " two\\a\\b\" xxx": [
      1,
      2,
      {
        "one": {
          "two": {
            "four": 4,
            "hello": 1,
            "three": 3,
            "world": 2
          }
        }
      }
    ]
  },
  "subtype": {
    "this is.a test[]{}": {
      "a": [
        "abc",
        5,
        -0.03,
        "table",
        {
          "a": {
            "b": {
              "c": 1
            },
            "d": 2
          }
        }
      ]
    },
    "x": 9
  },
  "tbl1": {
    "hello yo": "world",
    "x": 1,
    "y": 2,
    "yes": "no",
    "z": 3
  },
  "tbl2": {
    "hello": "world wide",
    "x": 1,
    "y": "2 3",
    "yes": "x",
    "z": 3
  },
  "url": "http://domain.com?x=y",
  "z": {},
  "zz": {
    "yy": {}
  }
})";

  REQUIRE( emitted == expected );

  // Let's make sure that Rcl can parse what it emits.
  auto const doc2 = parse( "fake-file", emitted );
  REQUIRE( doc2 );

  // Now a round trip.
  string const emitted2 = emit_json( *doc2 );
  REQUIRE( emitted == emitted2 );
}

TEST_CASE( "[emit] emit json with key ordering" ) {
  static string const input = R"({
  "__key_order": [
    "hello",
    "test",
    "foo",
    "bar",
    "aaa"
  ],
  "foo": 1,
  "bar": 2,
  "hello": "world",
  "zzz": "will disappear",
  "aaa": [
    {
      "__key_order": ["a"],
      "a": 99
    }
  ],
  "test": {
    "d": 9,
    "c": {
      "f": 5,
      "g": true,
      "h": "truedat",
      "__key_order": [
        "g",
        "f",
        "h"
      ]
    }
  },
})";

  // We are not testing this here as it is tested in the parser
  // module. We are just using it for convenience.
  auto const doc = parse( "fake-file", input );
  REQUIRE( doc );

  string const emitted = emit_json(
      *doc, JsonEmitOptions{ .key_order_tag = "__key_order" } );

  static string const expected = R"({
  "hello": "world",
  "test": {
    "c": {
      "g": true,
      "f": 5,
      "h": "truedat"
    },
    "d": 9
  },
  "foo": 1,
  "bar": 2,
  "aaa": [
    {
      "a": 99
    }
  ]
})";

  REQUIRE( emitted == expected );

  // We don't try to parse the emitted output again because it
  // won't have the __key_order tags in it, so we can't do an-
  // other round trip.
}

} // namespace
} // namespace rcl
