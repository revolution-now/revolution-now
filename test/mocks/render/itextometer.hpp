/****************************************************************
**itextometer.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2025-03-08.
*
* Description: Mock implementation of ITextometer.
*
*****************************************************************/
#pragma once

// render
#include "src/render/itextometer.hpp"

// mock
#include "src/mock/mock.hpp"

namespace rr {

struct MockTextometer : ITextometer {
  MOCK_METHOD( gfx::size, dimensions_for_line,
               (TextLayout const&, std::string const&),
               ( const ) );
  MOCK_METHOD( int, spacing_between_chars, (TextLayout const&),
               ( const ) );
  MOCK_METHOD( int, spacing_between_lines, (TextLayout const&),
               ( const ) );
  MOCK_METHOD( gfx::interval, trimmed_horizontally,
               ( TextLayout const&, char const ), ( const ) );
  MOCK_METHOD( int, font_height, (), ( const ) );
};

static_assert( !std::is_abstract_v<MockTextometer> );

} // namespace rr
