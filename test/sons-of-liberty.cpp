/****************************************************************
**sons-of-liberty.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-19.
*
* Description: Unit tests for the src/sons-of-liberty.* module.
*
*****************************************************************/
#include "test/fake/world.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/sons-of-liberty.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_player( e_nation::dutch );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _,
      L, L, L,
      _, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE(
    "[sons-of-liberty] compute_sons_of_liberty_percent" ) {
  double num_rebels_from_bells_only = 0.0;
  int    colony_population          = 0;
  bool   has_simon_bolivar          = false;

  auto f = [&] {
    return compute_sons_of_liberty_percent(
        num_rebels_from_bells_only, colony_population,
        has_simon_bolivar );
  };

  SECTION( "without bolivar" ) {
    has_simon_bolivar = false;
    SECTION( "population 1" ) {
      colony_population          = 1;
      num_rebels_from_bells_only = 0.1;
      REQUIRE( f() == 0.1 );
      num_rebels_from_bells_only = 0.2;
      REQUIRE( f() == 0.2 );
      num_rebels_from_bells_only = 0.5;
      REQUIRE( f() == 0.5 );
      num_rebels_from_bells_only = 0.9;
      REQUIRE( f() == 0.9 );
      num_rebels_from_bells_only = 1.0;
      REQUIRE( f() == 1.0 );
      num_rebels_from_bells_only = 1.1;
      REQUIRE( f() == 1.0 );
    }
    SECTION( "population 2" ) {
      colony_population          = 2;
      num_rebels_from_bells_only = 0.1;
      REQUIRE( f() == 0.05 );
      num_rebels_from_bells_only = 0.5;
      REQUIRE( f() == 0.25 );
      num_rebels_from_bells_only = 0.9;
      REQUIRE( f() == 0.45 );
      num_rebels_from_bells_only = 1.2;
      REQUIRE( f() == 0.6 );
      num_rebels_from_bells_only = 1.6;
      REQUIRE( f() == 0.8 );
      num_rebels_from_bells_only = 2.0;
      REQUIRE( f() == 1.0 );
      num_rebels_from_bells_only = 3.0;
      REQUIRE( f() == 1.0 );
    }
    SECTION( "population 4" ) {
      colony_population          = 4;
      num_rebels_from_bells_only = 0.2;
      REQUIRE( f() == 0.05 );
      num_rebels_from_bells_only = 0.3;
      REQUIRE( f() == 0.075 );
      num_rebels_from_bells_only = 1.1;
      REQUIRE( f() == 0.275 );
      num_rebels_from_bells_only = 2.3;
      REQUIRE( f() == 0.575 );
      num_rebels_from_bells_only = 3.0;
      REQUIRE( f() == 0.75 );
      num_rebels_from_bells_only = 3.1;
      REQUIRE( f() == 0.775 );
      num_rebels_from_bells_only = 4.0;
      REQUIRE( f() == 1.0 );
    }
  }

  SECTION( "with bolivar" ) {
    has_simon_bolivar = true;
    SECTION( "population 1" ) {
      colony_population          = 1;
      num_rebels_from_bells_only = 0.1;
      REQUIRE( f() == 0.1 + .2 );
      num_rebels_from_bells_only = 0.2;
      REQUIRE( f() == 0.2 + .2 );
      num_rebels_from_bells_only = 0.5;
      REQUIRE( f() == 0.5 + .2 );
      num_rebels_from_bells_only = 0.9;
      REQUIRE( f() == 1.0 );
      num_rebels_from_bells_only = 1.0;
      REQUIRE( f() == 1.0 );
      num_rebels_from_bells_only = 1.1;
      REQUIRE( f() == 1.0 );
    }
    SECTION( "population 2" ) {
      colony_population          = 2;
      num_rebels_from_bells_only = 0.1;
      REQUIRE( f() == 0.05 + .2 );
      num_rebels_from_bells_only = 0.5;
      REQUIRE( f() == 0.25 + .2 );
      num_rebels_from_bells_only = 0.9;
      REQUIRE( f() == 0.45 + .2 );
      num_rebels_from_bells_only = 1.2;
      REQUIRE( f() == 0.6 + .2 );
      num_rebels_from_bells_only = 1.6;
      REQUIRE( f() == 0.8 + .2 );
      num_rebels_from_bells_only = 2.0;
      REQUIRE( f() == 1.0 );
      num_rebels_from_bells_only = 3.0;
      REQUIRE( f() == 1.0 );
    }
    SECTION( "population 4" ) {
      colony_population          = 4;
      num_rebels_from_bells_only = 0.2;
      REQUIRE( f() == 0.05 + .2 );
      num_rebels_from_bells_only = 0.3;
      REQUIRE( f() == 0.075 + .2 );
      num_rebels_from_bells_only = 1.1;
      REQUIRE( f() == 0.275 + .2 );
      num_rebels_from_bells_only = 2.3;
      REQUIRE( f() == 0.575 + .2 );
      num_rebels_from_bells_only = 3.0;
      REQUIRE( f() == 0.75 + .2 );
      num_rebels_from_bells_only = 3.1;
      REQUIRE( f() == 0.775 + .2 );
      num_rebels_from_bells_only = 4.0;
      REQUIRE( f() == 1.0 );
    }
  }
}

TEST_CASE( "[sons-of-liberty] compute_sons_of_liberty_number" ) {
  double sons_of_liberty_integral_percent = 0.0;
  int    colony_population                = 0;

  auto f = [&] {
    return compute_sons_of_liberty_number(
        sons_of_liberty_integral_percent, colony_population );
  };

  SECTION( "population 1" ) {
    colony_population                = 1;
    sons_of_liberty_integral_percent = 0;
    REQUIRE( f() == 0 );
    sons_of_liberty_integral_percent = 10;
    REQUIRE( f() == 0 );
    sons_of_liberty_integral_percent = 40;
    REQUIRE( f() == 0 );
    sons_of_liberty_integral_percent = 70;
    REQUIRE( f() == 0 );
    sons_of_liberty_integral_percent = 99;
    REQUIRE( f() == 0 );
    sons_of_liberty_integral_percent = 100;
    REQUIRE( f() == 1 );
  }
  SECTION( "population 2" ) {
    colony_population                = 2;
    sons_of_liberty_integral_percent = 0;
    REQUIRE( f() == 0 );
    sons_of_liberty_integral_percent = 10;
    REQUIRE( f() == 0 );
    sons_of_liberty_integral_percent = 40;
    REQUIRE( f() == 0 );
    sons_of_liberty_integral_percent = 70;
    REQUIRE( f() == 1 );
    sons_of_liberty_integral_percent = 99;
    REQUIRE( f() == 1 );
    sons_of_liberty_integral_percent = 100;
    REQUIRE( f() == 2 );
  }
  SECTION( "population 4" ) {
    colony_population                = 4;
    sons_of_liberty_integral_percent = 0;
    REQUIRE( f() == 0 );
    sons_of_liberty_integral_percent = 10;
    REQUIRE( f() == 0 );
    sons_of_liberty_integral_percent = 40;
    REQUIRE( f() == 1 );
    sons_of_liberty_integral_percent = 70;
    REQUIRE( f() == 2 );
    sons_of_liberty_integral_percent = 99;
    REQUIRE( f() == 3 );
    sons_of_liberty_integral_percent = 100;
    REQUIRE( f() == 4 );
  }
}

TEST_CASE( "[sons-of-liberty] compute_tory_number" ) {
  int sons_of_liberty_number = 0;
  int colony_population      = 0;

  auto f = [&] {
    return compute_tory_number( sons_of_liberty_number,
                                colony_population );
  };

  SECTION( "population 1" ) {
    colony_population      = 1;
    sons_of_liberty_number = 0;
    REQUIRE( f() == 1 );
    sons_of_liberty_number = 1;
    REQUIRE( f() == 0 );
  }
  SECTION( "population 2" ) {
    colony_population      = 2;
    sons_of_liberty_number = 0;
    REQUIRE( f() == 2 );
    sons_of_liberty_number = 1;
    REQUIRE( f() == 1 );
    sons_of_liberty_number = 2;
    REQUIRE( f() == 0 );
  }
  SECTION( "population 4" ) {
    colony_population      = 4;
    sons_of_liberty_number = 0;
    REQUIRE( f() == 4 );
    sons_of_liberty_number = 1;
    REQUIRE( f() == 3 );
    sons_of_liberty_number = 2;
    REQUIRE( f() == 2 );
    sons_of_liberty_number = 3;
    REQUIRE( f() == 1 );
    sons_of_liberty_number = 4;
    REQUIRE( f() == 0 );
  }
}

TEST_CASE(
    "[sons-of-liberty] "
    "compute_sons_of_liberty_integral_percent" ) {
  auto f = [&]( double sons_of_liberty_percent ) {
    return compute_sons_of_liberty_integral_percent(
        sons_of_liberty_percent );
  };

  REQUIRE( f( 0.0 ) == 0 );
  REQUIRE( f( 0.035 ) == 4 );
  REQUIRE( f( 0.0399 ) == 4 );
  REQUIRE( f( 0.04 ) == 4 );
  REQUIRE( f( 0.14 ) == 14 );
  REQUIRE( f( 0.33333 ) == 33 );
  REQUIRE( f( 0.491 ) == 49 );
  REQUIRE( f( 0.50 ) == 50 );
  REQUIRE( f( 0.501 ) == 50 );
  REQUIRE( f( 0.7 ) == 70 );
  REQUIRE( f( 0.989 ) == 99 );
  REQUIRE( f( 0.99 ) == 99 );
  REQUIRE( f( 0.991 ) == 99 );
  REQUIRE( f( 0.999 ) == 100 );
  REQUIRE( f( 1.0 ) == 100 );
}

TEST_CASE( "[sons-of-liberty] compute_sons_of_liberty_bonus" ) {
  bool is_expert = false;
  auto f = [&]( double sons_of_liberty_integral_percent ) {
    return compute_sons_of_liberty_bonus(
        sons_of_liberty_integral_percent, is_expert );
  };

  REQUIRE( f( 0 ) == 0 );
  REQUIRE( f( 1 ) == 0 );
  REQUIRE( f( 5 ) == 0 );
  REQUIRE( f( 15 ) == 0 );
  REQUIRE( f( 40 ) == 0 );
  REQUIRE( f( 49 ) == 0 );
  REQUIRE( f( 50 ) == 1 );
  REQUIRE( f( 51 ) == 1 );
  REQUIRE( f( 70 ) == 1 );
  REQUIRE( f( 90 ) == 1 );
  REQUIRE( f( 99 ) == 1 );
  REQUIRE( f( 100 ) == 2 );

  is_expert = true;

  REQUIRE( f( 0 ) == 0 );
  REQUIRE( f( 1 ) == 0 );
  REQUIRE( f( 5 ) == 0 );
  REQUIRE( f( 15 ) == 0 );
  REQUIRE( f( 40 ) == 0 );
  REQUIRE( f( 49 ) == 0 );
  REQUIRE( f( 50 ) == 2 );
  REQUIRE( f( 51 ) == 2 );
  REQUIRE( f( 70 ) == 2 );
  REQUIRE( f( 90 ) == 2 );
  REQUIRE( f( 99 ) == 2 );
  REQUIRE( f( 100 ) == 4 );
}

TEST_CASE( "[sons-of-liberty] compute_tory_penalty" ) {
  e_difficulty difficulty = {};

  auto f = [&]( int tory_number ) {
    return compute_tory_penalty( difficulty, tory_number );
  };

  SECTION( "discoverer" ) {
    difficulty = e_difficulty::discoverer;
    REQUIRE( f( 0 ) == 0 );
    REQUIRE( f( 1 ) == 0 );
    REQUIRE( f( 2 ) == 0 );
    REQUIRE( f( 5 ) == 0 );
    REQUIRE( f( 8 ) == 0 );
    REQUIRE( f( 9 ) == 0 );
    REQUIRE( f( 10 ) == 1 );
    REQUIRE( f( 12 ) == 1 );
    REQUIRE( f( 19 ) == 1 );
    REQUIRE( f( 20 ) == 2 );
    REQUIRE( f( 29 ) == 2 );
    REQUIRE( f( 30 ) == 3 );
  }

  SECTION( "conquistador" ) {
    difficulty = e_difficulty::conquistador;
    REQUIRE( f( 0 ) == 0 );
    REQUIRE( f( 1 ) == 0 );
    REQUIRE( f( 2 ) == 0 );
    REQUIRE( f( 5 ) == 0 );
    REQUIRE( f( 7 ) == 0 );
    REQUIRE( f( 8 ) == 1 );
    REQUIRE( f( 9 ) == 1 );
    REQUIRE( f( 10 ) == 1 );
    REQUIRE( f( 12 ) == 1 );
    REQUIRE( f( 15 ) == 1 );
    REQUIRE( f( 16 ) == 2 );
    REQUIRE( f( 19 ) == 2 );
    REQUIRE( f( 20 ) == 2 );
    REQUIRE( f( 23 ) == 2 );
    REQUIRE( f( 24 ) == 3 );
    REQUIRE( f( 30 ) == 3 );
  }

  SECTION( "viceroy" ) {
    difficulty = e_difficulty::viceroy;
    REQUIRE( f( 0 ) == 0 );
    REQUIRE( f( 1 ) == 0 );
    REQUIRE( f( 2 ) == 0 );
    REQUIRE( f( 5 ) == 0 );
    REQUIRE( f( 6 ) == 1 );
    REQUIRE( f( 7 ) == 1 );
    REQUIRE( f( 8 ) == 1 );
    REQUIRE( f( 9 ) == 1 );
    REQUIRE( f( 10 ) == 1 );
    REQUIRE( f( 12 ) == 2 );
    REQUIRE( f( 15 ) == 2 );
    REQUIRE( f( 16 ) == 2 );
    REQUIRE( f( 17 ) == 2 );
    REQUIRE( f( 18 ) == 3 );
    REQUIRE( f( 19 ) == 3 );
    REQUIRE( f( 20 ) == 3 );
    REQUIRE( f( 23 ) == 3 );
    REQUIRE( f( 24 ) == 4 );
    REQUIRE( f( 25 ) == 4 );
    REQUIRE( f( 30 ) == 5 );
  }
}

TEST_CASE(
    "[sons-of-liberty] evolve_num_rebels_from_bells_only" ) {
  int bells_produced    = 0;
  int colony_population = 0;

  auto f = [&]( double num_rebels_from_bells_only ) {
    return evolve_num_rebels_from_bells_only(
        num_rebels_from_bells_only, bells_produced,
        colony_population );
  };

  SECTION( "population == 1" ) {
    colony_population = 1;
    SECTION( "bells == 0" ) {
      bells_produced = 0;
      REQUIRE( f( 0.0 ) == 0.0 );
      REQUIRE( f( 0.2 ) == .2 - 0.004 );
      REQUIRE( f( 0.5 ) == .5 - 0.01 );
      REQUIRE( f( 0.9 ) == .9 - 0.018 );
      REQUIRE( f( 1.0 ) == 1.0 - 0.02 );
      REQUIRE( f( 2.1 ) == 1.0 - 0.02 );
    }

    SECTION( "bells == 1" ) {
      bells_produced = 1;
      REQUIRE( f( 0.0 ) == 0 + 0.01 );
      REQUIRE( f( 0.2 ) == .2 + 0.006 );
      REQUIRE( f( 0.5 ) == .5 + 0.0 );
      REQUIRE( f( 0.9 ) == .9 - 0.008 );
      REQUIRE( f( 1.0 ) == 1.0 - 0.01 );
      REQUIRE( f( 1.5 ) == 1.0 - 0.01 );
    }

    SECTION( "bells == 2" ) {
      bells_produced = 2;
      REQUIRE( f( 0.0 ) == 0 + 0.02 );
      REQUIRE( f( 0.2 ) == .2 + 0.016 );
      REQUIRE( f( 0.5 ) == .5 + 0.01 );
      REQUIRE( f( 0.9 ) == .9 + 0.002 );
      REQUIRE( f( 1.0 ) == 1.0 + 0.0 );
      REQUIRE( f( 3.0 ) == 1.0 + 0.0 );
    }

    SECTION( "bells == 3" ) {
      bells_produced = 3;
      REQUIRE( f( 0.0 ) == 0 + 0.03 );
      REQUIRE( f( 0.2 ) == .2 + 0.026 );
      REQUIRE( f( 0.5 ) == .5 + 0.02 );
      REQUIRE( f( 0.9 ) == .9 + 0.012 );
      REQUIRE( f( 1.0 ) == 1.0 );
      REQUIRE( f( 2.0 ) == 1.0 );
    }

    SECTION( "bells == 10" ) {
      bells_produced = 10;
      REQUIRE( f( 0.0 ) == 0 + 0.1 );
      REQUIRE( f( 0.2 ) == .2 + 0.096 );
      REQUIRE( f( 0.5 ) == .5 + 0.09 );
      REQUIRE( f( 0.9 ) == .9 + 0.082 );
      REQUIRE( f( 1.0 ) == 1.0 );
      REQUIRE( f( 2.0 ) == 1.0 );
    }
  }

  SECTION( "population == 2" ) {
    colony_population = 2;
    SECTION( "bells == 1" ) {
      bells_produced = 1;
      REQUIRE( f( 0.0 ) == 0 + 0.01 );
      REQUIRE( f( 0.2 ) == .2 + 0.006 );
      REQUIRE( f( 0.5 ) == .5 + 0.00 );
      REQUIRE( f( 1.6 ) == 1.6 - 0.022 );
      REQUIRE( f( 2.0 ) == 2.0 - 0.03 );
      REQUIRE( f( 3.0 ) == 2.0 - 0.03 );
    }

    SECTION( "bells == 2" ) {
      bells_produced = 2;
      REQUIRE( f( 0.0 ) == 0 + 0.02 );
      REQUIRE( f( 0.2 ) == .2 + 0.016 );
      REQUIRE( f( 0.5 ) == .5 + 0.01 );
      REQUIRE( f( 1.6 ) == 1.6 - 0.012 );
      REQUIRE( f( 2.0 ) == 2.0 - 0.02 );
      REQUIRE( f( 3.0 ) == 2.0 - 0.02 );
    }

    SECTION( "bells == 3" ) {
      bells_produced = 3;
      REQUIRE( f( 0.0 ) == 0 + 0.03 );
      REQUIRE( f( 0.2 ) == .2 + 0.026 );
      REQUIRE( f( 0.5 ) == .5 + 0.02 );
      REQUIRE( f( 1.6 ) == 1.6 - 0.002 );
      REQUIRE( f( 2.0 ) == 2.0 - 0.01 );
      REQUIRE( f( 3.0 ) == 2.0 - 0.01 );
    }

    SECTION( "bells == 10" ) {
      bells_produced = 10;
      REQUIRE( f( 0.0 ) == 0 + 0.1 );
      REQUIRE( f( 0.2 ) == .2 + 0.096 );
      REQUIRE( f( 0.5 ) == .5 + 0.09 );
      REQUIRE( f( 1.6 ) == 1.6 + 0.068 );
      REQUIRE( f( 2.0 ) == 2.0 + 0.00 );
      REQUIRE( f( 3.0 ) == 2.0 + 0.00 );
    }
  }

  SECTION( "population == 12" ) {
    colony_population = 12;
    SECTION( "bells == 1" ) {
      bells_produced = 1;
      REQUIRE( f( 0.0 ) == 0.0 + 0.01 );
      REQUIRE( f( 0.5 ) == 0.5 + 0.0 );
      REQUIRE( f( 1.6 ) == 1.6 - 0.022 );
      REQUIRE( f( 3.0 ) == 3.0 - 0.05 );
      REQUIRE( f( 6.6 ) == 6.6 - 0.122 );
      REQUIRE( f( 10.9 ) == 10.9 - 0.208 );
      REQUIRE( f( 11.0 ) == 11.0 - 0.21 );
      REQUIRE( f( 12.0 ) == 12.0 - 0.23 );
      REQUIRE( f( 15.0 ) == 12.0 - 0.23 );
    }

    SECTION( "bells == 2" ) {
      bells_produced = 2;
      REQUIRE( f( 0.0 ) == 0.0 + 0.02 );
      REQUIRE( f( 0.5 ) == 0.5 + 0.01 );
      REQUIRE( f( 1.6 ) == 1.6 - 0.012 );
      REQUIRE( f( 3.0 ) == 3.0 - 0.04 );
      REQUIRE( f( 6.6 ) == 6.6 - 0.112 );
      REQUIRE( f( 10.9 ) == 10.9 - 0.198 );
      REQUIRE( f( 11.0 ) == 11.0 - 0.2 );
      REQUIRE( f( 12.0 ) == 12.0 - 0.22 );
      REQUIRE( f( 15.0 ) == 12.0 - 0.22 );
    }

    SECTION( "bells == 6" ) {
      bells_produced = 6;
      REQUIRE( f( 0.0 ) == 0.0 + 0.06 );
      REQUIRE( f( 0.5 ) == 0.5 + 0.05 );
      REQUIRE( f( 1.6 ) == 1.6 + 0.028 );
      REQUIRE( f( 3.0 ) == 3.0 + 0.00 );
      REQUIRE( f( 6.6 ) == 6.6 - 0.072 );
      REQUIRE( f( 10.9 ) == 10.9 - 0.158 );
      REQUIRE( f( 11.0 ) == 11.0 - 0.16 );
      REQUIRE( f( 12.0 ) == 12.0 - 0.18 );
      REQUIRE( f( 15.0 ) == 12.0 - 0.18 );
    }

    SECTION( "bells == 10" ) {
      bells_produced = 10;
      REQUIRE( f( 0.0 ) == 0.0 + 0.1 );
      REQUIRE( f( 0.5 ) == 0.5 + 0.09 );
      REQUIRE( f( 1.6 ) == 1.6 + 0.068 );
      REQUIRE( f( 3.0 ) == 3.0 + 0.04 );
      REQUIRE( f( 6.6 ) == 6.6 - 0.032 );
      REQUIRE( f( 10.9 ) == 10.9 - 0.118 );
      REQUIRE( f( 11.0 ) == 11.0 - 0.12 );
      REQUIRE( f( 12.0 ) == 12.0 - 0.14 );
      REQUIRE( f( 15.0 ) == 12.0 - 0.14 );
    }

    SECTION( "bells == 24" ) {
      bells_produced = 24;
      REQUIRE( f( 0.0 ) == 0.0 + 0.24 );
      REQUIRE( f( 0.5 ) == 0.5 + 0.23 );
      REQUIRE( f( 1.6 ) == 1.6 + 0.208 );
      REQUIRE( f( 3.0 ) == 3.0 + 0.18 );
      REQUIRE( f( 6.6 ) == 6.6 + 0.108 );
      REQUIRE( f( 10.9 ) == 10.9 + 0.022 );
      REQUIRE( f( 11.0 ) == 11.0 + 0.02 );
      REQUIRE( f( 11.95 ) == 11.95 + 0.001 );
      REQUIRE( f( 12.0 ) == 12.0 - 0.0 );
      REQUIRE( f( 15.0 ) == 12.0 - 0.0 );
    }
  }
}

// Some tests on SoL percent evolution rate were done in the
// original game and this test case describes each of those,
// their results, and ensures that our formula replicates those
// to sufficient accuracy.
//
// We can reproduce its behavior surprisingly well, save for the
// asymptotic behavior very close to the equilibrium points. It
// is very likely that the original game's behavior in those
// regimes is not working as intended due to some primitive
// floating point representations and rounding that it likely
// uses. So in our case, our asymptotic behavior is different,
// but still ok.
TEST_CASE( "[sons-of-liberty] asymptotic evolution" ) {
  double num_rebels_from_bells_only       = 0.0;
  int    sons_of_liberty_integral_percent = 0;
  int    bells_produced                   = 0;
  int    colony_population                = 0;

  int turns = 0;

  // First, it was observed that a colony of population 2 and
  // producing three bells per turn (held constant even after the
  // 50% mark by demoting the unit producing the bells) took
  // around 82 turns to hit 60% SoL (with no Bolivar).
  num_rebels_from_bells_only       = 0.0;
  sons_of_liberty_integral_percent = 0;
  bells_produced                   = 3;
  colony_population                = 2;

  while( sons_of_liberty_integral_percent < 60 ) {
    num_rebels_from_bells_only =
        evolve_num_rebels_from_bells_only(
            num_rebels_from_bells_only, bells_produced,
            colony_population );
    ++turns;
    sons_of_liberty_integral_percent =
        compute_sons_of_liberty_integral_percent(
            compute_sons_of_liberty_percent(
                num_rebels_from_bells_only, colony_population,
                /*has_simon_bolivar=*/false ) );
  }

  // Ours happens to be 79 and not 82, but that is close enough.
  // The fact that these are so close is a good testament that we
  // are using the same evolution formula as the original game.
  REQUIRE( turns == 79 );

  // Then, once the colony hit 60%, bell production was upped to
  // 4 per turn, which is exactly the minimum number needed for a
  // colony of population 2 to reach 100% SoL, which it does as-
  // ymptotically, but eventually reaches it in the original
  // game, and this was observed to take 89 turns.
  bells_produced = 4;

  turns = 0;
  while( sons_of_liberty_integral_percent < 100 ) {
    num_rebels_from_bells_only =
        evolve_num_rebels_from_bells_only(
            num_rebels_from_bells_only, bells_produced,
            colony_population );
    ++turns;
    sons_of_liberty_integral_percent =
        compute_sons_of_liberty_integral_percent(
            compute_sons_of_liberty_percent(
                num_rebels_from_bells_only, colony_population,
                /*has_simon_bolivar=*/false ) );
    // So that we don't go into an infinite loop should things
    // not be converging quickly enough.
    if( turns >= 1000 ) break;
  }

  // Ours is 183 turns and not 89, so it's about twice as long,
  // but that is ok because the precise asymptotic behavior of
  // the original game is probably sensitive to some primitive
  // rounding behavior that it uses. What we care about here is
  // that it does terminate after a reasonable number of turns.
  // If the player wants to speed this up then they can tem-
  // porarily increase bell production even by one bell to get it
  // to 100%, then it will stay there.
  REQUIRE( turns == 218 );

  // Second test with population 3 and 7 bells per turn. Seems to
  // have taken around 21 turns in the original game to hit 50%.

  num_rebels_from_bells_only       = 0.0;
  sons_of_liberty_integral_percent = 0;
  bells_produced                   = 7;
  colony_population                = 3;

  turns = 0;
  while( sons_of_liberty_integral_percent < 50 ) {
    num_rebels_from_bells_only =
        evolve_num_rebels_from_bells_only(
            num_rebels_from_bells_only, bells_produced,
            colony_population );
    ++turns;
    sons_of_liberty_integral_percent =
        compute_sons_of_liberty_integral_percent(
            compute_sons_of_liberty_percent(
                num_rebels_from_bells_only, colony_population,
                /*has_simon_bolivar=*/false ) );
  }

  // Close enough to 21.
  REQUIRE( turns == 28 );

  // Now after hitting 50% bell production went up to 9 per turn
  // and it was observed that it took 33 turns to get to 100.
  bells_produced = 9;

  turns = 0;
  while( sons_of_liberty_integral_percent < 100 ) {
    num_rebels_from_bells_only =
        evolve_num_rebels_from_bells_only(
            num_rebels_from_bells_only, bells_produced,
            colony_population );
    ++turns;
    sons_of_liberty_integral_percent =
        compute_sons_of_liberty_integral_percent(
            compute_sons_of_liberty_percent(
                num_rebels_from_bells_only, colony_population,
                /*has_simon_bolivar=*/false ) );
    // So that we don't go into an infinite loop should things
    // not be converging quickly enough.
    if( turns >= 1000 ) break;
  }

  // Close enough to 33. Here we don't have the inconsistency
  // with the original game that we had in the last asymptotic
  // test with a colony population of 2, because in this case we
  // are producing more bells than we need to get a population 3
  // colony to 100%.
  REQUIRE( turns == 34 );

  // Finally, we'll do one where a colony of population 1 and
  // producing one bell (which is the state when the colony is
  // founded) gets to 50%, which is the convergence point. This
  // will again test the asymptotic behavior of the evolution.
  num_rebels_from_bells_only       = 0.0;
  sons_of_liberty_integral_percent = 0;
  bells_produced                   = 1;
  colony_population                = 1;

  turns = 0;
  while( sons_of_liberty_integral_percent < 50 ) {
    num_rebels_from_bells_only =
        evolve_num_rebels_from_bells_only(
            num_rebels_from_bells_only, bells_produced,
            colony_population );
    ++turns;
    sons_of_liberty_integral_percent =
        compute_sons_of_liberty_integral_percent(
            compute_sons_of_liberty_percent(
                num_rebels_from_bells_only, colony_population,
                /*has_simon_bolivar=*/false ) );
    if( turns >= 1000 ) break;
  }

  // We don't really have good information from the original game
  // in this case because in the original game it doesn't con-
  // verge to 50% in this case, it converges to 33%, but it is
  // highly likely that that is due to rounding errors, and so we
  // don't want to replicate that here. So all that we are
  // testing here is that we get to the (correct) convergence
  // point in a finite number of turns.
  //
  // This is a large number of turns, which means that, in prac-
  // tice, it won't be practical for a colony of population 1 to
  // get to the 50% mark with only the default (1) bell produc-
  // tion, unless they want to wait 228 years. But that is prob-
  // ably fine, since we want to encourage the use of statesmen.
  REQUIRE( turns == 228 );
}

TEST_CASE( "[sons-of-liberty] compute_tory_penalty_level" ) {
  auto f = []( e_difficulty difficulty, int tory_number ) {
    return compute_tory_penalty_level( difficulty, tory_number );
  };

  REQUIRE( f( e_difficulty::viceroy, 0 ) == 0 );
  REQUIRE( f( e_difficulty::viceroy, 1 ) == 0 );
  REQUIRE( f( e_difficulty::viceroy, 2 ) == 0 );
  REQUIRE( f( e_difficulty::viceroy, 3 ) == 0 );
  REQUIRE( f( e_difficulty::viceroy, 4 ) == 0 );
  REQUIRE( f( e_difficulty::viceroy, 5 ) == 0 );
  REQUIRE( f( e_difficulty::viceroy, 6 ) == 1 );
  REQUIRE( f( e_difficulty::viceroy, 7 ) == 1 );
  REQUIRE( f( e_difficulty::viceroy, 8 ) == 1 );
  REQUIRE( f( e_difficulty::viceroy, 9 ) == 1 );
  REQUIRE( f( e_difficulty::viceroy, 10 ) == 1 );
  REQUIRE( f( e_difficulty::viceroy, 11 ) == 1 );
  REQUIRE( f( e_difficulty::viceroy, 12 ) == 2 );
  REQUIRE( f( e_difficulty::viceroy, 13 ) == 2 );

  REQUIRE( f( e_difficulty::discoverer, 0 ) == 0 );
  REQUIRE( f( e_difficulty::discoverer, 1 ) == 0 );
  REQUIRE( f( e_difficulty::discoverer, 2 ) == 0 );
  REQUIRE( f( e_difficulty::discoverer, 3 ) == 0 );
  REQUIRE( f( e_difficulty::discoverer, 4 ) == 0 );
  REQUIRE( f( e_difficulty::discoverer, 5 ) == 0 );
  REQUIRE( f( e_difficulty::discoverer, 6 ) == 0 );
  REQUIRE( f( e_difficulty::discoverer, 7 ) == 0 );
  REQUIRE( f( e_difficulty::discoverer, 8 ) == 0 );
  REQUIRE( f( e_difficulty::discoverer, 9 ) == 0 );
  REQUIRE( f( e_difficulty::discoverer, 10 ) == 1 );
  REQUIRE( f( e_difficulty::discoverer, 11 ) == 1 );
  REQUIRE( f( e_difficulty::discoverer, 12 ) == 1 );
  REQUIRE( f( e_difficulty::discoverer, 13 ) == 1 );
  REQUIRE( f( e_difficulty::discoverer, 14 ) == 1 );
  REQUIRE( f( e_difficulty::discoverer, 15 ) == 1 );
  REQUIRE( f( e_difficulty::discoverer, 16 ) == 1 );
  REQUIRE( f( e_difficulty::discoverer, 17 ) == 1 );
  REQUIRE( f( e_difficulty::discoverer, 18 ) == 1 );
  REQUIRE( f( e_difficulty::discoverer, 19 ) == 1 );
  REQUIRE( f( e_difficulty::discoverer, 20 ) == 2 );
  REQUIRE( f( e_difficulty::discoverer, 21 ) == 2 );
  REQUIRE( f( e_difficulty::discoverer, 22 ) == 2 );
  REQUIRE( f( e_difficulty::discoverer, 23 ) == 2 );
  REQUIRE( f( e_difficulty::discoverer, 24 ) == 2 );
  REQUIRE( f( e_difficulty::discoverer, 25 ) == 2 );
  REQUIRE( f( e_difficulty::discoverer, 26 ) == 2 );
}

TEST_CASE( "[sons-of-liberty] compute_colony_sons_of_liberty" ) {
  World   W;
  Colony& colony =
      W.add_colony_with_new_unit( { .x = 1, .y = 1 } );
  W.add_unit_indoors( colony.id, e_indoor_job::hammers );
  W.add_unit_indoors( colony.id, e_indoor_job::hammers );
  W.add_unit_indoors( colony.id, e_indoor_job::hammers );
  Player&             player = W.default_player();
  ColonySonsOfLiberty expected;

  auto f = [&] {
    return compute_colony_sons_of_liberty( player, colony );
  };

  expected = { .sol_integral_percent  = 0,
               .rebels                = 0,
               .tory_integral_percent = 100,
               .tories                = 4 };
  REQUIRE( f() == expected );

  colony.sons_of_liberty.num_rebels_from_bells_only = 1.0;
  expected = { .sol_integral_percent  = 25,
               .rebels                = 1,
               .tory_integral_percent = 75,
               .tories                = 3 };
  REQUIRE( f() == expected );

  colony.sons_of_liberty.num_rebels_from_bells_only = 1.9;
  expected = { .sol_integral_percent  = 48,
               .rebels                = 1,
               .tory_integral_percent = 52,
               .tories                = 3 };
  REQUIRE( f() == expected );

  colony.sons_of_liberty.num_rebels_from_bells_only = 2.0;
  expected = { .sol_integral_percent  = 50,
               .rebels                = 2,
               .tory_integral_percent = 50,
               .tories                = 2 };
  REQUIRE( f() == expected );

  colony.sons_of_liberty.num_rebels_from_bells_only = 4.0;
  expected = { .sol_integral_percent  = 100,
               .rebels                = 4,
               .tory_integral_percent = 0,
               .tories                = 0 };
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
