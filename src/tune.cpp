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

void clean_tunes() {}

} // namespace

//
REGISTER_INIT_ROUTINE( tunes, init_tunes, clean_tunes );

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
  };
}

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

Vec<TuneId> find_tunes( TuneOptDimensions dims, bool fuzzy_match,
                        bool not_like ) {
  auto scores = tune_difference_scores( dims );

  size_t enabled_dimensions =
      ( dims.tempo.has_value() ? 1 : 0 ) +           //
      ( dims.genre.has_value() ? 1 : 0 ) +           //
      ( dims.culture.has_value() ? 1 : 0 ) +         //
      ( dims.instrumentation.has_value() ? 1 : 0 ) + //
      ( dims.sentiment.has_value() ? 1 : 0 ) +       //
      ( dims.key.has_value() ? 1 : 0 ) +             //
      ( dims.tonality.has_value() ? 1 : 0 ) +        //
      ( dims.epoch.has_value() ? 1 : 0 );            //
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

TuneId random_tune() {
  return find_tunes( TuneDimensions{
      rng::pick_one<e_tune_tempo>(),           //
      rng::pick_one<e_tune_genre>(),           //
      rng::pick_one<e_tune_culture>(),         //
      rng::pick_one<e_tune_instrumentation>(), //
      rng::pick_one<e_tune_sentiment>(),       //
      rng::pick_one<e_tune_key>(),             //
      rng::pick_one<e_tune_tonality>(),        //
      rng::pick_one<e_tune_epoch>()            //
  }
                         .to_optional() )[0];
}

} // namespace rn
