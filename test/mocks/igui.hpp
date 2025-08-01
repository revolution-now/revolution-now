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
#include "src/co-combinator.hpp"
#include "src/igui.hpp"
#include "src/view.hpp"

// mock
#include "src/mock/mock.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

#define MOCK_GUI_METHOD( ret_type, name, params ) \
  MOCK_METHOD( ret_type, name, params, () )

namespace rn {

struct View;

struct MockIGui : IGui {
  MOCK_GUI_METHOD( wait<>, message_box, (std::string const&));

  MOCK_GUI_METHOD( wait<>, message_box,
                   (MessageBoxOptions const&,
                    std::string const&));

  MOCK_GUI_METHOD( void, transient_message_box,
                   (std::string const&));

  MOCK_GUI_METHOD( wait<std::chrono::microseconds>, wait_for,
                   ( std::chrono::microseconds ) );

  MOCK_GUI_METHOD( wait<maybe<std::string>>, choice,
                   (ChoiceConfig const&));

  MOCK_GUI_METHOD( wait<maybe<std::string>>, string_input,
                   (StringInputConfig const&));

  MOCK_GUI_METHOD( wait<maybe<int>>, int_input,
                   (IntInputConfig const&));

  using CheckBoxInfoMap = std::unordered_map<int, CheckBoxInfo>;
  using CheckBoxResultMap = std::unordered_map<int, bool>;
  MOCK_GUI_METHOD( wait<CheckBoxResultMap>, check_box_selector,
                   (std::string const&, CheckBoxInfoMap const&));

  MOCK_GUI_METHOD( wait<ui::e_ok_cancel>, ok_cancel_box,
                   (std::string const&, ui::View&));

  MOCK_GUI_METHOD( wait<>, ok_cancel_box_async,
                   (std::string const, ui::View&,
                    co::stream<ui::e_ok_cancel>&));

  MOCK_GUI_METHOD( wait<>, display_woodcut, ( e_woodcut ) );

  MOCK_METHOD( int, total_windows_created, (), ( const ) );
};

static_assert( !std::is_abstract_v<MockIGui> );

} // namespace rn
