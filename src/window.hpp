/****************************************************************
* window.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-30.
*
* Description: Handles windowing system for user interaction.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#include <functional>
#include <string_view>

namespace rn {

using RenderFunc = std::function<void(void)>;

void message_box( std::string_view msg, RenderFunc render_bg );

} // namespace rn
