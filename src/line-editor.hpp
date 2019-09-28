/****************************************************************
**line-editor.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-08.
*
* Description: Holds and displays a text buffer being edited.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "input.hpp"

// C++ standard library
#include <string>

namespace rn {

/****************************************************************
** LineEditor
*****************************************************************/
// LineEditor: class that accepts a stream of key presses and
// uses them to edit a one-line block of text together with a
// "cursor position" within that line.
class LineEditor {
  std::string buffer_{};
  int         pos_ = 0;

public:
  LineEditor() = default;
  LineEditor( std::string buffer, int pos );

  std::string const& buffer() const { return buffer_; }
  int                pos() const { return pos_; }

  // Leaving off cursor position, it will attempt to keep it
  // where it is, unless it is out of bounds in which case it
  // will be put at the end. One can specify -1 for the cursor
  // position which means one-past-the-ene; -2 places the cursor
  // over the last character, etc. Regardless of the `cursor_pos`
  // specified, it will always be clamped to the bounds of the
  // new string.
  void set( std::string_view new_buffer,
            Opt<int>         maybe_pos = std::nullopt );

  void clear();

  bool input( input::key_event_t const& event );
};
NOTHROW_MOVE( LineEditor );

/****************************************************************
** LineEditorInputView
*****************************************************************/
// Represents a movable window into a string. It takes a string
// and renders a slice of it depending on the "location" of the
// window. This is not simply slicing a string though; this class
// must maintain internal state, because the movement of the
// window is intended to follow the movement of a cursor position
// (which must also be supplied to render the window) and the
// movement of the window is dependent not only on the current
// cursor position but also on the history of previous cursor po-
// sitions. This is required in order to deliver a natural expe-
// rience that a user would expect when a string is navigated by
// moving a cursor along a line.
class LineEditorInputView {
  int start_pos_{0};
  // Size of the window.
  int width_;

public:
  LineEditorInputView( int w ) : width_( w ) {}

  int width() const { return width_; }

  // Given an absolute cursor position, return the distance from
  // the start of the viewing window.
  int rel_pos( int p ) const { return p - start_pos_; }

  // Will always return a string whose length is <= width. This
  // function cannot be const because in general it must be al-
  // lowed to mutate the LineEditorInputView's starting position
  // (state). The LineEditorInputView must maintain this state
  // because there are actually multiple ways that the LineEdi-
  // torInputView could be rendered given the same abs_cursor_pos
  // input, and the one that is chosen it affected by previous
  // renderings and cursor positions. This is to allow a natural
  // editing experience that a user would expect where the cursor
  // can move from the start to end of a line and only moving to
  // the next frame when the cursor touches one of the extremi-
  // ties.
  std::string render( int                abs_cursor_pos,
                      std::string const& buffer );
};
NOTHROW_MOVE( LineEditorInputView );

} // namespace rn
