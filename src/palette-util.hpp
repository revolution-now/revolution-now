/****************************************************************
**palette-util.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-17.
*
* Description: Non-game utils for palette manipulation.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// gfx
#include "gfx/pixel.hpp"

// base
#include "base/fs.hpp"
#include "base/maybe.hpp"

// c++ standard library
#include <cstdint>
#include <vector>

namespace rr {
struct Renderer;
}

namespace rn {

/****************************************************************
**Non-Game Utilities: Palette Generation/Display
*
* These functions are not run during the game; they are only run
* in a once-off manner to generate/update the game palettes.
*****************************************************************/
// Given an arbitrary color it will return the hierarchical
// location of that color in the bucketing scheme that we use in
// this game. E.g., calling it with #FF0000 might return
// something along the lines of "red.sat2.lum12".
std::string bucket_path( gfx::pixel c );

// Sorts colors in a quasi-human way.
void hsl_bucketed_sort( std::vector<gfx::pixel>& colors );

// Removes colors with a saturation below a threshold.
void remove_greys( std::vector<gfx::pixel>& colors );

// For holding a list of colors that are bucketed first by hue,
// then by saturation, then by luminosity.
using ColorBuckets = std::vector<
    std::vector<std::vector<base::maybe<gfx::pixel>>>>;

// This will iterate through the colors and place each one into a
// bucket depending on its values of hue, saturation, and
// luminosity (each of which are bucketed). There are only a
// finite number of buckets in each category, so in general the
// resulting colors will be fewer than the input if the input
// contains multiple colors that all fall into the same bucket.
// If there are no colors for a bucket then that bucket will be
// nothing.
ColorBuckets hsl_bucket( std::vector<gfx::pixel> const& colors );

// Load the image file(s) and scan every pixel and compile a list
// of unique colors. Then, if a target number of colors is
// specified, try to reduce the number of colors to achieve
// approximately the target number. The algorithm will try its
// best to achieve this number, but typically the set of returned
// colors may have a bit more or less. Also, the order of colors
// returned is unspecified.
std::vector<gfx::pixel> extract_palette(
    fs::path const&  glob,
    base::maybe<int> target = base::nothing );

// Will remove colors that are redundant or approximately
// redunant in order to meet the target count. It will always
// return a number of colors that is >= min_count so long as
// there are at least that many to begin with.
std::vector<gfx::pixel> coursen(
    std::vector<gfx::pixel> const& colors, int min_count );

// Will look in the `where` folder and will load all files
// (assuming they are image files) and will load/scan each one of
// them for their colors and will generate a ~256 color palette
// and will update the schema and rcl palette definition file.
// Note that running this could in general break your build
// because the game might be using colors (e.g.
// config_palette.red.sat0.lum1) that no longer appear after the
// update.
void update_palette( fs::path const& where );

// Will load/parse the rcl config palette file and will
// sort/bucket the colors and display them on the screen. NOTE:
// SDL graphics must have been initialized before calling this
// function.
void show_config_palette( rr::Renderer& renderer );

// Will load the rcl config palette and render it to a png
// file divided into hue/saturation buckets.
void write_palette_png( fs::path const& png_file );

// Generate the config schema and rcl data file with all the
// colors so that the game can reference them with e.g.
// config_palette.red.sat0.lum1. Note that this updates the
// schema file and hence requires recompilation to take effect.
void dump_palette( ColorBuckets const& colors,
                   fs::path const& schema, fs::path const& rcl );

// Display an array of colors.
void show_palette( std::vector<gfx::pixel> const& colors );

// Preview color shading and highlighting.  This will render
// a color gradient to the screen.
void show_color_adjustment( gfx::pixel center );

} // namespace rn
