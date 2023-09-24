/****************************************************************
**line-editor.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-25.
*
* Description: Unit tests for the line-editor module.
*
*****************************************************************/
#include "testing.hpp"

// Revolution Now
#include "line-editor.hpp"

// base
#include "src/base/meta.hpp"

// Must be last.
#include "catch-common.hpp"

namespace {

using namespace std;
using namespace rn;

TEST_CASE( "[line-editor] construction" ) {
  {
    LineEditor le;
    REQUIRE( le.buffer().empty() );
    REQUIRE( le.pos() == 0 );
  }
  {
    LineEditor le( "hello", 0 );
    REQUIRE( le.buffer() == "hello" );
    REQUIRE( le.pos() == 0 );
  }
  {
    LineEditor le( "hello", -1 );
    REQUIRE( le.buffer() == "hello" );
    REQUIRE( le.pos() == 0 );
  }
  {
    LineEditor le( "hello", 4 );
    REQUIRE( le.buffer() == "hello" );
    REQUIRE( le.pos() == 4 );
  }
  {
    LineEditor le( "hello", 5 );
    REQUIRE( le.buffer() == "hello" );
    REQUIRE( le.pos() == 5 );
  }
  {
    LineEditor le( "hello", 6 );
    REQUIRE( le.buffer() == "hello" );
    REQUIRE( le.pos() == 5 );
  }
  {
    LineEditor le( "", 1 );
    REQUIRE( le.buffer() == "" );
    REQUIRE( le.pos() == 0 );
  }
}

TEST_CASE( "[line-editor] input" ) {
  LineEditor le;

  auto input_char = [&]( int c, bool shift ) {
    input::key_event_t event;
    event.keycode      = c;
    event.mod.shf_down = shift;
    return le.input( event );
  };

  auto input = mp::overload{ input_char,
                             [&]( string_view sv, bool shift ) {
                               for( char const c : sv )
                                 input_char( c, shift );
                             } };

  REQUIRE( le.buffer() == "" );
  REQUIRE( le.pos() == 0 );

  input( '1', /*shift=*/false );
  REQUIRE( le.buffer() == "1" );
  REQUIRE( le.pos() == 1 );

  input( "1234", /*shift=*/false );
  REQUIRE( le.buffer() == "11234" );
  REQUIRE( le.pos() == 5 );

  input( "ab", /*shift=*/true );
  REQUIRE( le.buffer() == "11234AB" );
  REQUIRE( le.pos() == 7 );

  input( ::SDLK_LEFT, /*shift=*/false );
  input( ::SDLK_LEFT, /*shift=*/false );
  REQUIRE( le.buffer() == "11234AB" );
  REQUIRE( le.pos() == 5 );

  input( ::SDLK_BACKSPACE, /*shift=*/false );
  REQUIRE( le.buffer() == "1123AB" );
  REQUIRE( le.pos() == 4 );

  input( ::SDLK_DELETE, /*shift=*/false );
  REQUIRE( le.buffer() == "1123B" );
  REQUIRE( le.pos() == 4 );

  input( ::SDLK_HOME, /*shift=*/false );
  REQUIRE( le.buffer() == "1123B" );
  REQUIRE( le.pos() == 0 );

  input( ::SDLK_BACKSPACE, /*shift=*/false );
  REQUIRE( le.buffer() == "1123B" );
  REQUIRE( le.pos() == 0 );

  input( ::SDLK_DELETE, /*shift=*/false );
  REQUIRE( le.buffer() == "123B" );
  REQUIRE( le.pos() == 0 );

  input( ::SDLK_RIGHT, /*shift=*/false );
  REQUIRE( le.buffer() == "123B" );
  REQUIRE( le.pos() == 1 );

  input( ::SDLK_BACKSPACE, /*shift=*/false );
  REQUIRE( le.buffer() == "23B" );
  REQUIRE( le.pos() == 0 );

  input( ::SDLK_END, /*shift=*/false );
  REQUIRE( le.buffer() == "23B" );
  REQUIRE( le.pos() == 3 );

  le.clear();
  REQUIRE( le.buffer() == "" );
  REQUIRE( le.pos() == 0 );

  input( "345", /*shift=*/true );
  REQUIRE( le.buffer() == "#$%" );
  REQUIRE( le.pos() == 3 );

  input( ::SDLK_BACKSPACE, /*shift=*/false );
  input( ::SDLK_BACKSPACE, /*shift=*/false );
  input( ::SDLK_BACKSPACE, /*shift=*/false );
  REQUIRE( le.buffer() == "" );
  REQUIRE( le.pos() == 0 );

  input( ::SDLK_DELETE, /*shift=*/false );
  REQUIRE( le.buffer() == "" );
  REQUIRE( le.pos() == 0 );
}

TEST_CASE( "[line-editor] set" ) {
  SECTION( "starts empty" ) {
    LineEditor le( "", 0 );
    SECTION( "keep same" ) {
      le.set( "", 0 );
      REQUIRE( le.buffer() == "" );
      REQUIRE( le.pos() == 0 );
    }
    SECTION( "add string, keep cursor same" ) {
      le.set( "abc" );
      REQUIRE( le.buffer() == "abc" );
      REQUIRE( le.pos() == 0 );
    }
    SECTION( "add string, put cursor at end" ) {
      le.set( "abc", -1 );
      REQUIRE( le.buffer() == "abc" );
      REQUIRE( le.pos() == 3 );
    }
    SECTION( "add string, put cursor at middle" ) {
      le.set( "abc", 1 );
      REQUIRE( le.buffer() == "abc" );
      REQUIRE( le.pos() == 1 );
    }
    SECTION( "add string, put cursor negative" ) {
      le.set( "abc", -2 );
      REQUIRE( le.buffer() == "abc" );
      REQUIRE( le.pos() == 2 );
    }
    SECTION( "add string, put cursor at beginning" ) {
      le.set( "abc", 0 );
      REQUIRE( le.buffer() == "abc" );
      REQUIRE( le.pos() == 0 );
    }
  }
  SECTION( "starts single, cursor at beginning" ) {
    LineEditor le( "a", 0 );
    SECTION( "keep same" ) {
      le.set( "a", 0 );
      REQUIRE( le.buffer() == "a" );
      REQUIRE( le.pos() == 0 );
    }
    SECTION( "add string, keep cursor same" ) {
      le.set( "aabc" );
      REQUIRE( le.buffer() == "aabc" );
      REQUIRE( le.pos() == 0 );
    }
    SECTION( "add string, put cursor at end" ) {
      le.set( "aabc", 4 );
      REQUIRE( le.buffer() == "aabc" );
      REQUIRE( le.pos() == 4 );
    }
    SECTION( "add string, put cursor at middle" ) {
      le.set( "aabc", -3 );
      REQUIRE( le.buffer() == "aabc" );
      REQUIRE( le.pos() == 2 );
    }
    SECTION( "add string, put cursor negative" ) {
      le.set( "aabc", -5 );
      REQUIRE( le.buffer() == "aabc" );
      REQUIRE( le.pos() == 0 );
    }
    SECTION( "add string, put cursor at beginning" ) {
      le.set( "aabc", 0 );
      REQUIRE( le.buffer() == "aabc" );
      REQUIRE( le.pos() == 0 );
    }
    SECTION( "remove string" ) {
      le.set( "", -1 );
      REQUIRE( le.buffer() == "" );
      REQUIRE( le.pos() == 0 );
    }
  }
  SECTION( "starts single, cursor at end" ) {
    LineEditor le( "a", 1 );
    SECTION( "keep same" ) {
      le.set( "a", 1 );
      REQUIRE( le.buffer() == "a" );
      REQUIRE( le.pos() == 1 );
    }
    SECTION( "add string, keep cursor same" ) {
      le.set( "aabc" );
      REQUIRE( le.buffer() == "aabc" );
      REQUIRE( le.pos() == 1 );
    }
    SECTION( "add string, put cursor at end" ) {
      le.set( "aabc", 4 );
      REQUIRE( le.buffer() == "aabc" );
      REQUIRE( le.pos() == 4 );
    }
    SECTION( "add string, put cursor at middle" ) {
      le.set( "aabc", 1 );
      REQUIRE( le.buffer() == "aabc" );
      REQUIRE( le.pos() == 1 );
    }
    SECTION( "add string, put cursor negative" ) {
      le.set( "aabc", -1 );
      REQUIRE( le.buffer() == "aabc" );
      REQUIRE( le.pos() == 4 );
    }
    SECTION( "add string, put cursor at beginning" ) {
      le.set( "aabc", 0 );
      REQUIRE( le.buffer() == "aabc" );
      REQUIRE( le.pos() == 0 );
    }
    SECTION( "remove string" ) {
      le.set( "", 0 );
      REQUIRE( le.buffer() == "" );
      REQUIRE( le.pos() == 0 );
    }
  }
  SECTION( "starts multiple, cursor at end" ) {
    LineEditor le( "hello", 5 );
    SECTION( "keep same" ) {
      le.set( "hello", 5 );
      REQUIRE( le.buffer() == "hello" );
      REQUIRE( le.pos() == 5 );
    }
    SECTION( "add string, keep cursor same" ) {
      le.set( "hello world" );
      REQUIRE( le.buffer() == "hello world" );
      REQUIRE( le.pos() == 5 );
    }
    SECTION( "add string, put cursor at end" ) {
      le.set( "hello world", -2 );
      REQUIRE( le.buffer() == "hello world" );
      REQUIRE( le.pos() == 10 );
    }
    SECTION( "add string, put cursor at middle" ) {
      le.set( "hello world", -5 );
      REQUIRE( le.buffer() == "hello world" );
      REQUIRE( le.pos() == 7 );
    }
    SECTION( "add string, put cursor negative" ) {
      le.set( "hello world", -10 );
      REQUIRE( le.buffer() == "hello world" );
      REQUIRE( le.pos() == 2 );
    }
    SECTION( "add string, put cursor at beginning" ) {
      le.set( "hello world", 0 );
      REQUIRE( le.buffer() == "hello world" );
      REQUIRE( le.pos() == 0 );
    }
    SECTION( "remove string" ) {
      le.set( "he", 5 );
      REQUIRE( le.buffer() == "he" );
      REQUIRE( le.pos() == 2 );
    }
  }
  SECTION( "starts multiple, cursor in middle" ) {
    LineEditor le( "hello", 2 );
    SECTION( "keep same" ) {
      le.set( "hello", 2 );
      REQUIRE( le.buffer() == "hello" );
      REQUIRE( le.pos() == 2 );
    }
    SECTION( "add string, keep cursor same" ) {
      le.set( "hello world" );
      REQUIRE( le.buffer() == "hello world" );
      REQUIRE( le.pos() == 2 );
    }
    SECTION( "add string, put cursor at end" ) {
      le.set( "hello world", 11 );
      REQUIRE( le.buffer() == "hello world" );
      REQUIRE( le.pos() == 11 );
    }
    SECTION( "add string, put cursor at middle" ) {
      le.set( "hello world", -5 );
      REQUIRE( le.buffer() == "hello world" );
      REQUIRE( le.pos() == 7 );
    }
    SECTION( "add string, put cursor negative" ) {
      le.set( "hello world", -3 );
      REQUIRE( le.buffer() == "hello world" );
      REQUIRE( le.pos() == 9 );
    }
    SECTION( "add string, put cursor at beginning" ) {
      le.set( "hello world", -12 );
      REQUIRE( le.buffer() == "hello world" );
      REQUIRE( le.pos() == 0 );
    }
    SECTION( "remove string" ) {
      le.set( "h" );
      REQUIRE( le.buffer() == "h" );
      REQUIRE( le.pos() == 1 );
    }
  }
}

} // namespace
