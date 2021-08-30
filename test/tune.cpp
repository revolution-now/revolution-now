/****************************************************************
**tune.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-26.
*
* Description: Unit tests for the src/tune.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/tune.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::rcl::error );
FMT_TO_CATCH( ::rn::Tune );
FMT_TO_CATCH( ::rn::TuneDimensions );

namespace rn {
namespace {

using namespace std;

TEST_CASE( "[tune] rcl (Tune)" ) {
  using namespace rcl;
  using KV = table::value_type;

  SECTION( "success" ) {
    UNWRAP_CHECK(
        tbl, run_postprocessing( make_table(
                 KV{ "display_name", "some-name" },
                 KV{ "description", "some-description" },
                 KV{ "stem", "some-stem" },
                 KV{ "dimensions",
                     make_table_val(
                         KV{ "tempo", "medium" },
                         KV{ "genre", "trad" },
                         KV{ "culture", "old_world" },
                         KV{ "inst", "percussive" },
                         KV{ "sentiment", "war_lost" },
                         KV{ "key", "c" }, //
                         KV{ "tonality", "major" },
                         KV{ "epoch", "post_revolution" },
                         KV{ "purpose", "standard" } ) } ) ) );
    value v{ std::make_unique<table>( std::move( tbl ) ) };

    // Test.
    Tune expected{
        .display_name = "some-name",
        .stem         = "some-stem",
        .description  = "some-description",
        .dimensions =
            TuneDimensions{
                .tempo     = e_tune_tempo::medium,
                .genre     = e_tune_genre::trad,
                .culture   = e_tune_culture::old_world,
                .inst      = e_tune_inst::percussive,
                .sentiment = e_tune_sentiment::war_lost,
                .key       = e_tune_key::c,
                .tonality  = e_tune_tonality::major,
                .epoch     = e_tune_epoch::post_revolution,
                .purpose   = e_tune_purpose::standard,
            },
    };
    REQUIRE( convert_to<Tune>( v ) == expected );
  }
  SECTION( "failure 1" ) {
    UNWRAP_CHECK(
        tbl, run_postprocessing( make_table(
                 KV{ "display_name", "some-name" },
                 KV{ "description", "some-description" },
                 KV{ "???", "???" }, //
                 KV{ "stem", "some-stem" },
                 KV{ "dimensions",
                     make_table_val(
                         KV{ "tempo", "medium" },
                         KV{ "genre", "trad" },
                         KV{ "culture", "old_world" },
                         KV{ "inst", "percussive" },
                         KV{ "sentiment", "war_lost" },
                         KV{ "key", "c" }, //
                         KV{ "tonality", "major" },
                         KV{ "epoch", "post_revolution" },
                         KV{ "purpose", "standard" } ) } ) ) );
    value v{ std::make_unique<table>( std::move( tbl ) ) };

    // Test.
    REQUIRE( convert_to<Tune>( v ) ==
             error( "expected exactly 4 fields for Tune object, "
                    "but found 5." ) );
  }
  SECTION( "failure 2" ) {
    UNWRAP_CHECK(
        tbl, run_postprocessing( make_table(
                 KV{ "display_name", "some-name" },
                 KV{ "description", "some-description" },
                 KV{ "stem", "some-stem" },
                 KV{ "dimensions",
                     make_table_val(
                         KV{ "tempo", "medium" },
                         KV{ "genre", "trad" },
                         KV{ "culture", "old_world" },
                         KV{ "inst", "percussive" },
                         KV{ "sentimentxx", "war_lost" },
                         KV{ "key", "c" }, //
                         KV{ "tonality", "major" },
                         KV{ "epoch", "post_revolution" },
                         KV{ "purpose", "standard" } ) } ) ) );
    value v{ std::make_unique<table>( std::move( tbl ) ) };

    // Test.
    REQUIRE(
        convert_to<Tune>( v ) ==
        error( "field 'sentiment' required by TuneDimensions "
               "object but was not found." ) );
  }
  SECTION( "failure 3" ) {
    UNWRAP_CHECK(
        tbl, run_postprocessing( make_table(
                 KV{ "display_name", "some-name" },
                 KV{ "description", "some-description" },
                 KV{ "stem", 3 },
                 KV{ "dimensions",
                     make_table_val(
                         KV{ "tempo", "medium" },
                         KV{ "genre", "trad" },
                         KV{ "culture", "old_world" },
                         KV{ "inst", "percussive" },
                         KV{ "sentiment", "war_lost" },
                         KV{ "key", "c" }, //
                         KV{ "tonality", "major" },
                         KV{ "epoch", "post_revolution" },
                         KV{ "purpose", "standard" } ) } ) ) );
    value v{ std::make_unique<table>( std::move( tbl ) ) };

    // Test.
    REQUIRE( convert_to<Tune>( v ) ==
             error( "cannot produce std::string from value of "
                    "type int." ) );
  }
}

TEST_CASE( "[tune] Tune to_str" ) {
  Tune o{
      .display_name = "some-name",
      .stem         = "some-stem",
      .description  = "some-description",
      .dimensions =
          TuneDimensions{
              .tempo     = e_tune_tempo::medium,
              .genre     = e_tune_genre::trad,
              .culture   = e_tune_culture::old_world,
              .inst      = e_tune_inst::percussive,
              .sentiment = e_tune_sentiment::war_lost,
              .key       = e_tune_key::c,
              .tonality  = e_tune_tonality::major,
              .epoch     = e_tune_epoch::post_revolution,
              .purpose   = e_tune_purpose::standard,
          },
  };

  string_view expected =
      "Tune{"
      "display_name=some-name, "
      "stem=some-stem, "
      "description=\"some-description\", "
      "dimensions=TuneDimensions{"
      "tempo=medium, "
      "genre=trad, "
      "culture=old_world, "
      "inst=percussive, "
      "sentiment=war_lost, "
      "key=c, "
      "tonality=major, "
      "epoch=post_revolution, "
      "purpose=standard"
      "}"
      "}";

  REQUIRE( base::to_str( o ) == expected );
}
} // namespace
} // namespace rn
