/****************************************************************
**perlin-map-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-01-10.
*
* Description: Unit tests for the perlin-map module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/perlin-map.hpp"

// rand
#include "src/rand/entropy.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::base::invalid;
using ::base::valid;
using ::gfx::dsize;
using ::gfx::matrix;
using ::gfx::size;

[[nodiscard]] string format_map( matrix<e_surface> const& m ) {
  string out;
  for( int y = 0; y < m.size().h; ++y ) {
    out += "    ";
    for( int x = 0; x < m.size().w; ++x )
      out += fmt::format(
          "{},", m[y][x] == e_surface::land ? 'X' : '_' );
    out += '\n';
  }
  return out;
}

struct PrintableMap {
  matrix<e_surface> const& m;

  [[nodiscard]] friend bool operator==( PrintableMap const& l,
                                        PrintableMap const& r ) {
    return l.m == r.m;
  }

  [[maybe_unused]] friend void to_str(
      PrintableMap const& o, std::string& out,
      base::tag<PrintableMap> ) {
    out += format_map( o.m );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[perlin-map] generate_perlin_seed" ) {
  rng::entropy e;
  PerlinSeed expected;

  auto const f = [&] [[clang::noinline]] {
    return generate_perlin_seed( e );
  };

  e = {
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x00f00076,
  };
  expected = {
    .offset_x = 0x6151c187,
    .offset_y = 0x6da636d6,
    .base     = 0xfbe4a276,
    .flip     = true,
  };
  REQUIRE( f() == expected );

  e = {
    .e1 = 0x6da636d6,
    .e2 = 0x6151c187,
    .e3 = 0xfbe4a276,
    .e4 = 0x00f00077,
  };
  expected = {
    .offset_x = 0x6da636d6,
    .offset_y = 0x6151c187,
    .base     = 0xfbe4a276,
    .flip     = false,
  };
  REQUIRE( f() == expected );
}

// This was a specific case where there was a convergence failure
// which was then fixed by adding the dynamic tolerance adjust-
// ment in the case where the target tolerance couldn't quite be
// reached.
TEST_CASE(
    "[perlin-map] land_gen_perlin: 16x16/.6 convergence "
    "failure" ) {
  PerlinMapSettings const settings{
    .land_form = { .scale   = 12,
                   .fractal = { .n_octaves   = 5,
                                .persistence = 0.5,
                                .lacunarity  = 2 } },

    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 0.9, .h = 0.77 } },

    .seed = { .offset_x = 876454408,
              .offset_y = 2451924743,
              .base     = 1152681597,
              .flip     = false },
  };
  double const target_density = 0.6;
  size const world_sz{ .w = 16, .h = 16 };

  matrix<e_surface> surface, expected;

  auto const f = [&] [[clang::noinline]] {
    surface = {};
    return land_gen_perlin( settings, target_density, world_sz,
                            surface );
  };

  REQUIRE( f() == valid );

  auto constexpr _ = e_surface::water;
  auto constexpr X = e_surface::land;

  expected = matrix<e_surface>(
      16,
      vector{
        // clang-format off
        _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
        _,_,_,_,_,X,_,X,_,_,_,_,_,_,_,_,
        _,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,
        _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,
        _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,
        _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,
        _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,
        _,_,_,_,X,_,X,_,X,X,X,X,X,X,X,X,
        _,_,_,_,X,_,_,_,X,X,X,X,X,X,X,X,
        _,_,_,X,X,X,_,_,X,X,X,X,X,X,X,_,
        _,_,X,X,X,X,X,_,X,X,X,X,_,X,X,X,
        _,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,
        _,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,
        _,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,
        _,_,_,X,X,X,X,X,X,X,_,X,X,X,X,X,
        _,_,_,X,_,X,X,X,X,_,_,_,X,X,X,X,
        // clang-format on
      } );
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );
}

TEST_CASE( "[perlin-map] land_gen_perlin" ) {
  using enum e_perlin_map_error;
  matrix<e_surface> surface, expected;

  PerlinMapSettings settings;
  double target_density = {};
  size world_sz         = {};

  auto constexpr _ = e_surface::water;
  auto constexpr X = e_surface::land;

  PerlinSeed const kSeed = {
    .offset_x = 876454408,
    .offset_y = 2451924743,
    .base     = 1152681597,
    .flip     = false,
  };
  PerlinSeed const kSeedFlipped = [&] {
    auto res = kSeed;
    res.flip = true;
    return res;
  }();

  PerlinEdgeSuppression const kSuppression{
    .enabled  = true,
    .strength = { .w = 0.9, .h = 0.77 },
  };

  rng::PerlinFractalOptions const kFractalOptions{
    .n_octaves   = 5,
    .persistence = 0.5,
    .lacunarity  = 2,
  };

  PerlinLandForm const kLandForm{
    .scale   = 12,
    .fractal = kFractalOptions,
  };

  PerlinMapSettings const kSettings{
    .land_form        = kLandForm,
    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };

  auto const f = [&] [[clang::noinline]] {
    surface = {};
    return land_gen_perlin( settings, target_density, world_sz,
                            surface );
  };

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 0, .h = 0 };
  settings       = kSettings;
  target_density = 0.0;

  REQUIRE( f() == invalid_map_size );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 10, .h = 0 };
  settings       = kSettings;
  target_density = 0.0;

  REQUIRE( f() == invalid_map_size );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 0, .h = 10 };
  settings       = kSettings;
  target_density = 0.0;

  REQUIRE( f() == invalid_map_size );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 1, .h = 1 };
  settings       = kSettings;
  target_density = 0.0;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 1, .h = 1 };
  settings       = kSettings;
  target_density = 0.000001;

  REQUIRE( f() == invalid( density_too_small_no_land ) );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz                          = { .w = 1, .h = 16 };
  settings                          = kSettings;
  settings.edge_suppression.enabled = false;
  target_density                    = 0.5;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      X,
      _,
      _,
      _,
      _,
      _,
      _,
      _,
      _,
      X,
      X,
      X,
      X,
      X,
      X,
      X,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz                          = { .w = 16, .h = 1 };
  settings                          = kSettings;
  settings.edge_suppression.enabled = false;
  target_density                    = 0.5;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      X,X,_,X,X,X,X,X,_,_,_,_,_,_,X,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 4, .h = 4 };
  settings       = kSettings;
  target_density = 0.5;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,
      _,X,X,X,
      _,X,X,X,
      _,_,X,X,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 5, .h = 5 };
  settings       = kSettings;
  target_density = 0.5;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,X,X,X,_,
      _,X,X,X,_,
      _,X,X,X,_,
      _,X,X,X,_,
      _,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 6, .h = 6 };
  settings       = kSettings;
  target_density = 0.5;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,
      _,_,X,X,X,X,
      _,_,X,X,X,X,
      _,_,_,X,X,X,
      _,_,_,X,X,X,
      _,_,X,X,X,X,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 7, .h = 7 };
  settings       = kSettings;
  target_density = 0.5;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,X,X,_,
      _,X,X,X,X,X,_,
      _,X,X,X,X,X,_,
      _,_,X,X,X,X,_,
      _,_,X,X,X,X,_,
      _,_,X,X,X,X,_,
      _,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 7, .h = 7 };
  settings       = kSettings;
  target_density = 0.25;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,
      _,_,X,X,X,X,_,
      _,_,_,X,X,X,_,
      _,_,_,_,X,X,_,
      _,_,_,_,X,X,_,
      _,_,_,X,_,X,_,
      _,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 8, .h = 8 };
  settings       = kSettings;
  target_density = 0.3;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,
      _,_,_,_,X,X,X,_,
      _,_,_,_,X,X,X,X,
      _,_,_,_,_,X,X,X,
      _,_,_,_,_,X,X,X,
      _,_,_,_,_,X,X,X,
      _,_,X,X,_,X,X,_,
      _,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 9, .h = 9 };
  settings       = kSettings;
  target_density = 0.3;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,
      _,_,_,_,X,X,X,X,_,
      _,_,_,X,X,X,X,X,_,
      _,_,_,_,_,X,X,X,_,
      _,_,_,_,_,X,X,X,_,
      _,_,_,X,_,X,X,X,_,
      _,_,X,X,X,X,X,_,_,
      _,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 10, .h = 10 };
  settings       = kSettings;
  target_density = 0.3;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,_,_,
      _,_,_,_,X,X,X,X,X,_,
      _,_,_,_,_,X,X,X,X,_,
      _,_,_,_,_,X,X,X,X,X,
      _,_,_,_,_,X,X,X,X,X,
      _,_,_,_,_,X,X,X,X,X,
      _,_,_,_,_,_,_,_,X,_,
      _,_,_,_,_,_,_,_,_,_,
      _,_,X,_,X,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 16, .h = 16 };
  settings       = kSettings;
  target_density = 0.0;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 16, .h = 16 };
  settings       = kSettings;
  target_density = 0.25;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,_,_,_,X,X,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,
      _,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,X,_,_,_,_,_,_,_,_,X,X,_,
      _,_,_,_,X,X,X,X,_,_,_,_,X,X,X,_,
      _,_,_,_,X,X,_,X,_,_,_,_,X,X,X,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,
      _,_,_,_,_,_,X,_,_,_,_,_,_,X,X,X,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 16, .h = 16 };
  settings       = kSettings;
  target_density = 1.0;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 16, .h = 16 };
  settings       = kSettings;
  target_density = 0.9;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,X,X,X,X,X,X,X,X,_,_,X,X,X,_,
      _,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      _,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      _,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      _,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      _,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      _,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      _,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      _,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      _,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      _,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      _,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      _,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      _,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      _,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      _,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 16, .h = 16 };
  settings       = kSettings;
  target_density = 0.1;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,_,X,X,X,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,
      _,_,_,_,_,_,_,_,_,X,_,X,X,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 16, .h = 16 };
  settings       = kSettings;
  target_density = 0.5;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,_,X,_,X,X,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,
      _,_,_,_,X,X,_,_,_,_,X,X,X,X,X,_,
      _,_,_,X,X,X,X,_,_,_,X,X,_,X,X,_,
      _,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,
      _,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,
      _,_,_,_,X,X,X,X,X,_,X,X,X,X,X,X,
      _,_,_,X,X,X,X,X,X,_,_,X,X,X,X,X,
      _,_,_,_,_,X,X,X,_,_,_,_,X,X,X,X,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 16, .h = 16 };
  settings       = kSettings;
  settings.seed  = kSeedFlipped;
  target_density = 0.5;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,
      _,_,X,X,_,_,_,_,_,X,X,X,X,_,_,_,
      _,_,X,X,X,X,X,_,X,X,X,X,X,_,_,_,
      _,X,X,X,X,X,_,X,X,X,X,X,X,_,_,_,
      _,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,X,_,X,_,_,_,_,_,
      _,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,
      _,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,
      _,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,
      _,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,
      _,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,
      _,_,_,X,_,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz      = { .w = 16, .h = 16 };
  settings      = kSettings;
  settings.seed = {
    .offset_x = 876454409,
    .offset_y = 2451924744,
    .base     = 1152681597,
    .flip     = false,
  };
  target_density = 0.5;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,_,_,X,X,X,X,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,
      _,_,_,_,_,_,_,_,_,X,_,_,X,X,X,_,
      _,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,
      _,_,_,X,X,X,X,X,X,_,X,X,X,X,X,_,
      _,_,_,X,X,X,X,X,_,X,X,X,X,X,X,X,
      _,_,_,X,X,X,X,X,_,_,X,X,X,X,X,X,
      _,_,_,_,X,X,X,X,X,_,_,X,X,X,X,X,
      _,_,_,_,X,X,X,_,_,_,X,X,X,X,X,X,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz      = { .w = 16, .h = 16 };
  settings      = kSettings;
  settings.seed = {
    .offset_x = 876454409,
    .offset_y = 2451924744,
    .base     = 1152681598,
    .flip     = false,
  };
  target_density = 0.5;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,X,X,X,_,_,_,_,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,_,_,_,X,_,_,_,_,
      _,_,X,X,X,X,X,X,X,_,_,X,_,_,_,_,
      _,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,
      _,X,X,X,X,X,X,X,X,X,X,X,_,X,_,_,
      _,X,X,X,X,X,X,X,X,X,_,_,_,X,_,_,
      _,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 16, .h = 16 };
  settings = {
    .land_form = { .scale = 0, .fractal = kFractalOptions },
    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.5;

  REQUIRE( f() == invalid( invalid_settings ) );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 16, .h = 16 };
  settings = {
    .land_form = { .scale = 1.0, .fractal = kFractalOptions },
    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.5;

  REQUIRE( f() == invalid( invalid_settings ) );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 16, .h = 16 };
  settings = {
    .land_form = { .scale = 1.01, .fractal = kFractalOptions },
    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.5;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,X,_,_,_,X,X,_,_,_,X,_,X,_,_,
      _,X,_,_,_,_,X,_,_,X,_,X,_,X,_,_,
      _,_,_,_,X,X,_,X,_,_,X,X,X,X,_,_,
      _,_,_,X,X,X,X,X,X,_,_,X,X,X,_,_,
      _,_,_,X,_,X,_,X,X,_,X,X,X,X,X,_,
      _,_,_,X,X,X,X,X,X,X,_,X,X,_,_,_,
      _,_,X,_,X,_,X,_,X,X,X,X,X,X,X,_,
      _,_,_,_,X,X,X,_,X,X,_,_,X,X,X,_,
      _,X,X,_,X,X,_,_,X,X,X,X,_,_,X,_,
      _,_,X,_,_,X,X,X,X,X,_,X,X,X,_,_,
      _,X,X,X,X,X,X,_,X,X,X,_,X,_,_,_,
      _,_,X,_,X,_,X,X,X,_,X,_,X,_,X,X,
      _,_,X,_,X,X,_,_,X,X,X,X,X,_,_,_,
      _,_,X,_,X,X,_,X,_,X,_,_,_,X,_,_,
      _,_,_,X,X,X,_,_,X,X,_,X,_,X,_,_,
      _,_,X,X,X,_,X,_,X,_,_,X,X,_,X,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 16, .h = 16 };
  settings = {
    .land_form = { .scale = 1.01, .fractal = kFractalOptions },
    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.1;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,_,_,X,_,_,_,
      _,_,_,_,_,X,_,_,_,_,_,_,_,X,_,_,
      _,_,_,X,_,_,_,X,X,_,_,_,_,_,X,_,
      _,_,_,_,_,X,_,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,X,X,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,
      _,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,X,_,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,_,X,_,_,_,_,_,
      _,_,X,_,_,_,_,_,_,_,X,_,X,_,_,_,
      _,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,
      _,_,X,_,_,_,_,_,X,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 16, .h = 16 };
  settings = {
    .land_form = { .scale = 2, .fractal = kFractalOptions },
    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.25;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,X,_,_,_,X,_,
      _,_,_,X,_,_,_,_,_,_,_,_,_,X,_,_,
      _,_,_,X,_,_,_,_,_,_,_,_,_,X,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,
      _,_,_,X,X,X,_,X,X,_,_,_,X,_,_,_,
      _,_,_,X,_,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,X,_,_,X,_,_,_,X,X,_,_,_,_,
      _,_,_,X,_,_,X,_,X,_,X,X,_,X,_,_,
      _,_,_,X,_,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,X,X,X,_,_,_,_,_,
      _,_,_,_,X,X,X,_,X,X,X,_,X,_,_,_,
      _,_,_,X,_,_,X,_,X,X,X,_,_,_,_,_,
      _,_,_,X,_,_,_,_,_,X,X,_,_,_,_,_,
      _,_,_,X,_,_,_,X,_,_,_,_,_,_,_,_,
      _,_,_,X,_,_,X,X,X,_,_,X,X,_,X,_,
      _,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 16, .h = 16 };
  settings = {
    .land_form = { .scale = 3, .fractal = kFractalOptions },
    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.25;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,_,_,X,_,_,
      _,_,X,X,X,_,_,X,X,X,_,_,_,X,X,_,
      _,_,_,_,X,_,_,_,_,X,X,_,_,X,_,_,
      _,_,_,_,_,X,_,_,_,_,_,X,X,_,_,_,
      _,_,_,X,_,X,_,_,_,_,_,X,X,X,_,_,
      _,_,_,_,X,_,_,X,X,X,_,_,X,_,_,X,
      _,_,_,_,_,_,_,X,X,X,_,_,_,X,_,_,
      _,_,_,X,_,_,X,X,X,X,_,X,X,X,_,X,
      _,_,_,_,_,_,X,X,X,_,_,_,X,X,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,
      _,_,_,X,X,X,_,_,_,X,X,X,X,_,X,_,
      _,_,_,_,X,_,_,_,_,X,X,_,_,_,_,_,
      _,_,_,_,X,_,_,_,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 16, .h = 16 };
  settings = {
    .land_form        = { .scale   = 5.333333333,
                          .fractal = kFractalOptions },
    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.25;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,_,X,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,_,X,X,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,
      _,_,_,_,_,_,_,X,_,X,X,X,X,_,_,_,
      _,_,_,X,_,_,_,X,X,X,X,X,X,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,X,X,_,X,X,X,X,_,_,_,_,_,
      _,_,_,_,X,X,_,_,_,X,X,_,_,_,_,_,
      _,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 16, .h = 16 };
  settings = {
    .land_form = { .scale = 7, .fractal = kFractalOptions },
    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.25;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,_,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,_,X,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,_,X,_,_,_,
      _,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,
      _,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,
      _,_,X,X,X,X,X,X,X,X,X,X,X,_,X,_,
      _,_,X,X,_,_,_,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 16, .h = 16 };
  settings = {
    .land_form = { .scale = 9, .fractal = kFractalOptions },
    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.25;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,X,X,_,_,_,_,_,_,X,X,X,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,
      _,_,_,_,_,X,_,_,_,_,X,X,X,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,
      _,_,_,_,_,_,_,_,_,X,_,X,X,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,_,X,_,_,
      _,_,_,X,_,_,_,_,_,_,X,X,X,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,
      _,_,X,_,_,_,_,_,_,_,X,X,X,X,_,_,
      _,_,X,_,_,_,_,_,_,X,X,X,X,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 16, .h = 16 };
  settings = {
    .land_form = { .scale = 11, .fractal = kFractalOptions },
    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.25;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,
      _,_,_,_,_,X,X,X,X,X,_,_,_,X,_,_,
      _,_,_,_,_,_,_,X,X,_,_,_,X,X,_,_,
      _,_,_,_,_,_,_,X,_,_,_,X,X,X,X,_,
      _,_,_,_,_,_,_,_,X,_,_,X,X,X,X,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 16, .h = 16 };
  settings = {
    .land_form = { .scale = 15, .fractal = kFractalOptions },
    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.25;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,_,X,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,X,X,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 16, .h = 16 };
  settings = {
    .land_form = { .scale = 16, .fractal = kFractalOptions },
    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.25;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,
      _,_,_,_,_,X,X,_,X,X,X,X,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 16, .h = 16 };
  settings = {
    .land_form = { .scale = 20, .fractal = kFractalOptions },
    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.25;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 16, .h = 16 };
  settings = {
    .land_form = { .scale = 50, .fractal = kFractalOptions },
    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.25;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 16, .h = 16 };
  settings = {
    .land_form = { .scale = 200, .fractal = kFractalOptions },
    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.25;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 16, .h = 16 };
  settings = {
    .land_form = { .scale = 10000, .fractal = kFractalOptions },
    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.25;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 16, .h = 16 };
  settings = {
    .land_form = { .scale   = 12,
                   .fractal = { .n_octaves   = 5,
                                .persistence = 0.5,
                                .lacunarity  = 2 } },

    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.25;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,_,_,_,X,X,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,
      _,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,X,_,_,_,_,_,_,_,_,X,X,_,
      _,_,_,_,X,X,X,X,_,_,_,_,X,X,X,_,
      _,_,_,_,X,X,_,X,_,_,_,_,X,X,X,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,
      _,_,_,_,_,_,X,_,_,_,_,_,_,X,X,X,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form = { .scale   = 12,
                   .fractal = { .n_octaves   = 0,
                                .persistence = 0.5,
                                .lacunarity  = 2 } },

    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == invalid( invalid_settings ) );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form = { .scale   = 12,
                   .fractal = { .n_octaves   = 1,
                                .persistence = 0.5,
                                .lacunarity  = 2 } },

    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,X,X,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form = { .scale   = 12,
                   .fractal = { .n_octaves   = 2,
                                .persistence = 0.5,
                                .lacunarity  = 2 } },

    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,_,_,_,_,X,X,X,X,X,_,_,_,_,X,X,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,X,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form = { .scale   = 12,
                   .fractal = { .n_octaves   = 3,
                                .persistence = 0.5,
                                .lacunarity  = 2 } },

    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,_,_,X,X,X,X,X,X,_,_,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,X,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,X,_,X,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form = { .scale   = 12,
                   .fractal = { .n_octaves   = 6,
                                .persistence = 0.5,
                                .lacunarity  = 2 } },

    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,X,X,X,X,X,X,_,_,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,X,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,X,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form = { .scale   = 12,
                   .fractal = { .n_octaves   = 11,
                                .persistence = 0.5,
                                .lacunarity  = 2 } },

    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,X,X,X,X,X,X,_,_,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,X,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form = { .scale   = 12,
                   .fractal = { .n_octaves   = 28,
                                .persistence = 0.5,
                                .lacunarity  = 2 } },

    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,X,X,X,X,X,X,_,_,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,X,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form = { .scale   = 12,
                   .fractal = { .n_octaves   = 100,
                                .persistence = 0.5,
                                .lacunarity  = 2 } },

    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,X,X,X,X,X,X,_,_,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,X,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form = { .scale   = 12,
                   .fractal = { .n_octaves   = 5,
                                .persistence = 2.0,
                                .lacunarity  = 2 } },

    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,X,_,_,_,_,_,_,
      _,_,_,_,X,_,_,_,_,_,_,_,X,X,_,_,X,_,X,_,_,X,X,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,X,_,_,_,_,X,_,_,X,_,_,_,
      _,_,_,_,_,_,X,_,_,_,X,_,X,_,_,_,X,_,_,_,_,X,_,_,_,_,_,_,
      _,_,_,X,_,_,X,X,_,X,_,_,_,_,_,X,_,_,X,_,_,X,X,_,_,_,_,_,
      _,_,_,_,_,_,X,_,X,_,_,X,X,_,_,_,_,_,_,X,X,X,_,X,_,_,_,_,
      _,_,_,_,X,_,X,_,_,X,_,X,_,X,X,X,X,X,_,_,X,_,X,_,_,_,_,_,
      _,_,_,X,X,_,_,_,X,_,_,X,_,_,_,_,_,X,X,X,_,_,X,_,_,_,_,_,
      _,_,_,_,X,X,_,X,X,_,_,X,X,_,_,_,_,X,X,X,X,_,X,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,X,_,_,X,_,X,_,X,X,X,X,_,X,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,_,_,X,X,X,_,X,_,X,_,_,X,_,X,_,_,_,
      _,_,_,_,X,X,X,X,_,X,_,_,_,_,_,_,X,X,X,X,_,X,X,_,X,X,_,_,
      _,_,_,_,X,_,_,X,_,_,X,_,X,_,X,_,_,X,_,X,_,X,X,_,_,X,_,_,
      _,_,_,_,_,X,X,X,X,X,X,_,X,X,X,X,X,X,_,X,_,X,X,X,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,_,_,_,X,X,_,X,X,X,_,X,X,_,X,_,_,_,
      _,_,_,_,_,_,_,_,X,_,X,_,_,X,X,_,X,X,_,X,_,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,_,X,_,_,_,X,X,_,_,_,_,X,X,_,_,_,_,_,
      _,_,_,_,_,X,_,_,X,_,_,_,_,X,X,X,_,X,X,X,_,X,_,_,_,_,_,_,
      _,_,_,_,X,_,X,_,_,X,X,_,X,_,X,X,X,X,_,_,_,X,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,X,_,_,X,X,_,_,_,X,X,_,X,X,X,X,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,_,_,_,X,X,X,X,_,X,X,X,_,X,X,X,_,_,_,
      _,_,_,_,X,_,X,_,_,_,X,_,_,_,X,X,X,X,_,X,_,_,_,_,_,X,_,_,
      _,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,X,X,X,_,X,X,X,X,X,_,_,_,
      _,_,_,_,X,_,X,_,_,X,X,_,X,X,_,_,_,_,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,X,_,_,_,_,_,X,_,_,_,_,X,_,_,_,_,_,X,X,_,X,_,_,_,
      _,_,_,_,_,_,X,_,_,_,X,_,_,_,_,X,_,_,_,_,X,X,_,_,_,_,_,_,
      _,_,_,X,_,_,_,_,_,_,X,_,_,_,_,_,_,X,_,_,_,X,X,_,X,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form = { .scale   = 12,
                   .fractal = { .n_octaves   = 8,
                                .persistence = 5.0,
                                .lacunarity  = 2 } },

    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,X,_,_,_,_,_,X,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,X,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,X,X,X,_,_,_,X,_,_,_,_,X,X,_,X,_,_,_,
      _,_,_,_,_,_,_,_,X,X,_,_,_,_,_,X,_,_,X,_,X,X,_,X,X,_,_,_,
      _,_,_,_,_,_,X,_,_,_,X,_,_,_,_,X,X,_,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,_,X,_,X,_,_,_,X,_,X,X,X,X,_,X,X,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,_,X,X,X,_,_,_,X,_,_,X,X,X,_,_,X,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,_,_,_,X,_,_,_,_,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,_,_,X,_,X,_,_,_,_,X,X,_,X,_,_,X,_,X,_,_,
      _,_,_,X,X,_,_,X,X,_,_,_,X,X,_,_,_,_,_,X,_,X,X,X,X,X,_,_,
      _,_,_,_,X,_,X,_,_,X,X,_,X,_,_,_,X,_,X,X,_,_,X,_,_,X,_,_,
      _,_,_,_,_,_,_,_,_,X,X,_,X,_,_,X,_,X,_,_,X,X,X,X,X,_,_,_,
      _,_,_,_,X,_,_,X,X,X,_,X,_,X,X,X,_,_,_,_,_,_,_,_,_,_,X,_,
      _,_,_,_,_,_,X,_,_,_,X,_,X,_,_,X,X,_,_,X,_,_,_,_,_,_,_,_,
      _,_,_,X,_,X,_,X,_,X,_,X,_,_,X,_,X,X,X,X,X,_,_,_,X,X,_,_,
      _,_,_,_,X,X,X,_,X,X,_,X,_,_,X,_,_,_,_,_,_,_,X,_,X,X,_,_,
      _,_,_,_,X,_,X,_,_,X,X,_,X,X,_,X,X,_,_,_,_,_,X,_,_,X,_,_,
      _,_,_,_,X,_,X,X,_,X,X,X,X,_,X,_,X,X,X,_,X,_,_,_,_,_,_,_,
      _,_,_,_,X,_,_,_,_,_,X,_,_,X,X,X,_,X,X,_,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,X,_,_,_,X,_,_,_,_,_,_,_,_,_,_,X,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,_,_,X,X,_,X,_,_,_,_,_,X,_,_,_,
      _,_,_,_,X,X,X,X,X,X,_,X,_,_,_,X,X,_,_,_,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,X,_,_,X,_,_,X,_,_,_,X,_,X,X,_,_,_,_,_,
      _,_,_,X,_,_,X,X,_,X,_,_,X,_,X,X,X,X,X,_,_,_,X,_,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,_,X,_,_,X,X,_,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,X,_,_,X,_,_,_,_,_,X,_,_,X,_,X,X,_,_,_,_,X,_,_,_,
      _,_,X,_,X,X,_,_,_,X,_,X,X,_,X,_,_,_,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form = { .scale   = 12,
                   .fractal = { .n_octaves   = 8,
                                .persistence = 1.0,
                                .lacunarity  = 10 } },

    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,_,_,_,_,_,_,_,X,_,X,_,_,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,_,_,_,_,_,_,X,X,_,X,_,_,_,_,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,
      _,_,_,_,X,_,X,_,_,_,_,X,_,X,X,X,X,X,X,X,_,X,_,_,X,_,_,_,
      _,_,_,_,_,_,_,X,X,_,_,_,X,X,_,X,X,X,X,X,_,_,_,X,X,_,_,_,
      _,_,_,_,_,_,X,_,X,_,X,_,_,X,X,X,X,X,_,_,X,X,X,_,_,_,_,_,
      _,_,_,_,_,X,_,_,_,_,X,X,X,X,X,X,X,_,X,_,X,X,X,_,_,_,_,_,
      _,_,_,_,_,X,_,X,X,X,X,X,X,X,X,X,X,_,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,X,_,_,_,_,X,X,X,X,X,X,_,X,X,X,_,X,_,X,_,X,_,_,_,
      _,_,_,_,_,_,X,X,_,X,X,_,X,X,_,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,X,_,X,X,_,X,X,X,X,X,X,_,X,X,_,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,_,X,_,X,X,_,X,X,_,_,X,X,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,_,X,_,_,X,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,X,_,_,X,_,_,X,X,X,X,X,_,X,_,X,X,X,_,X,_,_,_,_,
      _,_,_,_,X,_,X,X,_,_,X,_,X,_,X,_,X,X,X,X,_,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,_,X,X,X,X,X,_,_,_,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,X,X,_,X,X,X,X,X,_,X,X,X,_,_,X,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,_,_,_,_,X,X,X,X,X,X,X,_,X,_,X,_,_,_,
      _,_,_,X,X,X,_,_,_,X,X,X,X,_,X,X,_,X,X,_,_,_,_,X,_,_,_,_,
      _,_,_,X,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,X,X,_,X,X,X,_,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,X,_,_,X,X,_,_,_,_,_,X,X,_,X,_,_,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,X,_,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,_,_,_,_,_,_,X,_,_,X,_,_,_,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form = { .scale   = 12,
                   .fractal = { .n_octaves   = 0,
                                .persistence = 0.5,
                                .lacunarity  = 2 } },

    .edge_suppression = { .enabled  = false,
                          .strength = { .w = 0.9, .h = 0.77 } },
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == invalid( invalid_settings ) );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 0.9, .h = 0.77 } },
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,X,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,X,X,X,X,X,X,_,_,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,X,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 0.9, .h = 0.77 } },
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,X,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,X,X,X,X,X,X,_,_,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,X,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 0.9, .h = 0.4 } },
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,_,_,_,X,X,_,_,X,X,_,_,X,X,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,_,X,X,X,X,X,X,X,_,X,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 0.9, .h = 0.2 } },
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,_,_,_,X,X,_,_,X,X,_,_,X,X,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,_,X,X,X,X,X,X,X,_,X,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 0.9, .h = 0.05 } },
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,_,_,_,X,X,_,_,X,X,_,_,X,X,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,_,X,X,X,X,X,X,X,_,X,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 0.9, .h = 0.0 } },
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,_,_,_,_,_,X,X,_,_,X,X,_,_,X,X,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,_,X,X,X,X,X,X,X,_,X,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 0.4, .h = 0.0 } },
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,X,X,X,X,X,X,_,_,X,X,X,X,X,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,X,_,X,X,X,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      X,_,_,X,_,_,_,X,_,_,_,X,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      X,X,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,
      X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,
      X,X,X,_,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,X,_,_,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,
      X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 0.2, .h = 0.0 } },
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,X,X,X,X,X,X,_,_,X,X,X,X,X,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,X,_,X,X,X,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      X,X,_,X,_,_,_,X,_,_,_,X,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      X,X,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,
      X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,
      X,X,X,_,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,X,_,_,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,
      X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 0.0, .h = 0.0 } },
    .seed             = kSeed,
  };
  target_density = 0.30;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,X,X,X,X,X,X,_,_,X,X,X,X,X,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,X,_,X,X,X,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      X,X,_,X,_,_,_,X,_,_,_,X,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      X,X,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,
      X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,
      X,X,X,_,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,X,_,_,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,
      X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 0.0, .h = 0.0 } },
    .seed             = kSeed,
  };
  target_density = 0.70;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,X,X,X,X,X,_,_,_,_,_,_,X,_,X,X,X,X,_,_,X,X,_,_,_,_,
      _,_,_,_,X,X,X,X,X,_,X,_,_,X,X,_,X,X,X,X,X,X,X,X,X,X,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,
      _,X,X,X,X,X,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,
      X,X,X,X,X,X,X,_,_,_,X,X,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,X,X,
      X,X,X,X,X,X,X,X,X,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,
      X,X,X,X,X,X,X,X,X,X,X,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,
      X,X,X,X,X,X,X,_,X,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      X,X,X,X,X,X,X,X,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      X,X,X,X,X,X,X,X,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      X,X,X,X,X,X,X,X,X,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      X,X,X,X,X,X,X,X,X,X,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      X,X,X,X,X,X,X,X,X,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      X,X,X,X,X,X,X,X,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      X,X,X,X,X,X,X,X,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 0.7, .h = 0.0 } },
    .seed             = kSeed,
  };
  target_density = 0.50;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,_,_,_,_,_,X,_,_,X,X,X,_,_,X,X,X,_,_,_,_,
      _,_,_,_,_,X,X,X,X,_,X,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,X,X,X,X,X,X,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,X,_,_,_,_,_,_,X,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,X,X,_,_,_,_,X,X,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,X,X,X,X,X,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,X,X,X,X,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,X,X,X,X,X,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      X,X,X,_,_,X,X,X,X,X,X,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      X,X,X,_,X,X,_,_,_,X,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      X,X,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,X,X,_,_,_,X,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      X,X,X,X,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,X,_,X,X,X,X,X,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,X,_,X,X,X,X,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 1.0, .h = 0.0 } },
    .seed             = kSeed,
  };
  target_density = 0.50;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,X,X,_,_,_,_,_,_,_,X,_,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,_,X,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,X,X,X,X,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 2.0, .h = 0.0 } },
    .seed             = kSeed,
  };
  target_density = 0.50;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 4.0, .h = 0.0 } },
    .seed             = kSeed,
  };
  target_density = 0.50;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 0.0, .h = 0.2 } },
    .seed             = kSeed,
  };
  target_density = 0.50;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,_,X,_,_,_,_,_,_,_,_,X,X,X,_,_,X,X,X,_,_,_,_,
      _,_,_,_,_,_,X,X,X,_,_,_,X,X,X,_,X,X,X,X,X,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,X,X,X,_,_,X,X,X,X,X,X,X,_,X,X,X,X,X,X,X,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,X,X,X,X,X,X,X,X,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,X,_,_,_,_,_,_,_,X,X,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,X,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      X,_,_,X,X,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,
      _,_,X,_,X,X,X,X,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,
      _,X,_,_,X,X,X,X,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,
      _,X,_,X,X,X,X,X,X,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,X,
      _,_,_,X,X,X,X,X,X,X,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,X,
      X,X,X,X,_,X,X,X,X,X,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,
      X,X,X,_,X,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      X,X,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      X,X,X,_,_,X,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      X,X,X,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      X,X,X,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,X,X,X,X,X,X,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,X,_,X,X,X,X,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 0.0, .h = 0.4 } },
    .seed             = kSeed,
  };
  target_density = 0.50;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,_,X,_,_,_,_,_,_,_,_,X,X,X,_,_,X,X,X,_,_,_,_,
      _,_,_,_,_,_,X,X,X,_,_,_,X,X,X,_,X,X,X,X,X,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,X,X,X,_,_,X,X,X,X,X,X,X,_,X,X,X,X,X,X,X,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,X,X,X,X,X,X,X,X,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,X,_,_,_,_,_,_,_,X,X,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,X,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      X,_,_,X,X,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,
      _,_,X,_,X,X,X,X,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,
      _,X,_,_,X,X,X,X,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,
      _,X,_,X,X,X,X,X,X,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,X,
      _,_,_,X,X,X,X,X,X,X,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,X,
      X,X,X,X,_,X,X,X,X,X,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,
      X,X,X,_,X,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      X,X,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      X,X,X,_,_,X,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      X,X,X,_,_,_,X,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      X,X,X,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,X,X,X,X,X,X,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,X,_,X,X,X,X,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 0.0, .h = 0.8 } },
    .seed             = kSeed,
  };
  target_density = 0.50;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,X,X,_,_,_,_,_,
      _,_,_,_,_,_,X,X,_,_,_,_,X,X,X,X,X,X,X,_,X,X,X,X,X,X,X,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,X,X,X,X,X,X,X,X,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,
      _,X,X,X,X,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,X,
      X,X,X,X,X,_,_,_,_,_,X,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,X,
      X,X,X,X,X,X,_,X,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,X,
      X,X,X,_,X,X,X,X,X,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,
      _,X,X,X,X,X,X,X,X,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,
      X,X,X,X,X,X,X,X,X,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,X,
      X,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,X,
      X,X,X,X,X,X,X,X,X,X,X,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,
      X,X,X,_,X,X,_,_,X,X,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      X,X,X,_,X,X,X,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      X,X,X,_,_,X,X,X,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      X,X,X,X,_,_,X,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      X,X,X,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,X,_,X,_,X,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,X,_,X,X,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 0.0, .h = 1.0 } },
    .seed             = kSeed,
  };
  target_density = 0.50;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,X,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,
      _,X,X,X,X,X,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,
      X,X,X,X,X,X,X,_,_,_,X,X,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,
      X,X,X,X,X,X,X,X,X,X,X,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,
      X,X,X,X,X,X,X,_,X,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,X,
      X,X,X,X,X,X,X,X,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      X,X,X,X,X,X,X,X,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      X,X,X,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,
      X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 0.0, .h = 2.0 } },
    .seed             = kSeed,
  };
  target_density = 0.50;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
      X,X,X,_,_,X,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = 2.0, .h = 2.0 } },
    .seed             = kSeed,
  };
  target_density = 0.50;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,X,X,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 28, .h = 28 };
  settings = {
    .land_form        = kLandForm,
    .edge_suppression = { .enabled  = true,
                          .strength = { .w = -2.0, .h = -0.0 } },
    .seed             = kSeed,
  };
  target_density = 0.50;

  REQUIRE( f() == invalid( invalid_settings ) );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz       = { .w = 56, .h = 70 };
  settings       = kSettings;
  target_density = 0.50;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,X,X,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,X,X,X,X,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,X,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,X,X,_,_,X,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,X,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,X,X,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,X,X,X,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,X,X,X,X,X,_,_,X,_,_,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,X,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,
      _,_,_,_,_,_,_,X,X,X,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,
      _,_,_,_,_,_,X,X,X,_,X,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,
      _,_,_,_,_,_,X,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,
      _,_,_,_,_,_,_,X,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,X,X,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,X,X,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,X,X,X,X,X,X,X,X,X,_,_,_,
      _,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,X,X,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,X,X,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,X,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,X,X,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,X,X,X,X,X,X,_,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,
      _,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,X,_,_,
      _,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,X,_,_,
      _,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,X,_,_,
      _,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,_,_,X,X,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,_,X,X,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,_,X,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,X,_,_,_,
      _,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,_,X,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,X,X,_,_,
      _,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,X,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,
      _,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,
      _,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,X,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );

  // ------------------------------------------------------------
  // Map
  // ------------------------------------------------------------
  world_sz = { .w = 56, .h = 70 };
  settings = {
    .land_form = { .scale = 9, .fractal = kFractalOptions },
    .edge_suppression = kSuppression,
    .seed             = kSeed,
  };
  target_density = 0.22;

  REQUIRE( f() == valid );

  // clang-format off
  expected = matrix<e_surface>( world_sz.w, vector{
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,X,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,X,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,X,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,X,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,_,_,X,X,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,_,X,X,X,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,X,_,_,X,X,X,X,X,X,X,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,X,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,X,X,X,X,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,X,X,_,_,X,X,X,_,_,X,X,X,X,X,X,_,X,X,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,X,X,X,X,_,_,_,_,_,_,X,X,X,X,X,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,X,X,X,X,X,X,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,X,X,X,X,X,X,X,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,X,X,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,X,X,X,X,X,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,
      _,_,_,_,_,X,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,X,X,_,_,X,X,X,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,X,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,_,_,X,_,_,X,X,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,X,X,X,X,X,X,X,X,X,_,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,X,X,_,X,X,X,X,X,_,_,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
      _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  } );
  // clang-format on
  REQUIRE( PrintableMap{ surface } == PrintableMap{ expected } );
}

} // namespace
} // namespace rn
