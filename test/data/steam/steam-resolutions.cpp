/****************************************************************
**steam-resolutions.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-11-01.
*
* Description: Monitor/resolution data from Steam used for
*              testing.
*
*****************************************************************/
#include "steam-resolutions.hpp"

using namespace std;

namespace testing {

namespace {

using ::gfx::size;

// These are the most common primary-monitor resolutions that
// players have as reported by the the Sept. 2024 Steam hardware
// survey on monitor geometry. Specifically, it tests that they
// are bucketed correctly.
//
// A couple super weird and rare ones were omitted, but most of
// them were included along with frequencies.
vector<size> const& steam_resolutions_sept_2024() {
  static vector<size> const resolutions{
    size{ .w = 1920, .h = 1080 }, // 0  : 55.73%
    size{ .w = 2560, .h = 1440 }, // 1  : 21.73%
    size{ .w = 2560, .h = 1600 }, // 2  :  4.30%
    size{ .w = 3840, .h = 2160 }, // 3  :  3.68%
    size{ .w = 1366, .h = 768 },  // 4  :  2.85%
    size{ .w = 3440, .h = 1440 }, // 5  :  2.22%
    size{ .w = 1920, .h = 1200 }, // 6  :  1.32%
    size{ .w = 1600, .h = 900 },  // 7  :  0.87%
    size{ .w = 1440, .h = 900 },  // 8  :  0.87%
    size{ .w = 2560, .h = 1080 }, // 9  :  0.81%
    size{ .w = 1680, .h = 1050 }, // 10 :  0.53%
    size{ .w = 1360, .h = 768 },  // 11 :  0.52%
    size{ .w = 1280, .h = 800 },  // 12 :  0.38%
    size{ .w = 2880, .h = 1800 }, // 13 :  0.35%
    size{ .w = 5120, .h = 1440 }, // 14 :  0.28%
    size{ .w = 1280, .h = 1024 }, // 15 :  0.28%
  };
  return resolutions;
};

} // namespace

vector<size> const& steam_resolutions() {
  return steam_resolutions_sept_2024();
};

} // namespace testing
