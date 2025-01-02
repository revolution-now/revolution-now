/****************************************************************
**screen.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-02-18.
*
* Description: Handles screen resolution and scaling.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "error.hpp"
#include "maybe.hpp"

// gfx
#include "gfx/cartesian.hpp"
#include "gfx/logical.hpp"

namespace rr {
struct Renderer;
}

namespace vid {
struct IVideo;
struct WindowHandle;
}

namespace gfx {
enum class e_resolution;
struct Resolution;
struct Resolutions;
}

namespace rn {

struct IEngine;

maybe<gfx::Resolution const&> get_resolution( IEngine& engine );

maybe<gfx::e_resolution> named_resolution( IEngine& engine );

gfx::size main_window_logical_size(
    vid::IVideo& video, vid::WindowHandle const& wh,
    gfx::Resolutions const& resolutions );

// !! origin at (0,0)
gfx::rect main_window_logical_rect(
    vid::IVideo& video, vid::WindowHandle const& wh,
    gfx::Resolutions const& resolutions );

void on_main_window_resized( vid::IVideo& video,
                             vid::WindowHandle const& wh );

void on_logical_resolution_changed(
    vid::IVideo& video, vid::WindowHandle const& wh,
    rr::Renderer& renderer, gfx::Resolutions& actual_resolutions,
    gfx::Resolutions const& new_resolutions );

// If the result is provided then all fields will be populated.
// If the underlying system API did not provide all components
// then the missing components will be computed from the ones
// provided. If this cannot be done then nothing will be re-
// turned.
maybe<gfx::MonitorDpi> monitor_dpi( vid::IVideo& video );

// This is the method that should be used whenever manually
// changing the resolution because it does so properly by in-
// jecting the resolution changes as an input message so that it
// can be scheduled to go into effect at the top of a frame and
// to cause the proper notifications to go out.
void change_resolution( gfx::Resolutions const& resolutions );

void change_resolution_to_named_if_available(
    gfx::Resolutions const& resolutions,
    gfx::e_resolution const target_named );

} // namespace rn
