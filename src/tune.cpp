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
#include "time.hpp"

// Revolution Now (config)
#include "../config/ucl/music.inl"

// base-util
#include "base-util/algo.hpp"
#include "base-util/misc.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

// Range-v3
#include "range/v3/algorithm/reverse.hpp"
#include "range/v3/range/conversion.hpp"
#include "range/v3/view/filter.hpp"
#include "range/v3/view/take_while.hpp"
#include "range/v3/view/transform.hpp"

// C++ standard library
#include <limits>
#include <regex>
#include <tuple>

using namespace std;

namespace rn {

namespace {

absl::flat_hash_map<TuneId, Tune const*> g_tunes;

#define TUNE_DIMENSION_ADD_IF_DIFFERENT( dim ) \
  if( !util::contains( dims.dim, tune.dimensions.dim ) ) score++

// Return a list of pairs where there is one pair for each tune,
// and each pair gives the tune ID and the score representing how
// different it is (distance) from the given set of dimensions.
// The result will be sorted from most similar to most different.
Vec<pair<TuneId, int>> tune_difference_scores(
    TuneVecDimensions dims ) {
  vector<pair<TuneId, int /*score*/>> scores;
  for( auto const& [id, tune_ptr] : g_tunes ) {
    auto const& tune  = *tune_ptr;
    int         score = 0;
    EVAL( PP_MAP_SEMI( TUNE_DIMENSION_ADD_IF_DIFFERENT,
                       TUNE_DIMENSION_LIST ) )
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
    lg.error( "tune list is empty." );
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

#define OPT_TO_VEC( what )                               \
  {                                                      \
    what.has_value()                                     \
        ? Vec<std::decay_t<decltype(                     \
              what.value() )>>{ { *what } }              \
        : Vec<std::decay_t<decltype( what.value() )>> {} \
  }

TuneVecDimensions TuneOptDimensions::to_vec_dims() const {
  return { EVAL( PP_MAP_COMMAS(
      OPT_TO_VEC, EVAL( TUNE_DIMENSION_LIST ) ) ) };
}

TuneOptDimensions TuneDimensions::to_opt_dims() const {
  return { EVAL( TUNE_DIMENSION_LIST ) };
}

void TunePlayerInfo::log() const {
  lg.debug( "TunePlayerInfo:" );
  lg.debug( "  id:       {} ({})", id, tune_stem_from_id( id ) );
  lg.debug( "  length:   {}", length );
  lg.debug( "  progress: {}", progress );
}

Vec<TuneId> const& all_tunes() {
  static Vec<TuneId> tunes;
  if( tunes.empty() )
    tunes = rg::to<Vec<TuneId>>( g_tunes |
                                 rv::transform( L( _.first ) ) );
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
  return find_tunes( dims.to_vec_dims(), fuzzy_match, not_like );
}

#define TUNE_DIMENSION_COUNT_IF_ENABLED( dim ) \
  ( !dimensions.dim.empty() ? 1 : 0 )

Vec<TuneId> find_tunes( TuneVecDimensions dimensions,
                        bool fuzzy_match, bool not_like ) {
  auto scores = tune_difference_scores( dimensions );

  if( !fuzzy_match ) {
    size_t enabled_dimensions = EVAL( PP_MAP_PLUS(
        TUNE_DIMENSION_COUNT_IF_ENABLED, TUNE_DIMENSION_LIST ) );

    CHECK( enabled_dimensions <= k_num_dimensions );

    int target_score_for_non_fuzzy =
        not_like ? enabled_dimensions : 0;
    scores = rg::to<Vec<Pair<TuneId, int>>>(
        scores |
        rv::filter(
            LC( _.second == target_score_for_non_fuzzy ) ) );
  }

  if( not_like ) rg::reverse( scores );

  auto res = rg::to<Vec<TuneId>>(
      scores | rv::transform( L( _.first ) ) );
  if( fuzzy_match ) { DCHECK( !res.empty() ); }
  return res;
}

Vec<TuneId> tunes_like( TuneId id ) {
  CHECK( g_tunes.contains( id ) );
  auto const& tune = *g_tunes[id];
  return find_tunes( tune.dimensions.to_opt_dims(), //
                     /*fuzzy_match=*/true,          //
                     /*not_like=*/false             //
  );
}

Vec<TuneId> tunes_not_like( TuneId id ) {
  CHECK( g_tunes.contains( id ) );
  auto const& tune = *g_tunes[id];
  return find_tunes( tune.dimensions.to_opt_dims(), //
                     /*fuzzy_match=*/true,          //
                     /*not_like=*/true              //
  );
}

#define TUNE_DIMENSION_PICK_ONE( dim ) \
  rng::pick_one<e_tune_##dim>()

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
      //
      EVAL( PP_MAP_COMMAS( TUNE_DIMENSION_PICK_ONE,
                           EVAL( TUNE_DIMENSION_LIST ) ) ) //
  };
  auto tunes_scores =
      tune_difference_scores( dims.to_vec_dims() );
  CHECK( !tunes_scores.empty() );
  auto first_score = tunes_scores[0].second;
  auto same_distance =
      tunes_scores |
      rv::take_while( LC( _.second == first_score ) );
  return rng::pick_one(
             rg::to<Vec<Pair<TuneId, int>>>( same_distance ) )
      .first;
}

} // namespace rn
