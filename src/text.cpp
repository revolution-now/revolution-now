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

// Abseil
#include "absl/strings/str_split.h"

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

Texture render_markup( MarkedUpText const& mk ) {
  auto fg = mk.style.highlight ? Color::red() : Color::white();
  return render_text_line_solid( fonts::standard, fg, mk.text );
}

Texture render_line_markup( vector<MarkedUpText> const& mks ) {
  vector<Texture> txs =
      mks                                        //
      | rv::remove_if( L( _.text.size() == 0 ) ) //
      | rv::transform( render_markup );
  CHECK( !txs.empty() );
  auto w = rg::accumulate(
      txs | rv::transform( L( _.size().w ) ), 0_w );
  auto maybe_h =
      txs | rv::transform( L( _.size().h ) ) | maximum();
  CHECK( maybe_h );
  auto  res = create_texture_transparent( {w, *maybe_h} );
  Coord where{};
  for( auto const& tx : txs ) {
    copy_texture( tx, res, where );
    where += tx.size().w;
  }
  return res;
}

} // namespace

void render_text_markup( std::string_view text,
                         Texture const& dest, Coord coord ) {
  vector<string_view> lines = absl::StrSplit( text, '\n' );
  vector<vector<MarkedUpText>> line_frags;
  line_frags.reserve( lines.size() );
  MarkupStyle curr_style{};
  for( auto line : lines ) {
    logger->debug( "parsing line: `{}`", line );
    vector<MarkedUpText> line_mkup;
    auto                 start = line.begin();
    auto                 end   = line.end();
    while( true ) {
      auto at_sign = find( start, end, '@' );
      line_mkup.push_back(
          {string_view( start, at_sign - start ), curr_style} );
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
  auto txs = util::map( render_line_markup, line_frags );
  auto h   = rg::accumulate(
      txs | rv::transform( L( _.size().h ) ), 0_h );
  auto maybe_w =
      txs | rv::transform( L( _.size().w ) ) | maximum();
  CHECK( maybe_w );
  auto  res = create_texture_transparent( {h, *maybe_w} );
  Coord where{};
  for( auto const& tx : txs ) {
    copy_texture( tx, res, where );
    where += tx.size().h;
  }
  copy_texture( res, dest, coord );
}

// Will flatten the text onto one line, then wrap it to within
// `max_cols` columns.
void render_text_markup_wrap( std::string_view text,
                              Texture const& dest, Coord coord,
                              int max_cols ) {
  /*
   * Steps:
   *   1) Split text into lines
   *   2) Join lines into one line separated by spaces
   *   3) Parse string
   *   4) Recover original string
   *   5) Wrap original string
   *   6) Re-flow the marked up lines to have the same length
   *      as the wrapped ones.
   *   7) Render lines as above.
   */
  (void)text;
  (void)dest;
  (void)coord;
  (void)max_cols;
}

void text_render_test() {
  char const* msg =
      "Ask not\n"
      "what your @[H]country@[] can do for@[H] you,\n"
      "but @[]@[H]i@[]n@[H]s@[]t@[H]e@[]a@[H]d@[]\n"
      "ask what y@[H]o@[]u can do for your @[H]country!";

  render_text_markup( msg, Texture{}, {100_y, 100_x} );

  ::SDL_RenderPresent( g_renderer );

  input::wait_for_q();
}

} // namespace rn
