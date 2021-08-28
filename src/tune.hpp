/****************************************************************
**tune.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-05-27.
*
* Description: Manages individual pieces of music in the game.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "maybe.hpp"
#include "time.hpp"
#include "typed-int.hpp"

// Rds
#include "rds/tune.hpp"

// Rcl
#include "rcl/ext.hpp"

// base
#include "base/fmt.hpp"
#include "base/to-str.hpp"

// base-util
#include "base-util/pp.hpp"

TYPED_ID( TuneId )

namespace rn {

// Take the first element of each tuple in the dimension
// definitions list; this yields a comma-separated list of
// dimension names.
#define TUNE_DIMENSION_LIST                                     \
  tempo, genre, culture, inst, sentiment, key, tonality, epoch, \
      purpose

#define TUNE_VEC_DIMENSION( name ) \
  std::vector<PP_JOIN( e_tune_, name )> name;

struct TuneVecDimensions {
  EVAL( PP_MAP( TUNE_VEC_DIMENSION, TUNE_DIMENSION_LIST ) )
};
NOTHROW_MOVE( TuneVecDimensions );

#define TUNE_OPT_DIMENSION( name ) \
  maybe<PP_JOIN( e_tune_, name )> name;

struct TuneOptDimensions {
  EVAL( PP_MAP( TUNE_OPT_DIMENSION, TUNE_DIMENSION_LIST ) )
  TuneVecDimensions to_vec_dims() const;
};
NOTHROW_MOVE( TuneOptDimensions );

#define TUNE_DIMENSION( name ) PP_JOIN( e_tune_, name ) name;

struct TuneDimensions {
  EVAL( PP_MAP( TUNE_DIMENSION, TUNE_DIMENSION_LIST ) )
  TuneOptDimensions to_opt_dims() const;
  bool operator==( TuneDimensions const& ) const = default;

  // Allows deserializing from an Rcl config file.
  friend rcl::convert_err<TuneDimensions> convert_to(
      rcl::value const& v, rcl::tag<TuneDimensions> );

  // ADL stringifier.
  friend void to_str( TuneDimensions const& o,
                      std::string&          out );
};
NOTHROW_MOVE( TuneDimensions );

#define K_NUM_DIMENSIONS \
  EVAL( PP_MAP_PLUS( PP_CONST_ONE, TUNE_DIMENSION_LIST ) )

constexpr size_t k_num_dimensions = K_NUM_DIMENSIONS;

// Sanity checks.
static_assert( sizeof( TuneDimensions ) ==
               k_num_dimensions * sizeof( e_tune_tempo ) );
static_assert( sizeof( TuneOptDimensions ) ==
               k_num_dimensions *
                   sizeof( maybe<e_tune_tempo> ) );

// Client code is not supposed to get access to this struct di-
// rectly, it is just in the header to allow deserialization from
// the config files.
struct Tune {
  std::string display_name;
  std::string stem;
  std::string description;

  // Classification.
  TuneDimensions dimensions;

  bool operator==( Tune const& ) const = default;

  // Allows deserializing from an Rcl config file.
  friend rcl::convert_err<Tune> convert_to( rcl::value const& v,
                                            rcl::tag<Tune> );

  // ADL stringifier.
  friend void to_str( Tune const& o, std::string& out );
};
NOTHROW_MOVE( Tune );

// This can only be populated by a music player.
struct TunePlayerInfo {
  TuneId            id;
  maybe<Duration_t> length;
  // If the player is currently playing a tune then it will re-
  // turn a number in [0,1.0] representing the progress through
  // the tune. Returns `nothing` if no tune is playing or if the
  // most recent tune has finished.
  maybe<double> progress;

  void log() const;
};
NOTHROW_MOVE( TunePlayerInfo );

/****************************************************************
** Tune API
*****************************************************************
* This API is specifically designed to discourage client code
* from knowing about any specific tune or referring to tunes by
* name. Client code should interact with the list of tunes only
* through their classification attribute enums ideally. In other
* words, the game should run fine if the list of tunes were
* swapped out for new ones with different names. There should not
* be any tune names hardcoded in the game. Ditto for tune IDs.
*****************************************************************/

// Return list of stems of all tunes. The IDs themselves are not
// useful to client code other than to iterate over the tunes.
std::vector<TuneId> const& all_tunes();

// Get Tune object for tune stem/id.
std::string const& tune_display_name_from_id( TuneId id );
std::string const& tune_desc_from_id( TuneId id );
std::string const& tune_stem_from_id( TuneId id );

TuneDimensions const& tune_dimensions( TuneId id );

// List all tunes that meet certain enum criteria. If
// `fuzzy_match` is false then only tunes will be returned that
// precisely fit the given criteria (either matching it or not
// matching it according to the value of `not_like`). If
// `fuzzy_match` is true the resulting list will contain all
// tunes, but sorted in the order of how closely they meet the
// given criteria (again, either matching or not matching de-
// pending on the value of `not_like`). If a parameter is om-
// mitted (nothing or empty vector) then it acts as a wildcard,
// i.e., any value for that field will match the given one.
// Therefore, the more parameters that are left as `nothing` or
// empty, the more values there will be for a `not_like == false`
// scenario and the fewer values there will be for a
// `not_like=true` scenario. The criteria for matching depends on
// which overload is called: for the TuneOptDimensions, a
// specified dimension must match a tune's dimension exactly; for
// TuneVecDimensions it mush match any of the possibilities in
// the vector associated to that dimension.
std::vector<TuneId> find_tunes( TuneOptDimensions dimensions,
                                bool fuzzy_match = true,
                                bool not_like    = false );
// Matches any of the possibilities for each dimension.
std::vector<TuneId> find_tunes( TuneVecDimensions dimensions,
                                bool fuzzy_match = true,
                                bool not_like    = false );

// List tunes like given tune. Actually it will return a list of
// all tunes, sorted so that tunes that are most similar to the
// given one appear earlier in the list.
std::vector<TuneId> tunes_like( TuneId id );

// List tunes not like given tune. Actually it will return a list
// of all tunes, sorted so that tunes that are most different to
// the given one appear earlier in the list.
std::vector<TuneId> tunes_not_like( TuneId id );

// This will generate a random tune by using the classification
// attributes as random variables, thus yielding an stream of
// tunes that are evenly distributed across classification dimen-
// sions, as opposed to e.g. being evenly distributed across ID,
// which is not very meaningful. When a given random set of
// dimension values are chosen that still does not uniquely
// specify a tune, since multiple tunes can have the same set of
// dimensions. In those cases, a random tune will be selected
// among the resulting subset.
TuneId random_tune();

} // namespace rn

namespace std {

DEFINE_HASH_FOR_TYPED_INT( ::rn::TuneId )

} // namespace std

TOSTR_TO_FMT( ::rn::Tune );
TOSTR_TO_FMT( ::rn::TuneDimensions );
