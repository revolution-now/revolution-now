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
#include "aliases.hpp"
#include "errors.hpp"
#include "fonts.hpp"
#include "input.hpp"
#include "logging.hpp"
#include "ranges.hpp"
#include "screen.hpp"

// base-util
#include "base-util/algo.hpp"
#include "base-util/misc.hpp"
#include "base-util/string.hpp"

// Abseil
#include "absl/strings/str_split.h"

// Range-v3
#include "range/v3/numeric/accumulate.hpp"
#include "range/v3/view/remove_if.hpp"
#include "range/v3/view/transform.hpp"
#include "range/v3/view/zip.hpp"

using namespace std;

namespace rn {

namespace {

struct MarkupStyle {
  bool highlight{false};
};

struct MarkedUpText {
  string_view text{};
  MarkupStyle style{};
};

auto parse_markup( string_view sv ) -> Opt<MarkupStyle> {
  if( sv.size() == 0 ) return MarkupStyle{};
  if( sv.size() != 1 ) return nullopt; // parsing failed
  switch( sv[0] ) {
    case '@': return MarkupStyle{};
    case 'H': return MarkupStyle{/*highlight=*/true};
  }
  return nullopt; // parsing failed.
}

Vec<Vec<MarkedUpText>> parse_text( string_view text ) {
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
            {string_view( start, at_sign - start ),
             curr_style} );
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

Texture render( e_font font, Color fg, string_view txt ) {
  return render_text_line_solid( font, fg, txt );
}

Texture render_markup( e_font font, MarkedUpText const& mk,
                       TextMarkupInfo const& info ) {
  auto fg = mk.style.highlight ? info.highlight : info.normal;
  return render_text_line_solid( font, fg, mk.text );
}

Texture render_line( e_font font, Color fg, string_view txt ) {
  return render( font, fg, txt );
}

Texture render_line_markup( e_font                      font,
                            vector<MarkedUpText> const& mks,
                            TextMarkupInfo const&       info ) {
  auto renderer = [&]( MarkedUpText const& mk ) {
    return render_markup( font, mk, info );
  };
  vector<Texture> txs =
      mks                                        //
      | rv::remove_if( L( _.text.size() == 0 ) ) //
      | rv::transform( renderer );
  CHECK( !txs.empty() );
  auto w = rg::accumulate(
      txs | rv::transform( L( _.size().w ) ), 0_w );
  auto [has_h, maybe_h] =
      txs | rv::transform( L( _.size().h ) ) | maximum();
  CHECK( has_h );
  auto  res = create_texture_transparent( {w, maybe_h} );
  Coord where{};
  for( auto const& tx : txs ) {
    copy_texture( tx, res, where );
    where += tx.size().w;
  }
  return res;
}

Texture render_lines( e_font font, Color fg,
                      Vec<Str> const& txt ) {
  auto renderer = [&]( string_view line ) {
    return render_line( font, fg, line );
  };
  auto txs = util::map( renderer, txt );
  auto h   = rg::accumulate(
      txs | rv::transform( L( _.size().h ) ), 0_h );
  auto [has_w, maybe_w] =
      txs | rv::transform( L( _.size().w ) ) | maximum();
  CHECK( has_w );
  auto  res = create_texture_transparent( {h, maybe_w} );
  Coord where{};
  for( auto const& tx : txs ) {
    copy_texture( tx, res, where );
    where += tx.size().h;
  }
  return res;
}

Texture render_lines_markup(
    e_font font, Vec<Vec<MarkedUpText>> const& mk_text,
    TextMarkupInfo const& info ) {
  auto renderer = [&]( auto const& mks ) {
    return render_line_markup( font, mks, info );
  };
  auto txs = util::map( renderer, mk_text );
  auto h   = rg::accumulate(
      txs | rv::transform( L( _.size().h ) ), 0_h );
  auto [has_w, maybe_w] =
      txs | rv::transform( L( _.size().w ) ) | maximum();
  CHECK( has_w );
  auto  res = create_texture_transparent( {h, maybe_w} );
  Coord where{};
  for( auto const& tx : txs ) {
    copy_texture( tx, res, where );
    where += tx.size().h;
  }
  return res;
}

} // namespace

Texture render_text_markup( e_font                font,
                            TextMarkupInfo const& info,
                            std::string_view      text ) {
  return render_lines_markup( font, parse_text( text ), info );
}

Texture render_text( e_font font, Color color,
                     std::string_view text ) {
  return render_lines( font, color,
                       absl::StrSplit( text, '\n' ) );
}

Texture render_text_reflow( e_font font, Color fg,
                            std::string_view text,
                            int              max_cols ) {
  return render_lines( font, fg,
                       util::wrap_text( text, max_cols ) );
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
//        in a Vec<Vec<MarkedUpText>>.
//     7) Render marked up lines.

Texture render_text_markup_reflow( e_font                font,
                                   TextMarkupInfo const& info,
                                   std::string_view      text,
                                   int max_cols ) {
  // (1)
  auto words = util::split_strip_any( text, " \t\r\n" );

  // (2)
  auto text_one_line = util::join( words, " " );

  // (3)
  auto mk_texts = parse_text( text_one_line );
  CHECK( mk_texts.size() == 1 );
  auto mk_text = std::move( mk_texts[0] );

  // (4)
  auto non_mk_text = rg::accumulate(
      mk_text | rv::transform( L( string( _.text ) ) ),
      string( "" ) );

  // (5)
  auto wrapped = util::wrap_text( non_mk_text, max_cols );

  // (6)
  vector<vector<MarkedUpText>> reflowed;
  reflowed.resize( wrapped.size() );
  // mk_pos tracks the current item in mk_text and mk_char_pos
  // tracks the current character in the current mk_text element.
  // We will advance these numbers as we move through the charac-
  // ters of the wrapped lines.
  int mk_pos = 0, mk_char_pos = 0;
  for( auto p : rv::zip( wrapped, reflowed ) ) {
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
      new_mk_text.text.remove_prefix( mk_char_pos );
      new_mk_text.text.remove_suffix( remaining_after_consume );
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

  // (7)
  return render_lines_markup( font, reflowed, info );
}

void text_render_test() {
  char const* msg =
      "Ask not\n"
      "what your @[H]country@[] can do for@[H] you,\n"
      "but @[]@[H]i@[]n@[H]s@[]t@[H]e@[]a@[H]d@[]\n"
      "ask what y@[H]o@[]u can do for your @[H]country!";

  char const* msg2 = R"(
    In order to avoid @[H]@[H]@[H]all@[] menu handler code going into the menu
    module, it will be possible for modules to register handlers
    for menu items. This @[H]will consist @[]of a void( e_menu_item )
    function to handle a click on the menu item and a bool@[H](@[]e_menu_item@[H])
    @[]function to return if the menu item should be enabled or disabled.
    The functions take arguments @[H]so that a registerer@[] can use the
    same function for multiple items and use branching within the
    function to distinguish menu items. @[H]@[H]
  )";

  TextMarkupInfo info{Color::white(), Color::red()};

  auto tx1 = render_text_markup( fonts::standard(), info, msg );
  // auto tx1 =
  //    render_text( fonts::standard(), Color::white(), msg );
  copy_texture( tx1, Texture{}, {50_y, 100_x} );

  auto tx2 = render_text_markup_reflow( fonts::standard(), info,
                                        msg2, 50 );
  // auto tx2 = render_text_reflow( fonts::standard(),
  //                               Color::white(), msg2, 50 );
  copy_texture( tx2, Texture{}, {200_y, 100_x} );

  ::SDL_RenderPresent( g_renderer );

  input::wait_for_q();
}

} // namespace rn
