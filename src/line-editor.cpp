/****************************************************************
**line-editor.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-08.
*
* Description: Holds and displays a text buffer being edited.
*
*****************************************************************/
#include "line-editor.hpp"

// Revolution Now
#include "errors.hpp"

// SDL
#include "SDL.h"

// C++ standard library
#include <algorithm>

using namespace std;

namespace rn {

namespace {

bool is_char_allowed( char c ) {
  static string_view const cs{
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "0123456789 +-*/^._,()[]!@#$%&={}|?<>`~;"};
  return find( begin( cs ), end( cs ), c ) != end( cs );
}

} // namespace

#define LE_ASSERT_INVARIANTS \
  CHECK( pos_ >= 0 )         \
  CHECK( pos_ <= (int)buffer_.length() )

bool LineEditor::input( input::key_event_t const& event ) {
  // bool alt     = event.alt_down;
  // bool ctrl    = event.ctrl_down;
  auto pressed = event.keycode;
  // if( ctrl && !alt ) {
  //  if( pressed == 0x7f ) {
  //    // Hack for OS X in which backspace seems  to  appear
  //    // as a control  character  with  the  key being 0x7f.
  //    pressed = ::SDLK_BACKSPACE;
  //    ctrl = alt = false;
  //  }
  //}

  if( pressed < 128 && is_char_allowed( char( pressed ) ) ) {
    ASSIGN_CHECK_OPT( ascii,
                      input::ascii_char_for_event( event ) );
    buffer_.insert( pos_, 1, char( ascii ) );
    ++pos_;
    LE_ASSERT_INVARIANTS;
    return true;
  }

  switch( pressed ) {
    case ::SDLK_LEFT:
      --pos_;
      pos_ = ( pos_ < 0 ) ? 0 : pos_;
      LE_ASSERT_INVARIANTS;
      return true;
    case ::SDLK_RIGHT:
      ++pos_;
      pos_ = ( pos_ > (int)buffer_.length() ) ? buffer_.length()
                                              : pos_;
      LE_ASSERT_INVARIANTS;
      return true;
    case ::SDLK_BACKSPACE:
      if( pos_ > 0 ) {
        buffer_.erase( pos_ - 1, 1 );
        --pos_;
        LE_ASSERT_INVARIANTS;
      }
      return true;
    case ::SDLK_DELETE:
      if( pos_ < (int)buffer_.length() ) {
        buffer_.erase( pos_, 1 );
        LE_ASSERT_INVARIANTS;
      }
      return true;
    case ::SDLK_END:
      pos_ = buffer_.length();
      LE_ASSERT_INVARIANTS;
      return true;
    case ::SDLK_HOME:
      pos_ = 0;
      LE_ASSERT_INVARIANTS;
      return true;
  };
  LE_ASSERT_INVARIANTS;
  return false;
}

void LineEditor::clear() {
  pos_    = 0;
  buffer_ = "";
  LE_ASSERT_INVARIANTS;
}

#define ASSERT_INVARIANTS_BUF                           \
  CHECK( ( buffer.length() == 0 && start_pos_ == 0 ) || \
         ( start_pos_ < (int)buffer.length() ) );

#define ASSERT_INVARIANTS_POS                                 \
  CHECK( width_ > 0 );                                        \
  CHECK( start_pos_ >= 0 );                                   \
  CHECK( ( ( width_ & 1 ) &&                                  \
           ( start_pos_ % width_ == 0 ||                      \
             start_pos_ % width_ == ( width_ - 1 ) ||         \
             start_pos_ % width_ == ( width_ / 2 ) ||         \
             start_pos_ % width_ == ( width_ / 2 ) - 1 ) ) || \
         ( !( width_ & 1 ) &&                                 \
           ( start_pos_ % ( width_ / 2 ) == 0 ) ) );          \
  CHECK( abs_cursor_pos >= start_pos_ );                      \
  CHECK( abs_cursor_pos < start_pos_ + width_ );

#define ASSERT_INVARIANTS \
  ASSERT_INVARIANTS_POS   \
  ASSERT_INVARIANTS_BUF

string LineEditorInputView::render( int           abs_cursor_pos,
                                    string const& buffer ) {
  int pos = abs_cursor_pos;

  // First update internal state
  if( start_pos_ == (int)buffer.size() || pos < start_pos_ ||
      pos >= start_pos_ + width_ ) {
    if( width_ & 1 ) {
      start_pos_ = pos - ( ( pos % width_ ) % ( width_ / 2 ) );
      if( start_pos_ == pos ) start_pos_ -= ( width_ / 2 + 1 );
      if( start_pos_ < 0 ) start_pos_ = 0;
      ASSERT_INVARIANTS
    } else {
      start_pos_ = pos - ( pos % ( width_ / 2 ) );
      if( start_pos_ == pos ) start_pos_ -= width_ / 2;
      if( start_pos_ < 0 ) start_pos_ = 0;
      ASSERT_INVARIANTS
    }
  }
  ASSERT_INVARIANTS

  // Now render. First take all characters in the buffer after
  // the starting position, then cut it down to a maximum of the
  // width of the window.
  string out( buffer.begin() + start_pos_, buffer.end() );
  if( int( out.size() ) > width_ ) out.resize( width_ );
  CHECK( out.size() <= size_t( width_ ) );
  return out;
}
} // namespace rn
