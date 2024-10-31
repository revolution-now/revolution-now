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

namespace rr {
struct Renderer;
}

namespace gfx {
struct Resolution;
struct ResolutionScores;
}

namespace rn {

struct SelectedResolution;

enum class e_resolution;

maybe<gfx::Resolution const&> get_global_resolution();

// TODO: temporary.
maybe<gfx::ResolutionScores const&>
get_global_resolution_scores();

// NOTE: you should not normally call this in most game code, in-
// stead you should go through the compositor in order to allow
// it to control the logical size of each element on the screen.
gfx::size main_window_logical_size();
gfx::size main_window_physical_size();

maybe<e_resolution> main_window_named_logical_resolution();

// !! origin at (0,0)
gfx::rect main_window_logical_rect();

void on_main_window_resized( rr::Renderer& renderer );

void on_logical_resolution_changed(
    rr::Renderer&             renderer,
    SelectedResolution const& resolution );

void       cycle_resolution( int delta );
void       set_resolution_idx_to_optimal();
maybe<int> get_resolution_idx();
maybe<int> get_resolution_cycle_size();

// Returns true if the window is now fullscreen.
bool toggle_fullscreen();
bool is_window_fullscreen();
void restore_window();

// Shrinking to fit viewport.
void shrink_window_to_fit();
bool can_shrink_window_to_fit();

void* main_os_window_handle();

void hide_window();

maybe<double> monitor_dpi();

} // namespace rn
