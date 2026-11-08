/****************************************************************
**gnuplot-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-03-15.
*
* Description: Unit tests for the gnuplot module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/gnuplot.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Data.
*****************************************************************/
string_view const kExpected = R"gnuplot(#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"one","two","three"
"1.2","3.4","4.5"
"2.3","4.5","5.6"
"3.4","5.6","6.7"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "Test Graph"
set key outside right
set grid
set xlabel "Time"
set ylabel "Speed"
set key autotitle columnhead
set xrange [0:1]
set yrange [1:3]
plot for [col=2:*] $CSVData using 1:col with lines lw 3

set output
system sprintf( "eog --fullscreen '%s' >/dev/null 2>&1 &", outfile )
)gnuplot";

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[gnuplot] generate_gnuplot" ) {
  GnuPlotSettings settings;
  settings.title   = "Test Graph";
  settings.x_label = "Time";
  settings.y_label = "Speed";
  settings.x_range = "0:1";
  settings.y_range = "1:3";

  CsvData csv_data;
  csv_data.header = { "one", "two", "three" };
  csv_data.rows   = {
    { "1.2", "3.4", "4.5" },
    { "2.3", "4.5", "5.6" },
    { "3.4", "5.6", "6.7" },
  };

  REQUIRE( generate_gnuplot( settings, csv_data ) == kExpected );
}

} // namespace
} // namespace rn
