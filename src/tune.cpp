/****************************************************************
**tune.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-05-27.
*
* Description: Manages individual pieces of music in the game.
*
*****************************************************************/
#include "tune.hpp"

// Revolution Now
#include "config-files.hpp"
#include "errors.hpp"
#include "init.hpp"
#include "logging.hpp"
#include "rand.hpp"
#include "ranges.hpp"

// base-util
#include "base-util/algo.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

// C++ standard library
#include <limits>
#include <regex>
#include <tuple>

using namespace std;

namespace rn {

namespace {

absl::flat_hash_map<TuneId, Tune const*> g_tunes;

using DimensionTuple = tuple<e_tune_tempo,           //
                             e_tune_genre,           //
                             e_tune_culture,         //
                             e_tune_instrumentation, //
                             e_tune_sentiment,       //
                             e_tune_key,             //
                             e_tune_tonality,        //
                             e_tune_epoch            //
                             >;
constexpr auto k_num_dimensions =
    tuple_size<DimensionTuple>::value;

// Return a list of pairs where there is one pair for each tune,
// and each pair gives the tune ID and the score representing how
// different it is (distance) from the given set of dimensions.
// The result will be sorted from most similar to most different.
Vec<pair<TuneId, int>> tune_difference_scores(
    Opt<e_tune_tempo>           tempo,           //
    Opt<e_tune_genre>           genre,           //
    Opt<e_tune_culture>         culture,         //
    Opt<e_tune_instrumentation> instrumentation, //
    Opt<e_tune_sentiment>       sentiment,       //
    Opt<e_tune_key>             key,             //
    Opt<e_tune_tonality>        tonality,        //
    Opt<e_tune_epoch>           epoch            //
) {
  vector<pair<TuneId, int /*score*/>> scores;
  for( auto const& [id, tune_ptr] : g_tunes ) {
    auto const& tune  = *tune_ptr;
    int         score = 0;
    if( tempo.has_value() && tune.tempo != tempo.value() )
      score++;
    if( genre.has_value() && tune.genre != genre.value() )
      score++;
    if( culture.has_value() && tune.culture != culture.value() )
      score++;
    if( instrumentation.has_value() &&
        tune.instrumentation != instrumentation.value() )
      score++;
    if( sentiment.has_value() &&
        tune.sentiment != sentiment.value() )
      score++;
    if( key.has_value() && tune.key != key.value() ) score++;
    if( tonality.has_value() &&
        tune.tonality != tonality.value() )
      score++;
    if( epoch.has_value() && tune.epoch != epoch.value() )
      score++;
    scores.emplace_back( id, score );
  }
  // Sort from most similar to most different.
  util::sort_by_key( scores, L( _.second ) );
  return scores;
}

TuneId gen_tune_id() {
  static int next_id = 12345;
  return TuneId( next_id++ );
}

void init_tunes() {
  if( config_music.tunes.size() == 0 )
    logger->warn( "Tune list is empty." );
  absl::flat_hash_set<string> stems;

  int    idx = 0;
  string rx  = "[a-z0-9-]*";
  regex  rx_compiled( rx );
  for( auto const& tune : config_music.tunes ) {
    g_tunes.emplace( gen_tune_id(), &tune );
    CHECK( regex_match( tune.stem, rx_compiled ),
           "tune stem {} must match regex {}", tune.stem, rx );
    CHECK( !stems.contains( tune.stem ),
           "stem {} appears in more than one tune.", tune.stem );
    CHECK( !tune.stem.empty(),
           "stem for tune #{} (zero-based) is empty.", idx );
    CHECK( !tune.display_name.empty(),
           "display_name for tune {} is empty.", tune.stem );
    CHECK( !tune.description.empty(),
           "description for tune {} is empty.", tune.stem );
    stems.insert( tune.stem );
    idx++;
  }
}

void clean_tunes() {}

} // namespace

//
REGISTER_INIT_ROUTINE( tunes, init_tunes, clean_tunes );

Vec<TuneId> all_tunes() {
  Vec<TuneId> res;
  res.reserve( g_tunes.size() );
  for( auto const& [id, tune_ptr] : g_tunes ) {
    (void)tune_ptr;
    res.push_back( id );
  }
  return res;
}

string const& tune_display_name_from_id( TuneId id ) {
  CHECK( g_tunes.contains( id ) );
  return g_tunes[id]->display_name;
}

string const& tune_desc_from_id( TuneId id ) {
  CHECK( g_tunes.contains( id ) );
  return g_tunes[id]->description;
}

string const& tune_stem_from_id( TuneId id ) {
  CHECK( g_tunes.contains( id ) );
  return g_tunes[id]->stem;
}

Vec<TuneId> find_tunes(
    Opt<e_tune_tempo>           tempo,           //
    Opt<e_tune_genre>           genre,           //
    Opt<e_tune_culture>         culture,         //
    Opt<e_tune_instrumentation> instrumentation, //
    Opt<e_tune_sentiment>       sentiment,       //
    Opt<e_tune_key>             key,             //
    Opt<e_tune_tonality>        tonality,        //
    Opt<e_tune_epoch>           epoch,           //
    bool                        fuzzy_match,     //
    bool                        not_like         //
) {
  auto scores = tune_difference_scores(
      tempo, genre, culture, instrumentation, sentiment, key,
      tonality, epoch );

  size_t enabled_dimensions =
      ( tempo.has_value() ? 1 : 0 ) +           //
      ( genre.has_value() ? 1 : 0 ) +           //
      ( culture.has_value() ? 1 : 0 ) +         //
      ( instrumentation.has_value() ? 1 : 0 ) + //
      ( sentiment.has_value() ? 1 : 0 ) +       //
      ( key.has_value() ? 1 : 0 ) +             //
      ( tonality.has_value() ? 1 : 0 ) +        //
      ( epoch.has_value() ? 1 : 0 );            //
  CHECK( enabled_dimensions <= k_num_dimensions );

  if( !fuzzy_match ) {
    int target_score_for_non_fuzzy =
        not_like ? enabled_dimensions : 0;
    scores = scores |
             rv::filter(
                 LC( _.second == target_score_for_non_fuzzy ) );
  }

  if( not_like ) rg::reverse( scores );

  return scores | rv::transform( L( _.first ) );
}

Vec<TuneId> tunes_like( TuneId id ) {
  CHECK( g_tunes.contains( id ) );
  auto const& tune = *g_tunes[id];
  return find_tunes( tune.tempo,           //
                     tune.genre,           //
                     tune.culture,         //
                     tune.instrumentation, //
                     tune.sentiment,       //
                     tune.key,             //
                     tune.tonality,        //
                     tune.epoch,           //
                     /*fuzzy_match=*/true, //
                     /*not_like=*/false    //
  );
}

Vec<TuneId> tunes_not_like( TuneId id ) {
  CHECK( g_tunes.contains( id ) );
  auto const& tune = *g_tunes[id];
  return find_tunes( tune.tempo,           //
                     tune.genre,           //
                     tune.culture,         //
                     tune.instrumentation, //
                     tune.sentiment,       //
                     tune.key,             //
                     tune.tonality,        //
                     tune.epoch,           //
                     /*fuzzy_match=*/true, //
                     /*not_like=*/true     //
  );
}

TuneId random_tune() {
  return find_tunes( rng::pick_one<e_tune_tempo>(),           //
                     rng::pick_one<e_tune_genre>(),           //
                     rng::pick_one<e_tune_culture>(),         //
                     rng::pick_one<e_tune_instrumentation>(), //
                     rng::pick_one<e_tune_sentiment>(),       //
                     rng::pick_one<e_tune_key>(),             //
                     rng::pick_one<e_tune_tonality>(),        //
                     rng::pick_one<e_tune_epoch>()            //
                     )[0];
}

Vec<TuneId> random_playlist() {
  absl::flat_hash_set<TuneId> s;
  Vec<TuneId>                 res;
  logger->debug( "Generating random playlist..." );
  while( res.size() < g_tunes.size() ) {
    auto id = random_tune();
    if( s.contains( id ) ) continue;
    s.insert( id );
    res.push_back( id );
  }
  return res;
}

} // namespace rn
