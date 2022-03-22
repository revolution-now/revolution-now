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
#include "logger.hpp"

// base
#include "base/range-lite.hpp"

// base-util
#include "base-util/algo.hpp"
#include "base-util/string.hpp"

// Abseil
#include "absl/strings/str_split.h"

using namespace std;

namespace rn {
namespace {

namespace rl = ::base::rl;

/****************************************************************
** Markup Parsing
*****************************************************************/
/*
 * // The below is a parsco parser for this markup DSL if it will
 * // ever be of interest.
 *
 * struct markup_lang {};
 *
 * struct text {
 *   text( string s_ ) : s( std::move( s_ ) ) {}
 *   string s;
 * };
 *
 * struct markup {
 *   markup( string s ) : contents( std::move( s ) ) {}
 *   string contents;
 * };
 *
 * using item = base::variant<text, markup>;
 *
 * struct doc {
 *   doc( vector<item> v ) : items( std::move( v ) ) {}
 *   vector<item> items;
 * };
 *
 * parser<text> parser_for( lang<markup_lang>, tag<text> ) {
 *   auto at = [] { return str( "@@" ) >> ret( '@' ); };
 *   return emplace<text>( many1_L( not_of( "@" ) | at() ) );
 * }
 *
 * parser<markup> parser_for( lang<markup_lang>, tag<markup> ) {
 *   return construct<markup>(
 *       chr( '@' ) >>
 *       bracketed( '[', many( one_of, "HS" ), ']' ) );
 * }
 *
 * parser<doc> parser_for( lang<markup_lang>, tag<doc> ) {
 *   return construct<doc>(
 *       exhaust( many_type<markup_lang, item>() ) );
 * }
 */

struct MarkupStyle {
  bool highlight{ false };
  bool shadow{ false };
};
NOTHROW_MOVE( MarkupStyle );

struct MarkedUpText {
  string      text{};
  MarkupStyle style{};
};
NOTHROW_MOVE( MarkedUpText );

auto parse_markup( string_view sv ) -> maybe<MarkupStyle> {
  if( sv.size() == 0 ) return MarkupStyle{};
  if( sv.size() != 1 ) return nothing; // parsing failed
  switch( sv[0] ) {
    case '@': return MarkupStyle{};
    case 'H': return MarkupStyle{ .highlight = true };
    case 'S': return MarkupStyle{ .shadow = true };
  }
  return nothing; // parsing failed.
}

vector<vector<MarkedUpText>> parse_text( string_view text ) {
  vector<string_view> lines = absl::StrSplit( text, '\n' );
  vector<vector<MarkedUpText>> line_frags;
  line_frags.reserve( lines.size() );
  MarkupStyle curr_style{};
  for( auto line : lines ) {
    vector<MarkedUpText> line_mkup;
    auto                 start = line.begin();
    auto                 end   = line.end();
    while( true ) {
      auto at_sign = find( start, end, '@' );
      if( at_sign - start > 0 )
        line_mkup.push_back(
            { string( start, at_sign - start ), curr_style } );
      if( at_sign == end ) break;
      CHECK( end - at_sign > 1, "internal parser error" );
      CHECK( *at_sign == '@', "internal parser error" );
      CHECK( end - at_sign >= 3,
             "markup syntax error: `@` must be followed by "
             "`[...]`" );
      CHECK( *( at_sign + 1 ) == '[',
             "markup syntax error: missing `[` after `@`" );
      auto markup_start = at_sign + 2;
      auto markup_end   = find( markup_start, end, ']' );
      CHECK( markup_end != end,
             "markup syntax error: missing closing `]`" );
      auto markup =
          string_view( markup_start, markup_end - markup_start );
      auto maybe_markup = parse_markup( markup );
      CHECK( maybe_markup, "failed to parse markup: `{}`",
             markup );
      curr_style = *maybe_markup;
      start      = markup_end + 1;
    }
    line_frags.emplace_back( std::move( line_mkup ) );
  }
  return line_frags;
}

/****************************************************************
** Rendering
*****************************************************************/
void render_impl( rr::Typer& typer, gfx::pixel color,
                  string_view text ) {
  typer.set_color( color );
  typer.write( text );
}

void render_markup( rr::Typer& typer, MarkedUpText const& mk,
                    TextMarkupInfo const& info ) {
  if( mk.style.highlight ) {
    render_impl( typer, info.highlight, mk.text );
    return;
  }

  if( mk.style.shadow ) {
    {
      rr::Typer typer_shadow =
          typer.with_frame_offset( gfx::size{ .w = 1, .h = 0 } );
      render_impl( typer_shadow, info.shadowed_shadow_color,
                   mk.text );
    }
    {
      rr::Typer typer_shadow =
          typer.with_frame_offset( gfx::size{ .w = 0, .h = 1 } );
      render_impl( typer_shadow, info.shadowed_shadow_color,
                   mk.text );
    }
    {
      rr::Typer typer_shadow =
          typer.with_frame_offset( gfx::size{ .w = 1, .h = 1 } );
      render_impl( typer_shadow, info.shadowed_shadow_color,
                   mk.text );
    }
    render_impl( typer, info.shadowed_text_color, mk.text );
    return;
  }

  // Here we assume now special style info, just do default ren-
  // dering.
  return render_impl( typer, info.normal, mk.text );
}

void render_line( rr::Typer& typer, gfx::pixel fg,
                  string_view text ) {
  return render_impl( typer, fg, text );
}

void render_line_markup( rr::Typer&                  typer,
                         vector<MarkedUpText> const& mks,
                         TextMarkupInfo const&       info ) {
  for( MarkedUpText const& mut : mks )
    if( !mut.text.empty() ) //
      render_markup( typer, mut, info );
}

void render_lines( rr::Typer& typer, gfx::pixel fg,
                   vector<string> const& txt ) {
  for( string const& line : txt ) {
    render_line( typer, fg, line );
    render_impl( typer, gfx::pixel{}, "\n" );
  }
}

void render_lines_markup(
    rr::Typer&                          typer,
    vector<vector<MarkedUpText>> const& mk_text,
    TextMarkupInfo const&               info ) {
  for( vector<MarkedUpText> const& muts : mk_text ) {
    render_line_markup( typer, muts, info );
    render_impl( typer, gfx::pixel{}, "\n" );
    // If there was any shadowed text on this line then we need
    // to add one additional pixel (vertically) of space when
    // moving to the next line.
    bool has_shadow = std::any_of(
        muts.begin(), muts.end(), []( MarkedUpText const& mut ) {
          return mut.style.shadow;
        } );
    if( has_shadow )
      typer.move_frame_by( gfx::size{ .w = 0, .h = 1 } );
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
//     3) Parse long line for markup, yielding Vec<MarkedUpText>
//     4) Extract text from markup results.  This should be the
//        line from #2 but without any markup.
//     5) Wrap the line from #4
//     6) Re-flow the marked up line from #3 into lines of length
//        corresponding to the wrapped lines from #5, resulting
//        in a vector<vector<MarkedUpText>>.

vector<vector<MarkedUpText>> text_markup_reflow_impl(
    TextReflowInfo const& reflow_info, string_view text ) {
  // (1)
  auto words = util::split_strip_any( text, " \t\r\n" );

  // (2)
  auto text_one_line = util::join( words, " " );

  // (3)
  auto mk_texts = parse_text( text_one_line );
  CHECK( mk_texts.size() == 1 );
  auto mk_text = std::move( mk_texts[0] );

  // (4)
  auto non_mk_text =
      rl::all( mk_text ).map_L( string( _.text ) ).accumulate();

  // (5)
  auto wrapped =
      util::wrap_text( non_mk_text, reflow_info.max_cols );

  // (6)
  vector<vector<MarkedUpText>> reflowed;
  reflowed.resize( wrapped.size() );
  // mk_pos tracks the current item in mk_text and mk_char_pos
  // tracks the current character in the current mk_text element.
  // We will advance these numbers as we move through the charac-
  // ters of the wrapped lines.
  int mk_pos = 0, mk_char_pos = 0;
  for( auto p : rl::zip( wrapped, reflowed ) ) {
    auto& [line, reflowed_line] = p;
    int target_size             = int( line.size() );
    // Keep using up MarkedUpText elements (or parts of them)
    // until we have exausted this line of text. This loop body
    // is messy because in general we must consume partial
    // MarkedUpText elements in each iteration.
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
      MarkedUpText new_mk_text = mk_text[mk_pos];
      string_view  sv          = new_mk_text.text;
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
** Public API
*****************************************************************/
void render_text_markup( rr::Renderer& renderer,
                         gfx::point where, e_font font,
                         TextMarkupInfo const& info,
                         std::string_view      text ) {
  (void)font; // TODO
  // The color will be set later.
  rr::Typer typer = renderer.typer( where, gfx::pixel{} );
  render_lines_markup( typer, parse_text( text ), info );
}

void render_text( rr::Renderer& renderer, gfx::point where,
                  e_font font, gfx::pixel color,
                  std::string_view text ) {
  (void)font; // TODO
  // The color will be set later.
  rr::Typer typer = renderer.typer( where, gfx::pixel{} );
  render_lines( typer, color, absl::StrSplit( text, '\n' ) );
}

void render_text( rr::Renderer& renderer, gfx::point where,
                  std::string_view text, gfx::pixel color ) {
  render_text( renderer, where, font::standard(), color, text );
}

void render_text_markup_reflow(
    rr::Renderer& renderer, gfx::point where, e_font font,
    TextMarkupInfo const& markup_info,
    TextReflowInfo const& reflow_info, string_view text ) {
  (void)font; // TODO
  vector<vector<MarkedUpText>> markedup_reflowed =
      text_markup_reflow_impl( reflow_info, text );
  // The color will be set later.
  rr::Typer typer = renderer.typer( where, gfx::pixel{} );
  render_lines_markup( typer, markedup_reflowed, markup_info );
}

string remove_markup( string_view text ) {
  string res;
  // The result will always be the same or smaller.
  res.reserve( text.size() );
  int pos = 0;
  while( pos < int( text.size() ) ) {
    char c = text[pos++];
    if( c != '@' ) {
      res.push_back( c );
      continue;
    }
    while( pos < int( text.size() ) && text[pos++] != ']' ) {}
  }
  return res;
}

Delta rendered_text_size( TextReflowInfo const& reflow_info,
                          string_view           text ) {
  vector<vector<MarkedUpText>> lines =
      text_markup_reflow_impl( reflow_info, text );
  gfx::size const kCharSize =
      rr::rendered_text_line_size_pixels( "X" );
  Delta res;
  res.h = H{ kCharSize.h * int( lines.size() ) };
  for( vector<MarkedUpText> const& line : lines ) {
    W line_width{};
    for( MarkedUpText const& segment : line )
      line_width +=
          W{ int( segment.text.size() ) * kCharSize.w };
    res.w = std::max( res.w, line_width );
  }
  return res;
}

} // namespace rn
