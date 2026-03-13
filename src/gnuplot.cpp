/****************************************************************
**gnuplot.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-03-12.
*
* Description: Generates GNUPlot files.
*
*****************************************************************/
#include "gnuplot.hpp"

// base
#include "base/logger.hpp"

// C++ standard library.
#include <fstream>

namespace rn {

namespace {

using namespace std;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
void generate_gnuplot( fs::path const& dir,
                       std::string const& stem,
                       GnuPlotSettings const& settings,
                       CsvData const& csv_data ) {
  string const fname = dir / format( "{}.gnuplot", stem );
  lg.info( "writing gnuplot file: {}", fname );
  ofstream out( fname );
  CHECK( out.good(), "failed to open file {}", fname );

  auto const emit_row = [&]( auto const& row ) {
    for( string sep; string const& col : row )
      out << exchange( sep, "," ) << quoted( col );
    out << '\n';
  };

  out << "#!/usr/bin/env -S gnuplot -p\n";
  out << format( "set datafile separator {}\n", settings.sep );
  out << '\n';

  out << "$CSVData << EOF\n";
  emit_row( csv_data.header );
  for( auto const& row : csv_data.rows ) emit_row( row );
  out << "EOF\n";
  out << '\n';

  out << format( "set title \"{}\"\n", settings.title );
  out << "set key outside right\n";
  out << "set grid\n";
  out << format( "set xlabel \"{}\"\n", settings.x_label );
  out << format( "set ylabel \"{}\"\n", settings.y_label );
  out << "set key autotitle columnhead\n";
  if( settings.x_range.has_value() )
    out << format( "set xrange [{}]\n", settings.x_range );
  if( settings.y_range.has_value() )
    out << format( "set yrange [{}]\n", settings.y_range );
  out << "plot for [col=2:*] $CSVData using 1:col with lines lw "
         "2\n";
}

} // namespace rn
