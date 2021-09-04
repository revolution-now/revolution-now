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
#include "gfx.hpp"
#include "init.hpp"
#include "logger.hpp"
#include "ttf.hpp"

// base
#include "base/hash.hpp"
#include "base/keyval.hpp"
#include "base/lambda.hpp"
#include "base/range-lite.hpp"

// base-util
#include "base-util/algo.hpp"
#include "base-util/string.hpp"

// Abseil
#include "absl/strings/str_split.h"

// C++ standard library
#include <unordered_map>

using namespace std;

namespace rn {

namespace rl = ::base::rl;

namespace {

/****************************************************************
** Caching
*****************************************************************/
// This struct must contain all of the options that can affect
// the generation of a texture from any of the possible inputs
// (or parameters) in any function in this module.
struct TextCacheKey {
  string text; // may contain markup.
  e_font font;
  Color  color;
  // If this is active then we are in markup mode.
  maybe<TextMarkupInfo> markup_info;
  // If this is active then we are in reflow mode.
  maybe<TextReflowInfo> reflow_info;
  // !! Update std::hash below when adding new fields.

  // Adds some member functions to make this struct a cache key.
  MAKE_CACHE_KEY( TextCacheKey, text, font, color, markup_info,
                  reflow_info );
};
NOTHROW_MOVE( TextCacheKey );

} // namespace
} // namespace rn

namespace std {

template<>
struct hash<::rn::TextCacheKey> {
  auto operator()( ::rn::TextCacheKey const& o ) const noexcept {
    size_t h = hash<string>{}( o.text );
    base::hash_combine( h, o.font );
    base::hash_combine( h, o.color );
    base::hash_combine( h, o.markup_info );
    base::hash_combine( h, o.reflow_info );
    return h;
  }
};

} // namespace std

namespace rn {
namespace {

// Need pointer stability in this map.
unordered_map<TextCacheKey, Texture> g_text_cache;
constexpr int const k_max_text_cache_size = 2000;

maybe<Texture const&> text_cache_lookup(
    TextCacheKey const& key ) {
  return base::lookup( g_text_cache, key );
}

void trim_text_cache() {
  if( g_text_cache.size() < k_max_text_cache_size ) return;
  // First check if the cache has reached the max size; if so,
  // then cut it down to half size using the global Texture ID
  // (monotonically increasing) to decide which ones are the
  // oldest and remove them.
  vector<int> ids;
  ids.reserve( g_text_cache.size() );
  for( auto const& p : g_text_cache )
    ids.push_back( p.second.id() );
  util::sort( ids );
  unordered_set<int> to_remove(
      ids.begin(), ids.begin() + ( k_max_text_cache_size / 2 ) );
  vector<TextCacheKey const*> keys_to_remove;
  keys_to_remove.reserve( to_remove.size() );
  for( auto const& [k, v] : g_text_cache )
    if( to_remove.contains( v.id() ) )
      keys_to_remove.push_back( &k );
  lg.debug( "removing {} entries from text cache.",
            keys_to_remove.size() );
  for( auto* k : keys_to_remove ) g_text_cache.erase( *k );
}

Texture const& insert_into_text_cache( TextCacheKey&& key,
                                       Texture&&      tx ) {
  trim_text_cache();
  auto it = g_text_cache.insert( { key, std::move( tx ) } );
  return it.first->second;
}

#define RETURN_IF_CACHED( ... )                                  \
  TextCacheKey cache_key{ __VA_ARGS__ };                         \
  auto         maybe_cached_tx = text_cache_lookup( cache_key ); \
  if( maybe_cached_tx ) return std::move( *maybe_cached_tx );

#define CACHE_AND_RETURN( tx )                           \
  return insert_into_text_cache( std::move( cache_key ), \
                                 std::move( tx ) );

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
  string_view text{};
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
            { string_view( start, at_sign - start ),
              curr_style } );
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
Texture render( e_font font, Color fg, string_view txt ) {
  return ttf_render_text_line_uncached( font, fg, txt );
}

Texture render_markup( e_font font, MarkedUpText const& mk,
                       TextMarkupInfo const& info ) {
  if( mk.style.highlight )
    return ttf_render_text_line_uncached( font, info.highlight,
                                          mk.text );
  if( mk.style.shadow ) {
    Texture main_text = ttf_render_text_line_uncached(
        font, info.shadowed_text_color, mk.text );
    Texture res =
        Texture::create( main_text.size() + 1_w + 1_h );
    clear_texture_transparent( res );
    Texture shadow = ttf_render_text_line_uncached(
        font, info.shadowed_shadow_color, mk.text );
    shadow.copy_to( res, Coord( 0_x, 1_y ) );
    shadow.copy_to( res, Coord( 1_x, 0_y ) );
    main_text.copy_to( res, Coord{} );
    return res;
  }
  // Here we assume now special style info, just do default ren-
  // dering.
  return ttf_render_text_line_uncached( font, info.normal,
                                        mk.text );
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
  auto txs = rl::all( mks )
                 .remove_if_L( _.text.size() == 0 )
                 .map( renderer )
                 .to_vector();

  CHECK( !txs.empty() );
  auto w       = rl::all( txs ).map_L( _.size().w ).accumulate();
  auto maybe_h = rl::all( txs ).map_L( _.size().h ).max();
  CHECK( maybe_h );
  auto  res = create_texture_transparent( { w, *maybe_h } );
  Coord where{};
  for( auto const& tx : txs ) {
    copy_texture( tx, res, where );
    where += tx.size().w;
  }
  return res;
}

Texture render_lines( e_font font, Color fg,
                      vector<string> const& txt ) {
  auto renderer = [&]( string_view line ) {
    return render_line( font, fg, line );
  };
  auto txs     = util::map( renderer, txt );
  auto h       = rl::all( txs ).map_L( _.size().h ).accumulate();
  auto maybe_w = rl::all( txs ).map_L( _.size().w ).max();
  CHECK( maybe_w );
  auto  res = create_texture_transparent( { h, *maybe_w } );
  Coord where{};
  for( auto const& tx : txs ) {
    copy_texture( tx, res, where );
    where += tx.size().h;
  }
  return res;
}

Texture render_lines_markup(
    e_font font, vector<vector<MarkedUpText>> const& mk_text,
    TextMarkupInfo const& info ) {
  auto renderer = [&]( auto const& mks ) {
    return render_line_markup( font, mks, info );
  };
  auto txs     = util::map( renderer, mk_text );
  auto h       = rl::all( txs ).map_L( _.size().h ).accumulate();
  auto maybe_w = rl::all( txs ).map_L( _.size().w ).max();
  CHECK( maybe_w );
  auto  res = create_texture_transparent( { h, *maybe_w } );
  Coord where{};
  for( auto const& tx : txs ) {
    copy_texture( tx, res, where );
    where += tx.size().h;
  }
  return res;
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
//     7) Render marked up lines.

Texture render_text_markup_reflow_impl(
    e_font font, TextMarkupInfo const& markup_info,
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
  return render_lines_markup( font, reflowed, markup_info );
}

void init_text() {}

void cleanup_text() {
  for( auto& p : g_text_cache ) p.second.free();
}

REGISTER_INIT_ROUTINE( text );

} // namespace

/****************************************************************
** Public API
*****************************************************************/
// All API methods here that return textures employ caching, and
// furthermore that caching happens at the level of these func-
// tions; functions called herein will not do caching themselves.

Texture const& render_text_markup( e_font                font,
                                   TextMarkupInfo const& info,
                                   std::string_view      text ) {
  RETURN_IF_CACHED(
      /*text=*/string( text ),
      /*font=*/font,
      /*color=*/info.normal,
      /*markup_info=*/info,
      /*reflow_info=*/nothing //
  );
  auto res =
      render_lines_markup( font, parse_text( text ), info );
  CACHE_AND_RETURN( res );
}

Texture const& render_text( e_font font, Color color,
                            std::string_view text ) {
  RETURN_IF_CACHED(
      /*text=*/string( text ),
      /*font=*/font,
      /*color=*/color,
      /*markup_info=*/nothing,
      /*reflow_info=*/nothing //
  );
  auto res =
      render_lines( font, color, absl::StrSplit( text, '\n' ) );
  CACHE_AND_RETURN( res );
}

Texture const& render_text( std::string_view text,
                            Color            color ) {
  // Caching will happen in the following method.
  return render_text( font::standard(), color, text );
}

Texture const& render_text_markup_reflow(
    e_font font, TextMarkupInfo const& markup_info,
    TextReflowInfo const& reflow_info, string_view text ) {
  RETURN_IF_CACHED(
      /*text=*/string( text ),
      /*font=*/font,
      /*color=*/markup_info.normal,
      /*markup_info=*/markup_info,
      /*reflow_info=*/reflow_info //
  );
  auto res = render_text_markup_reflow_impl( font, markup_info,
                                             reflow_info, text );
  CACHE_AND_RETURN( res );
}

/****************************************************************
** Debugging
*****************************************************************/
int text_cache_size() { return g_text_cache.size(); }

/****************************************************************
** Testing
*****************************************************************/
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

  TextMarkupInfo info{ .normal    = Color::white(),
                       .highlight = Color::red() };

  auto const& tx1 =
      render_text_markup( font::standard(), info, msg );
  // auto tx1 =
  //    render_text( font::standard(), Color::white(), msg );
  copy_texture( tx1, Texture::screen(), { 50_y, 100_x } );

  auto const& tx2 = render_text_markup_reflow(
      font::standard(), info, { 50 }, msg2 );
  copy_texture( tx2, Texture::screen(), { 200_y, 100_x } );

  // ::SDL_RenderPresent( g_renderer );

  // input::wait_for_q();
}

} // namespace rn

namespace std {

size_t hash<::rn::TextMarkupInfo>::operator()(
    ::rn::TextMarkupInfo const& o ) const noexcept {
  size_t h = hash<rn::Color>{}( o.normal );
  base::hash_combine( h, o.highlight );
  base::hash_combine( h, o.shadowed_text_color );
  base::hash_combine( h, o.shadowed_shadow_color );
  return h;
}

} // namespace std
