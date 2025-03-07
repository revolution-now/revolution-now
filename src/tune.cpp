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
#include "error.hpp"
#include "rand-enum.hpp"
#include "time.hpp"

// config
#include "config/music.rds.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/lambda.hpp"
#include "base/logger.hpp"
#include "base/range-lite.hpp"
#include "base/to-str-ext-chrono.hpp"

// base-util
#include "base-util/algo.hpp"

// C++ standard library
#include <limits>
#include <numeric>
#include <regex>
#include <tuple>
#include <unordered_set>

using namespace std;

namespace rn {

namespace rl = ::base::rl;

namespace {

#define TUNE_DIMENSION_ADD_IF_DIFFERENT( dim ) \
  if( !util::contains( dims.dim, tune.dimensions.dim ) ) score++

// Return a list of pairs where there is one pair for each tune,
// and each pair gives the tune ID and the score representing how
// different it is (distance) from the given set of dimensions.
// The result will be sorted from most similar to most different.
vector<pair<TuneId, int>> tune_difference_scores(
    TuneVecDimensions dims ) {
  vector<pair<TuneId, int /*score*/>> scores;
  for( int id = 0; auto const& tune : config_music.tunes ) {
    int score = 0;
    EVAL( PP_MAP_SEMI( TUNE_DIMENSION_ADD_IF_DIFFERENT,
                       TUNE_DIMENSION_LIST ) )
    scores.emplace_back( id, score );
    ++id;
  }
  // Sort from most similar to most different.
  util::sort_by_key( scores, L( _.second ) );
  return scores;
}

} // namespace

#define OPT_TO_VEC( what )                                     \
  {                                                            \
    what.has_value()                                           \
        ? vector<                                              \
              std::decay_t<decltype( what.value() )>>{ *what } \
        : vector<std::decay_t<decltype( what.value() )>> {}    \
  }

TuneVecDimensions TuneOptDimensions::to_vec_dims() const {
  return { EVAL( PP_MAP_COMMAS(
      OPT_TO_VEC, EVAL( TUNE_DIMENSION_LIST ) ) ) };
}

TuneOptDimensions to_opt_dims(
    TuneDimensions const& dimensions ) {
  return {
    dimensions.tempo,     //
    dimensions.genre,     //
    dimensions.culture,   //
    dimensions.inst,      //
    dimensions.sentiment, //
    dimensions.key,       //
    dimensions.tonality,  //
    dimensions.epoch,     //
    dimensions.purpose,   //
  };
}

void TunePlayerInfo::log() const {
  lg.debug( "TunePlayerInfo:" );
  lg.debug( "  id:       {} ({})", id, tune_stem_from_id( id ) );
  lg.debug( "  length:   {}", length );
  lg.debug( "  progress: {}", progress );
}

vector<TuneId> const& all_tunes() {
  static vector<TuneId> tunes = [] {
    vector<TuneId> res;
    res.resize( config_music.tunes.size() );
    iota( res.begin(), res.end(), 0 );
    return res;
  }();
  return tunes;
}

string const& tune_display_name_from_id( TuneId id ) {
  CHECK_LT( id, ssize( config_music.tunes ) );
  return config_music.tunes[id].display_name;
}

string const& tune_desc_from_id( TuneId id ) {
  CHECK_LT( id, ssize( config_music.tunes ) );
  return config_music.tunes[id].description;
}

string const& tune_stem_from_id( TuneId id ) {
  CHECK_LT( id, ssize( config_music.tunes ) );
  return config_music.tunes[id].stem;
}

TuneDimensions const& tune_dimensions( TuneId id ) {
  CHECK_LT( id, ssize( config_music.tunes ) );
  return config_music.tunes[id].dimensions;
}

vector<TuneId> find_tunes( TuneOptDimensions dims,
                           bool fuzzy_match, bool not_like ) {
  return find_tunes( dims.to_vec_dims(), fuzzy_match, not_like );
}

#define TUNE_DIMENSION_COUNT_IF_ENABLED( dim ) \
  ( !dimensions.dim.empty() ? 1 : 0 )

vector<TuneId> find_tunes( TuneVecDimensions dimensions,
                           bool fuzzy_match, bool not_like ) {
  auto scores = tune_difference_scores( dimensions );

  if( !fuzzy_match ) {
    size_t enabled_dimensions = EVAL( PP_MAP_PLUS(
        TUNE_DIMENSION_COUNT_IF_ENABLED, TUNE_DIMENSION_LIST ) );

    CHECK( enabled_dimensions <= k_num_dimensions );

    int target_score_for_non_fuzzy =
        not_like ? enabled_dimensions : 0;
    erase_if( scores,
              LC( _.second != target_score_for_non_fuzzy ) );
  }

  if( not_like ) reverse( scores.begin(), scores.end() );

  auto res = rl::all( scores ).keys().to_vector();
  if( fuzzy_match ) { DCHECK( !res.empty() ); }
  return res;
}

vector<TuneId> tunes_like( TuneId id ) {
  CHECK_LT( id, ssize( config_music.tunes ) );
  auto const& tune = config_music.tunes[id];
  return find_tunes( to_opt_dims( tune.dimensions ), //
                     /*fuzzy_match=*/true,           //
                     /*not_like=*/false              //
  );
}

vector<TuneId> tunes_not_like( TuneId id ) {
  CHECK_LT( id, ssize( config_music.tunes ) );
  auto const& tune = config_music.tunes[id];
  return find_tunes( to_opt_dims( tune.dimensions ), //
                     /*fuzzy_match=*/true,           //
                     /*not_like=*/true               //
  );
}

#define TUNE_DIMENSION_PICK_ONE( dim ) \
  pick_one<e_tune_##dim>( rand )

// The idea here is that we pick a random set of dimensions, then
// rank all tunes according to their distance from that. Then we
// take the distance of the most similar one, and pick out all
// the tunes that have that same distance, and select randomly
// among them. If we were not to choose randomly among all the
// most similar tunes of equal distance then we would run into
// problems where two tunes have the exact same dimensions and
// one would never get picked.
TuneId random_tune( IRand& rand ) {
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
      rl::all( tunes_scores )
          .take_while_L( _.second == first_score )
          .to_vector();
  return rand.pick_one( same_distance ).first;
}

} // namespace rn
