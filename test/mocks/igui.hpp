/****************************************************************
**igui.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-25.
*
* Description: For dependency injection in unit tests.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "src/igui.hpp"

// mock
#include "src/mock/mock.hpp"

#define MOCK_GUI_METHOD( ret_type, name, params ) \
  MOCK_METHOD( ret_type, name, params, () )

namespace rn {

struct MockIGui : IGui {
  MOCK_GUI_METHOD( wait<>, message_box, ( std::string_view ) );

  MOCK_GUI_METHOD( wait<std::string>, choice,
                   (ChoiceConfig const&));

  MOCK_GUI_METHOD( wait<std::string>, string_input,
                   (StringInputConfig const&));

  MOCK_GUI_METHOD( wait<std::chrono::microseconds>, wait_for,
                   ( std::chrono::microseconds ) );

  MOCK_GUI_METHOD( wait<int>, int_input,
                   (IntInputConfig const&));
};

static_assert( !std::is_abstract_v<MockIGui> );

} // namespace rn
