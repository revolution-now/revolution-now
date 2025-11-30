/****************************************************************
**text.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-03-10.
*
* Description: Handles rendering larger bodies of text with
*              markup.
*
*****************************************************************/
#include "text.hpp"

// Revolution Now
#include "error.hpp"
#include "markup.hpp"

// config
#include "config/ui.rds.hpp"

// render
#include "render/extra.hpp"
#include "render/itextometer.hpp"
#include "render/renderer.hpp"
#include "render/typer.hpp"

// base
#include "base/range-lite.hpp"
#include "base/string.hpp"

// base-util
#include "base-util/string.hpp"

using namespace std;

namespace rn {
namespace {

namespace rl = ::base::rl;

using ::gfx::oriented_point;
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

/****************************************************************
** Rendering
*****************************************************************/
void render_impl( rr::Typer& typer, pixel color,
                  string_view text ) {
  typer.set_color( color );
  typer.write( text );
}

void render_markup( rr::Typer& typer, MarkedUpChunk const& mk,
                    TextMarkupInfo const& info ) {
  if( info.shadow.has_value() ) {
    rr::Typer typer_right =
        typer.with_frame_offset( size{ .w = 1 } );
    rr::Typer typer_down =
        typer.with_frame_offset( size{ .h = 1 } );
    render_impl( typer_right, *info.shadow, mk.text );
    render_impl( typer_down, *info.shadow, mk.text );
  }

  if( mk.style.highlight )
    render_impl( typer, info.highlight, mk.text );
  else
    // Here we assume now special style info, just do default
    // rendering.
    render_impl( typer, info.normal, mk.text );
}

void render_line( rr::Typer& typer, pixel fg,
                  string_view text ) {
  return render_impl( typer, fg, text );
}

void render_line_markup( rr::Typer& typer,
                         vector<MarkedUpChunk> const& mks,
                         TextMarkupInfo const& info ) {
  for( MarkedUpChunk const& mut : mks )
    if( !mut.text.empty() ) //
      render_markup( typer, mut, info );
}

void render_lines( rr::Typer& typer, pixel fg,
                   vector<string> const& txt ) {
  for( string const& line : txt ) {
    render_line( typer, fg, line );
    render_impl( typer, pixel{}, "\n" );
  }
}

void render_lines_markup(
    rr::Typer& typer,
    vector<vector<MarkedUpChunk>> const& mk_text,
    TextMarkupInfo const& info ) {
  for( vector<MarkedUpChunk> const& muts : mk_text ) {
    render_line_markup( typer, muts, info );
    render_impl( typer, pixel{}, "\n" );
  }
}

// Will flatten the text onto one line, then wrap it to within
// `max_cols` columns.
//
// This function is a bit of a disaster, but oh well... it's non-
// trivial because we want to a) wrap the text as if the markup
// characters were not present, b) avoid duplicating markup
// parser in this function, c) try to keep the markup parser and
// word-wrapper decoupled. Not sure if it's really that important
// to impose those requirements, but in any case imposing them
// means that we have to approach this in the following way:
//
//   Steps:
//     1) Split text into words and strip each one of all spaces
//     2) Join words into one long line separated by spaces
//     3) Parse long line for markup, yielding Vec<MarkedUpChunk>
//     4) Extract text from markup results.  This should be the
//        line from #2 but without any markup.
//     5) Wrap the line from #4
//     6) Re-flow the marked up line from #3 into lines of length
//        corresponding to the wrapped lines from #5, resulting
//        in a vector<vector<MarkedUpChunk>>.

vector<vector<MarkedUpChunk>> text_markup_reflow_impl(
    TextReflowInfo const& reflow_info, string_view text ) {
  vector<vector<MarkedUpChunk>> reflowed;

  // (1)
  auto const words = util::split_strip_any( text, " \t\r\n" );

  // (2)
  auto const text_one_line = util::join( words, " " );

  // (3)
  UNWRAP_CHECK( mk_texts, parse_markup( text_one_line ) );
  // Can have either zero chunks (input is either empty or con-
  // tains only space characters) or, if not, then exactly one
  // chunk.
  if( mk_texts.chunks.empty() ) return reflowed;
  CHECK( mk_texts.chunks.size() == 1 );
  auto mk_text = std::move( mk_texts.chunks[0] );

  // (4)
  auto non_mk_text =
      rl::all( mk_text ).map_L( string( _.text ) ).accumulate();

  // (5)
  auto wrapped =
      util::wrap_text( non_mk_text, reflow_info.max_cols );

  // (6)
  reflowed.resize( wrapped.size() );
  // mk_pos tracks the current item in mk_text and mk_char_pos
  // tracks the current character in the current mk_text element.
  // We will advance these numbers as we move through the charac-
  // ters of the wrapped lines.
  int mk_pos = 0, mk_char_pos = 0;
  for( auto p : rl::zip( wrapped, reflowed ) ) {
    auto& [line, reflowed_line] = p;
    int target_size             = int( line.size() );
    // Keep using up MarkedUpChunk elements (or parts of them)
    // until we have exausted this line of text. This loop body
    // is messy because in general we must consume partial
    // MarkedUpChunk elements in each iteration.
    while( target_size > 0 ) {
      CHECK( mk_pos < int( mk_text.size() ) );
      auto chunk_size = int( mk_text[mk_pos].text.size() );
      CHECK( mk_char_pos < chunk_size );
      auto remaining = chunk_size - mk_char_pos;
      auto to_consume =
          ( remaining <= target_size ) ? remaining : target_size;
      CHECK( to_consume > 0 );
      auto remaining_after_consume =
          chunk_size - mk_char_pos - to_consume;
      CHECK( remaining_after_consume >= 0 );
      CHECK( remaining_after_consume <= chunk_size );
      MarkedUpChunk new_mk_text = mk_text[mk_pos];
      string_view sv            = new_mk_text.text;
      sv.remove_prefix( mk_char_pos );
      sv.remove_suffix( remaining_after_consume );
      new_mk_text.text = string( sv );
      reflowed_line.push_back( new_mk_text );
      target_size -= to_consume;
      CHECK( target_size >= 0 );
      mk_char_pos += to_consume;
      CHECK( mk_char_pos <= chunk_size );
      if( mk_char_pos == chunk_size ) {
        mk_pos++;
        mk_char_pos = 0;
      }
    }
    // We've exhausted a line; however, if we are not on the last
    // line then that means we need to advance to the next line
    // which we will do in the next for loop iteration; However,
    // we must advance the mk_char_pos by one char (space) be-
    // cause there are spaces in place of newline chars in tex-
    // t_one_line.
    if( mk_pos < int( mk_text.size() ) ) {
      auto chunk_size = int( mk_text[mk_pos].text.size() );
      CHECK( mk_char_pos < chunk_size );
      mk_char_pos++;
      if( mk_char_pos == chunk_size ) {
        mk_pos++;
        mk_char_pos = 0;
      }
    }
  }
  CHECK( mk_pos == int( mk_text.size() ),
         "mk_pos: {}, mk_text.size(): {}", mk_pos,
         mk_text.size() );
  CHECK( mk_char_pos == 0 );
  CHECK( reflowed.size() == wrapped.size() );

  return reflowed;
}

} // namespace

/****************************************************************
** TextMarkupInfo
*****************************************************************/
pixel TextMarkupInfo::default_normal_color() {
  return config_ui.dialog_text.normal;
}

pixel TextMarkupInfo::default_highlight_color() {
  return config_ui.dialog_text.highlighted;
}

/****************************************************************
** Public API
*****************************************************************/
void render_text_markup( rr::Renderer& renderer, point where,
                         e_font font,
                         rr::TextLayout const& text_layout,
                         TextMarkupInfo const& info,
                         std::string_view text ) {
  (void)font; // TODO
  // The color will be set later.
  rr::Typer typer =
      renderer.typer( text_layout, where, pixel{} );
  UNWRAP_CHECK( mk_texts, parse_markup( text ) );
  render_lines_markup( typer, mk_texts.chunks, info );
}

void render_text( rr::Renderer& renderer, point where,
                  e_font font, pixel color,
                  std::string_view text ) {
  (void)font; // TODO
  // The color will be set later.
  rr::Typer typer = renderer.typer( where, pixel{} );
  render_lines( typer, color, base::str_split( text, '\n' ) );
}

void render_text( rr::Renderer& renderer, point where,
                  std::string_view text, pixel color ) {
  render_text( renderer, where, font::standard(), color, text );
}

void render_text_markup_reflow(
    rr::Renderer& renderer, point where, e_font font,
    TextMarkupInfo const& markup_info,
    TextReflowInfo const& reflow_info, string_view text ) {
  (void)font; // TODO
  vector<vector<MarkedUpChunk>> markedup_reflowed =
      text_markup_reflow_impl( reflow_info, text );
  // The color will be set later.
  rr::Typer typer = renderer.typer( where, pixel{} );
  render_lines_markup( typer, markedup_reflowed, markup_info );
}

Delta rendered_text_size( rr::ITextometer const& textometer,
                          rr::TextLayout const& text_layout,
                          TextReflowInfo const& reflow_info,
                          string_view text ) {
  vector<vector<MarkedUpChunk>> lines =
      text_markup_reflow_impl( reflow_info, text );
  Delta res;
  res.h = H{ textometer.font_height() * int( lines.size() ) };
  for( vector<MarkedUpChunk> const& line : lines ) {
    int line_width = 0;
    for( MarkedUpChunk const& segment : line )
      line_width +=
          textometer
              .dimensions_for_line( text_layout, segment.text )
              .w +
          textometer.spacing_between_chars( text_layout );
    line_width -=
        textometer.spacing_between_chars( text_layout );
    line_width = std::max( line_width, 0 );
    res.w      = std::max( res.w, line_width );
  }
  if( lines.size() > 0 )
    // Add spacing between lines.
    res.h += textometer.spacing_between_lines( text_layout ) *
             ( lines.size() - 1 );
  return res;
}

Delta rendered_text_size_no_reflow(
    rr::ITextometer const& textometer,
    rr::TextLayout const& text_layout, string_view text ) {
  string no_markup = remove_markup( text );
  vector<string_view> lines =
      util::split_on_any( no_markup, "\r\n" );
  Delta res;
  res.h = H{ textometer.font_height() * int( lines.size() ) };
  for( string_view line : lines ) {
    int const line_width =
        textometer
            .dimensions_for_line( text_layout, string( line ) )
            .w;
    res.w = std::max( res.w, line_width );
  }
  return res;
}

void render_text_overlay_with_anchor(
    rr::Renderer& renderer, rr::TextLayout const& text_layout,
    vector<string> const& lines, oriented_point const op,
    pixel const fg_color, pixel const bg_color,
    int const scale ) {
  if( scale == 0 ) return;
  rect const info_region = [&] {
    rr::Typer const dummy_typer = renderer.typer( text_layout );
    size const info_region_size = [&] {
      auto delta_for = [&]( string_view const text ) {
        return dummy_typer.dimensions_for_line( text );
      };
      size res;
      for( auto const& line : lines ) {
        size const delta = delta_for( line );
        res.w            = std::max( res.w, delta.w );
        res.h += delta.h;
        ++res.h; // space between lines.
      }
      // Leave room for a one (scaled) pixel border.
      res.w += 2;
      res.h += 2;
      return res * scale;
    }();

    point const info_region_start =
        gfx::find_placement( op, info_region_size );

    return rect{ .origin = info_region_start,
                 .size   = info_region_size };
  }();

  rr::Painter painter = renderer.painter();
  painter.draw_solid_rect( info_region, bg_color );
  auto const text_origin = [&] {
    auto res = info_region.origin;
    // Account for the (scaled) border.
    res.x += 1 * scale;
    res.y += 1 * scale;
    ++res.x;
    ++res.y;
    return res / scale;
  }();
  SCOPED_RENDERER_MOD_MUL( painter_mods.repos.scale, scale );
  rr::Typer typer =
      renderer.typer( text_layout, text_origin, fg_color );
  for( auto const& line : lines ) {
    typer.write( line );
    typer.newline();
  }
}

void text_cutoff( rr::ITextometer const& textometer,
                  rr::TextLayout const& text_layout,
                  int const max_pixel_width,
                  string_view const suffix,
                  string_view const fallback, string& text ) {
  auto const width_good = [&]( string const& candidate ) {
    return textometer
               .dimensions_for_line( text_layout, candidate )
               .w <= max_pixel_width;
  };

  if( width_good( text ) ) return;

  while( !text.empty() ) {
    text.pop_back();
    string candidate = format( "{}{}", text, suffix );
    if( width_good( candidate ) ) {
      text = std::move( candidate );
      return;
    }
  }

  text = string( fallback );
}

} // namespace rn
