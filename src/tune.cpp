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
#include "time.hpp"

// base-util
#include "base-util/algo.hpp"
#include "base-util/misc.hpp"

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

// Return a list of pairs where there is one pair for each tune,
// and each pair gives the tune ID and the score representing how
// different it is (distance) from the given set of dimensions.
// The result will be sorted from most similar to most different.
Vec<pair<TuneId, int>> tune_difference_scores(
    TuneOptDimensions dims ) {
  vector<pair<TuneId, int /*score*/>> scores;
  for( auto const& [id, tune_ptr] : g_tunes ) {
    auto const& tune  = *tune_ptr;
    int         score = 0;
    if( dims.tempo.has_value() &&
        tune.dimensions.tempo != dims.tempo.value() )
      score++;
    if( dims.genre.has_value() &&
        tune.dimensions.genre != dims.genre.value() )
      score++;
    if( dims.culture.has_value() &&
        tune.dimensions.culture != dims.culture.value() )
      score++;
    if( dims.instrumentation.has_value() &&
        tune.dimensions.instrumentation !=
            dims.instrumentation.value() )
      score++;
    if( dims.sentiment.has_value() &&
        tune.dimensions.sentiment != dims.sentiment.value() )
      score++;
    if( dims.key.has_value() &&
        tune.dimensions.key != dims.key.value() )
      score++;
    if( dims.tonality.has_value() &&
        tune.dimensions.tonality != dims.tonality.value() )
      score++;
    if( dims.epoch.has_value() &&
        tune.dimensions.epoch != dims.epoch.value() )
      score++;
    if( dims.purpose.has_value() &&
        tune.dimensions.purpose != dims.purpose.value() )
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
  if( config_music.tunes.size() == 0 ) {
    logger->error( "Tune list is empty." );
    return;
  }
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

void cleanup_tunes() {}

} // namespace

//
REGISTER_INIT_ROUTINE( tunes );

void TunePlayerInfo::log() const {
  logger->info( "TunePlayerInfo:" );
  logger->info( "  id:       {} ({})", id,
                tune_stem_from_id( id ) );
  logger->info( "  length:   {}", length );
  logger->info( "  progress: {}", progress );
}

TuneOptDimensions TuneDimensions::to_optional() const {
  return {
      tempo,           //
      genre,           //
      culture,         //
      instrumentation, //
      sentiment,       //
      key,             //
      tonality,        //
      epoch,           //
      purpose,         //
  };
}

Vec<TuneId> const& all_tunes() {
  static Vec<TuneId> tunes;
  if( tunes.empty() )
    tunes = g_tunes | rv::transform( L( _.first ) );
  return tunes;
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

TuneDimensions const& tune_dimensions( TuneId id ) {
  CHECK( g_tunes.contains( id ) );
  return g_tunes[id]->dimensions;
}

Vec<TuneId> find_tunes( TuneOptDimensions dims, bool fuzzy_match,
                        bool not_like ) {
  auto scores = tune_difference_scores( dims );

  if( !fuzzy_match ) {
    size_t enabled_dimensions =
        ( dims.tempo.has_value() ? 1 : 0 ) +           //
        ( dims.genre.has_value() ? 1 : 0 ) +           //
        ( dims.culture.has_value() ? 1 : 0 ) +         //
        ( dims.instrumentation.has_value() ? 1 : 0 ) + //
        ( dims.sentiment.has_value() ? 1 : 0 ) +       //
        ( dims.key.has_value() ? 1 : 0 ) +             //
        ( dims.tonality.has_value() ? 1 : 0 ) +        //
        ( dims.epoch.has_value() ? 1 : 0 ) +           //
        ( dims.purpose.has_value() ? 1 : 0 );          //
    CHECK( enabled_dimensions <= k_num_dimensions );

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
  return find_tunes( tune.dimensions.to_optional(), //
                     /*fuzzy_match=*/true,          //
                     /*not_like=*/false             //
  );
}

Vec<TuneId> tunes_not_like( TuneId id ) {
  CHECK( g_tunes.contains( id ) );
  auto const& tune = *g_tunes[id];
  return find_tunes( tune.dimensions.to_optional(), //
                     /*fuzzy_match=*/true,          //
                     /*not_like=*/true              //
  );
}

// The idea here is that we pick a random set of dimensions, then
// rank all tunes according to their distance from that. Then we
// take the distance of the most similar one, and pick out all
// the tunes that have that same distance, and select randomly
// among them. If we were not to choose randomly among all the
// most similar tunes of equal distance then we would run into
// problems where two tunes have the exact same dimensions and
// one would never get picked.
TuneId random_tune() {
  TuneOptDimensions dims{
      rng::pick_one<e_tune_tempo>(),           //
      rng::pick_one<e_tune_genre>(),           //
      rng::pick_one<e_tune_culture>(),         //
      rng::pick_one<e_tune_instrumentation>(), //
      rng::pick_one<e_tune_sentiment>(),       //
      rng::pick_one<e_tune_key>(),             //
      rng::pick_one<e_tune_tonality>(),        //
      rng::pick_one<e_tune_epoch>(),           //
      rng::pick_one<e_tune_purpose>()          //
  };
  auto tunes_scores = tune_difference_scores( dims );
  CHECK( !tunes_scores.empty() );
  auto first_score = tunes_scores[0].second;
  tunes_scores     = tunes_scores |
                 rv::take_while( LC( _.second == first_score ) );
  return rng::pick_one( tunes_scores ).first;
}

} // namespace rn
