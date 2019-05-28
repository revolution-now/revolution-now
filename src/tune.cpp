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

using namespace std;

namespace rn {

namespace {

absl::flat_hash_map<TuneId, Tune const*> g_tunes;

// Return a list of pairs where there is one pair for each tune,
// and each pair gives the tune ID and the score representing how
// different it is from the given tune.
Vec<pair<TuneId, int>> tune_comparison_scores(
    e_tune_tempo           tempo,           //
    e_tune_genre           genre,           //
    e_tune_culture         culture,         //
    e_tune_instrumentation instrumentation, //
    e_tune_sentiment       sentiment,       //
    e_tune_key             key,             //
    e_tune_tonality        tonality,        //
    e_tune_epoch           epoch            //
) {
  vector<pair<TuneId, int /*score*/>> scores;
  for( auto const& [id, tune_ptr] : g_tunes ) {
    auto const& tune  = *tune_ptr;
    int         score = 0;
    if( tune.tempo != tempo ) score++;
    if( tune.genre != genre ) score++;
    if( tune.culture != culture ) score++;
    if( tune.instrumentation != instrumentation ) score++;
    if( tune.sentiment != sentiment ) score++;
    if( tune.key != key ) score++;
    if( tune.tonality != tonality ) score++;
    if( tune.epoch != epoch ) score++;
    scores.emplace_back( id, score );
  }
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

Vec<TuneId> tunes_with(
    Opt<e_tune_tempo>           tempo,           //
    Opt<e_tune_genre>           genre,           //
    Opt<e_tune_culture>         culture,         //
    Opt<e_tune_instrumentation> instrumentation, //
    Opt<e_tune_sentiment>       sentiment,       //
    Opt<e_tune_key>             key,             //
    Opt<e_tune_tonality>        tonality,        //
    Opt<e_tune_epoch>           epoch            //
) {
  vector<TuneId> res;
  for( auto const& [id, tune_ptr] : g_tunes ) {
    auto const& tune = *tune_ptr;
    if( tempo.has_value() && tempo.value() != tune.tempo )
      continue;
    if( genre.has_value() && genre.value() != tune.genre )
      continue;
    if( culture.has_value() && culture.value() != tune.culture )
      continue;
    if( instrumentation.has_value() &&
        instrumentation.value() != tune.instrumentation )
      continue;
    if( sentiment.has_value() &&
        sentiment.value() != tune.sentiment )
      continue;
    if( key.has_value() && key.value() != tune.key ) continue;
    if( tonality.has_value() &&
        tonality.value() != tune.tonality )
      continue;
    if( epoch.has_value() && epoch.value() != tune.epoch )
      continue;
    // Not excluded, so include it.
    res.push_back( id );
  }
  return res;
}

Vec<TuneId> tunes_without(
    Opt<e_tune_tempo>           tempo,           //
    Opt<e_tune_genre>           genre,           //
    Opt<e_tune_culture>         culture,         //
    Opt<e_tune_instrumentation> instrumentation, //
    Opt<e_tune_sentiment>       sentiment,       //
    Opt<e_tune_key>             key,             //
    Opt<e_tune_tonality>        tonality,        //
    Opt<e_tune_epoch>           epoch            //
) {
  vector<TuneId> res;
  for( auto const& [id, tune_ptr] : g_tunes ) {
    auto const& tune = *tune_ptr;
    if( tempo.has_value() && tempo.value() == tune.tempo )
      continue;
    if( genre.has_value() && genre.value() == tune.genre )
      continue;
    if( culture.has_value() && culture.value() == tune.culture )
      continue;
    if( instrumentation.has_value() &&
        instrumentation.value() == tune.instrumentation )
      continue;
    if( sentiment.has_value() &&
        sentiment.value() == tune.sentiment )
      continue;
    if( key.has_value() && key.value() == tune.key ) continue;
    if( tonality.has_value() &&
        tonality.value() == tune.tonality )
      continue;
    if( epoch.has_value() && epoch.value() == tune.epoch )
      continue;
    // Not excluded, so include it.
    res.push_back( id );
  }
  return res;
}

Vec<TuneId> tunes_like( TuneId id ) {
  CHECK( g_tunes.contains( id ) );
  auto const& tune = *g_tunes[id];

  auto scores = tune_comparison_scores( tune.tempo,           //
                                        tune.genre,           //
                                        tune.culture,         //
                                        tune.instrumentation, //
                                        tune.sentiment,       //
                                        tune.key,             //
                                        tune.tonality,        //
                                        tune.epoch            //
  );
  util::sort_by_key( scores, L( _.second ) );
  return scores                          //
         | rv::transform( L( _.first ) ) //
         | rv::take( g_tunes.size() / 2 );
}

Vec<TuneId> tunes_not_like( TuneId id ) {
  CHECK( g_tunes.contains( id ) );
  auto const& tune = *g_tunes[id];

  auto scores = tune_comparison_scores( tune.tempo,           //
                                        tune.genre,           //
                                        tune.culture,         //
                                        tune.instrumentation, //
                                        tune.sentiment,       //
                                        tune.key,             //
                                        tune.tonality,        //
                                        tune.epoch            //
  );
  util::sort_by_key( scores, L( _.second ) );
  return scores                          //
         | rv::reverse                   //
         | rv::transform( L( _.first ) ) //
         | rv::take( g_tunes.size() / 2 );
}

TuneId random_tune() {
  auto rnd_tempo   = rng::pick_one<e_tune_tempo>();
  auto rnd_genre   = rng::pick_one<e_tune_genre>();
  auto rnd_culture = rng::pick_one<e_tune_culture>();
  auto rnd_instrumentation =
      rng::pick_one<e_tune_instrumentation>();
  auto rnd_sentiment = rng::pick_one<e_tune_sentiment>();
  auto rnd_key       = rng::pick_one<e_tune_key>();
  auto rnd_tonality  = rng::pick_one<e_tune_tonality>();
  auto rnd_epoch     = rng::pick_one<e_tune_epoch>();

  auto scores = tune_comparison_scores( rnd_tempo,           //
                                        rnd_genre,           //
                                        rnd_culture,         //
                                        rnd_instrumentation, //
                                        rnd_sentiment,       //
                                        rnd_key,             //
                                        rnd_tonality,        //
                                        rnd_epoch            //
  );
  CHECK( scores.size() > 0, "programmer error!" );
  util::sort_by_key( scores, L( _.second ) );
  return scores[0].first;
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
