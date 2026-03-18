/****************************************************************
**gnuplot.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-03-12.
*
* Description: Generates GNUPlot files.
*
*****************************************************************/
#pragma once

// base
#include "base/fs.hpp"
#include "base/maybe.hpp"

// C++ standard library
#include <string>
#include <vector>

namespace rn {

struct GnuPlotSettings {
  std::string title   = "Unnamed Graph";
  std::string x_label = "X";
  std::string y_label = "Y";
  base::maybe<std::string> x_range;
  base::maybe<std::string> y_range;
};

struct CsvData {
  std::vector<std::string> header;
  std::vector<std::vector<std::string>> rows;
};

// Generates a self-contained gnuplot file that, when executed,
// will display the graph of the CSV data contained inside of it.
void generate_gnuplot( fs::path const& dir,
                       std::string const& stem,
                       GnuPlotSettings const& settings,
                       CsvData const& csv_data );

std::string generate_gnuplot( GnuPlotSettings const& settings,
                              CsvData const& csv_data );

} // namespace rn
