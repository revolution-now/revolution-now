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
#include "aliases.hpp"
#include "typed-int.hpp"

// base-util
#include "base-util/pp.hpp"

TYPED_ID( TuneId )

namespace rn {

// clang-format off
#define TUNE_DIMENSIONS_DEFINITIONS                               \
  ( tempo,     fast, medium, slow ),                              \
  ( genre,     trad, classical ),                                 \
  ( culture,   native, new_world, old_world ),                    \
  ( inst,      fife_and_drum, fiddle, percussive, orchestrated ), \
  ( sentiment, happy, sad, war_triumph, war_lost ),               \
  ( key,       a, bb, b, c, cs, d, eb, e, f, fs, g, ab ),         \
  ( tonality,  major, minor ),                                    \
  ( epoch,     standard, post_revolution ),                       \
  ( purpose,   standard, special_event )
// clang-format on

#define TUNE_DIMENSION_ENUM( name, ... ) \
  enum class e_tune_##name{ __VA_ARGS__ };

// Create a reflected enum for each of the dimensions. These need
// to be reflected enums for deserialization from the config
// files. E.g., for the `tempo` dimension this will generate:
//
//   enum class e_tune_##tempo { fast, medium, slow };
//
// which of course is itself a macro.
EVAL( PP_MAP_TUPLE( TUNE_DIMENSION_ENUM,
                    TUNE_DIMENSIONS_DEFINITIONS ) )

// Take the first element of each tuple in the dimension
// definitions list; this yields a comma-separated list of
// dimension names.
#define TUNE_DIMENSION_LIST \
  PP_MAP_COMMAS( HEAD_TUPLE, TUNE_DIMENSIONS_DEFINITIONS )

#define TUNE_VEC_DIMENSION( name ) \
  Vec<PP_JOIN( e_tune_, name )> name;

struct TuneVecDimensions {
  EVAL( PP_MAP( TUNE_VEC_DIMENSION, TUNE_DIMENSION_LIST ) )
};
NOTHROW_MOVE( TuneVecDimensions );

#define TUNE_OPT_DIMENSION( name ) \
  Opt<PP_JOIN( e_tune_, name )> name;

struct TuneOptDimensions {
  EVAL( PP_MAP( TUNE_OPT_DIMENSION, TUNE_DIMENSION_LIST ) )
  TuneVecDimensions to_vec_dims() const;
};
NOTHROW_MOVE( TuneOptDimensions );

#define TUNE_DIMENSION( name ) PP_JOIN( e_tune_, name ) name;

struct TuneDimensions {
  EVAL( PP_MAP( TUNE_DIMENSION, TUNE_DIMENSION_LIST ) )
  TuneOptDimensions to_opt_dims() const;
};
NOTHROW_MOVE( TuneDimensions );

#define K_NUM_DIMENSIONS           \
  EVAL( PP_MAP_PLUS( PP_CONST_ONE, \
                     TUNE_DIMENSIONS_DEFINITIONS ) )

constexpr size_t k_num_dimensions = K_NUM_DIMENSIONS;

// Sanity checks.
static_assert( sizeof( TuneDimensions ) ==
               k_num_dimensions * sizeof( e_tune_tempo ) );
static_assert( sizeof( TuneOptDimensions ) ==
               k_num_dimensions * sizeof( Opt<e_tune_tempo> ) );

// Client code is not supposed to get access to this struct di-
// rectly, it is just in the header to allow deserialization from
// the config files.
struct Tune {
  std::string display_name;
  std::string stem;
  std::string description;

  // Classification.
  TuneDimensions dimensions;
};
NOTHROW_MOVE( Tune );

// This can only be populated by a music player.
struct TunePlayerInfo {
  TuneId          id;
  Opt<Duration_t> length;
  // If the player is currently playing a tune then it will re-
  // turn a number in [0,1.0] representing the progress through
  // the tune. Returns `nullopt` if no tune is playing or if the
  // most recent tune has finished.
  Opt<double> progress;

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
Vec<TuneId> const& all_tunes();

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
// mitted (nullopt or empty vector) then it acts as a wildcard,
// i.e., any value for that field will match the given one.
// Therefore, the more parameters that are left as `nullopt` or
// empty, the more values there will be for a `not_like == false`
// scenario and the fewer values there will be for a
// `not_like=true` scenario. The criteria for matching depends on
// which overload is called: for the TuneOptDimensions, a
// specified dimension must match a tune's dimension exactly; for
// TuneVecDimensions it mush match any of the possibilities in
// the vector associated to that dimension.
Vec<TuneId> find_tunes( TuneOptDimensions dimensions,
                        bool              fuzzy_match = true,
                        bool              not_like    = false );
// Matches any of the possibilities for each dimension.
Vec<TuneId> find_tunes( TuneVecDimensions dimensions,
                        bool              fuzzy_match = true,
                        bool              not_like    = false );

// List tunes like given tune. Actually it will return a list of
// all tunes, sorted so that tunes that are most similar to the
// given one appear earlier in the list.
Vec<TuneId> tunes_like( TuneId id );

// List tunes not like given tune. Actually it will return a list
// of all tunes, sorted so that tunes that are most different to
// the given one appear earlier in the list.
Vec<TuneId> tunes_not_like( TuneId id );

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
