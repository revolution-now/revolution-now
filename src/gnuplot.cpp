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
#include "base/io.hpp"
#include "base/logger.hpp"
#include "base/string.hpp"
#include "base/to-str-ext-std.hpp"

// C++ standard library.
#include <charconv>
#include <cmath>
#include <fstream>
#include <ranges>
#include <sstream>

namespace rg = ::std::ranges;
namespace rv = ::std::ranges::views;

namespace rn {

namespace {

using namespace std;

using ::rv::iota;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
string generate_gnuplot( GnuPlotSettings const& settings,
                         CsvData const& csv_data ) {
  ostringstream out;

  auto const emit_row = [&]( auto const& row ) {
    for( string sep; string const& col : row )
      out << exchange( sep, "," ) << quoted( col );
    out << '\n';
  };

  out << "#!/usr/bin/env -S gnuplot -p\n";
  out << format( "set datafile separator comma\n" );
  out << '\n';

  out << "$CSVData << EOF\n";
  emit_row( csv_data.header );
  for( auto const& row : csv_data.rows ) emit_row( row );
  out << "EOF\n";
  out << '\n';
  out << "outfile = system( \"mktemp /tmp/gnuplot-XXXXXX.png\" "
         ")\n";
  out << '\n';
  out << "set term png size 1920,1200 font \"Fira Sans,14\"\n";
  out << "set output outfile\n";
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
         "3\n";
  out << '\n';
  out << "set output\n";
  out << "system sprintf( \"eog --fullscreen '%s' >/dev/null "
         "2>&1 &\", outfile )\n";
  return out.str();
}

void generate_gnuplot( fs::path const& dir,
                       std::string const& stem,
                       GnuPlotSettings const& settings,
                       CsvData const& csv_data ) {
  string const fname = dir / format( "{}.gnuplot", stem );
  lg.info( "writing gnuplot file: {}", fname );
  ofstream out( fname );
  CHECK( out.good(), "failed to open file {}", fname );
  out << generate_gnuplot( settings, csv_data );
}

namespace {

struct ExtractedRow {
  double idx = {};
  vector<double> cols;
};

struct ExtractedPlotData {
  string header;
  vector<ExtractedRow> rows;
};

ExtractedPlotData extract_plot_data( fs::path const& p ) {
  ExtractedPlotData res;
  vector<string> lines;
  CHECK( base::read_file_lines( p, lines ),
         "failed to read file {}", p );
  CHECK( lines.size() > 5 );
  CHECK( lines[3] == "$CSVData << EOF" );
  lines.erase( lines.begin(), lines.begin() + 4 );
  auto const eof_it = rg::find( lines, "EOF" );
  CHECK( eof_it != lines.end(), "cannot find EOF" );
  lines.erase( eof_it, lines.end() );
  CHECK_GT( lines.size(), 0u );

  auto const split_line = []( string const& line ) {
    ExtractedRow row;
    auto entries = base::str_split( line, ',' );
    for( int idx = 0; string const& entry : entries ) {
      CHECK( !entry.empty() );
      CHECK( entry[0] == '"' );
      CHECK( entry[entry.size() - 1] == '"' );
      string const val_str = entry.substr( 1, entry.size() - 1 );
      double val           = {};
      auto const [ptr, ec] = from_chars(
          val_str.data(), val_str.data() + val_str.size(), val );
      CHECK( ec == std::errc{}, "failed to parse '{}'",
             val_str );
      if( idx++ == 0 ) {
        row.idx = val;
        continue;
      }
      row.cols.push_back( val );
    }
    return row;
  };

  res.header = lines[0];
  lines.erase( lines.begin() );
  for( string const& line : lines )
    res.rows.push_back( split_line( line ) );
  return res;
}

} // namespace

double rms_plots( fs::path const& l, fs::path const& r,
                  int const cut ) {
  ExtractedPlotData l_data = extract_plot_data( l );
  ExtractedPlotData r_data = extract_plot_data( r );
  CHECK( l_data.header == r_data.header, "header mismatch" );
  CHECK_EQ( l_data.rows.size(), r_data.rows.size() );
  CHECK_GE( cut, 0 );
  for( int const _ : iota( 0, cut ) ) {
    if( !l_data.rows.empty() )
      l_data.rows.erase( l_data.rows.begin() );
    if( !r_data.rows.empty() )
      r_data.rows.erase( r_data.rows.begin() );
    if( !l_data.rows.empty() ) l_data.rows.pop_back();
    if( !r_data.rows.empty() ) r_data.rows.pop_back();
  }
  CHECK_EQ( l_data.rows.size(), r_data.rows.size() );
  double err = 0.0;
  int terms  = 0;
  for( int const row : iota( 0u, l_data.rows.size() ) ) {
    auto const& l_row = l_data.rows[row];
    auto const& r_row = r_data.rows[row];
    CHECK( l_row.idx == r_row.idx, "row mismatch: {}", row );
    auto const& l_cols = l_row.cols;
    auto const& r_cols = r_row.cols;
    CHECK( !l_cols.empty(), "row {} empty", row );
    CHECK( !r_cols.empty() );
    CHECK_EQ( l_cols.size(), r_cols.size() );
    for( auto const [l_val, r_val] :
         rv::zip( l_cols, r_cols ) ) {
      ++terms;
      double const diff = l_val - r_val;
      err += diff * diff;
    }
  }
  err /= terms;
  err = sqrt( err );
  CHECK_GE( err, 0.0 );
  return err;
}

} // namespace rn
