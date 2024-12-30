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
struct ResolutionScores;
struct SelectedResolution;
struct Resolutions;
}

namespace rn {

struct IEngine;

maybe<gfx::Resolution const&> get_global_resolution(
    IEngine& engine );

// TODO: temporary.
maybe<gfx::ResolutionScores const&> get_global_resolution_scores(
    IEngine& engine );

// NOTE: you should not normally call this in most game code, in-
// stead you should go through the compositor in order to allow
// it to control the logical size of each element on the screen.
gfx::size main_window_logical_size(
    vid::IVideo& video, vid::WindowHandle const& wh,
    gfx::Resolutions const& resolutions );

maybe<gfx::e_resolution> main_window_named_logical_resolution(
    gfx::Resolutions const& resolutions );

// !! origin at (0,0)
gfx::rect main_window_logical_rect(
    vid::IVideo& video, vid::WindowHandle const& wh,
    gfx::Resolutions const& resolutions );

void on_main_window_resized( vid::IVideo& video,
                             vid::WindowHandle const& wh,
                             gfx::Resolutions& resolutions,
                             rr::Renderer& renderer );

void on_logical_resolution_changed(
    vid::IVideo& video, vid::WindowHandle const& wh,
    rr::Renderer& renderer, gfx::Resolutions& resolutions,
    gfx::SelectedResolution const& selected_resolution );

void cycle_resolution( gfx::Resolutions const& resolutions,
                       int const delta );
void set_resolution_idx_to_optimal(
    gfx::Resolutions const& resolutions );
maybe<int> get_resolution_idx(
    gfx::Resolutions const& resolutions );
maybe<int> get_resolution_cycle_size(
    gfx::Resolutions const& resolutions );

// If the result is provided then all fields will be populated.
// If the underlying system API did not provide all components
// then the missing components will be computed from the ones
// provided. If this cannot be done then nothing will be re-
// turned.
maybe<gfx::MonitorDpi> monitor_dpi( vid::IVideo& video );

} // namespace rn
