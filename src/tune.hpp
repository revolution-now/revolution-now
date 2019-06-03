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
#include "enum.hpp"
#include "typed-int.hpp"

TYPED_ID( TuneId )

namespace rn {

// These need to be reflected enums for deserialization from the
// config files.
enum class e_( tune_tempo, fast, medium, slow );
enum class e_( tune_genre, trad, classical );
enum class e_( tune_culture, native, new_world, old_world );
enum class e_( tune_instrumentation, fife_and_drum, fiddle,
               percussive, orchestrated );
enum class e_( tune_sentiment, happy, sad, war_triumph,
               war_lost );
enum class e_( tune_key, a, bb, b, c, cs, d, eb, e, f, fs, g,
               ab );
enum class e_( tune_tonality, major, minor );
enum class e_( tune_epoch, standard, post_revolution );
enum class e_( tune_purpose, standard, special_event );

struct TuneOptDimensions {
  Opt<e_tune_tempo>           tempo;
  Opt<e_tune_genre>           genre;
  Opt<e_tune_culture>         culture;
  Opt<e_tune_instrumentation> instrumentation;
  Opt<e_tune_sentiment>       sentiment;
  Opt<e_tune_key>             key;
  Opt<e_tune_tonality>        tonality;
  Opt<e_tune_epoch>           epoch;
  Opt<e_tune_purpose>         purpose;
};

struct TuneDimensions {
  TuneOptDimensions to_optional() const;

  e_tune_tempo           tempo;
  e_tune_genre           genre;
  e_tune_culture         culture;
  e_tune_instrumentation instrumentation;
  e_tune_sentiment       sentiment;
  e_tune_key             key;
  e_tune_tonality        tonality;
  e_tune_epoch           epoch;
  e_tune_purpose         purpose;
};

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

// List all tunes that meet certain enum criteria. If
// `fuzzy_match` is false then only tunes will be returned that
// precisely fit the given criteria (either matching it or not
// matching it according to the value of `not_like`). If
// `fuzzy_match` is true the resulting list will contain all
// tunes, but sorted in the order of how closely they meet the
// given criteria (again, either matching or not matching de-
// pending on the value of `not_like`). If a parameter is om-
// mitted (nullopt) then it acts as a wildcard, i.e., any value
// for that field will match the given one. Therefore, the more
// parameters that are left as `nullopt`, the more values there
// will be for a `not_like == false` scenario and the fewer
// values there will be for a `not_like=true` scenario.
Vec<TuneId> find_tunes( TuneOptDimensions dimensions,
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
// which is not very meaningful.
TuneId random_tune();

} // namespace rn
