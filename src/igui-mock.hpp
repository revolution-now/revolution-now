/****************************************************************
**igui-mock.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-25.
*
* Description: For dependency injection in unit tests.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "igui.hpp"

// mock
#include "mock/mock.hpp"

#define MOCK_GUI_METHOD( ret_type, name, params ) \
  MOCK_METHOD( ret_type, name, params, () )

namespace rn {

struct MockIGui : IGui {
  MOCK_GUI_METHOD( wait<>, message_box, ( std::string_view ) );
  MOCK_GUI_METHOD( wait<std::string>, choice,
                   (ChoiceConfig const&));
};

static_assert( !std::is_abstract_v<MockIGui> );

} // namespace rn
