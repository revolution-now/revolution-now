#!/usr/bin/env -S gnuplot -p
      set title "Spatial Land Density (generated) (mode [2000])"
      set datafile separator ","
      set key outside right
      set grid
      set xlabel "X or Y coordinate"
      set ylabel "density"

      # Use the first row as column headers for titles.
      set key autotitle columnhead

      set yrange [0:1.0]
      set xrange [0:1.0]

      plot for [col=2:*] "generated.csv" using 1:col with lines lw 2