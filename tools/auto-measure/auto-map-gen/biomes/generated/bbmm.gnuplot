#!/usr/bin/env -S gnuplot -p
      set title "Biome Density (generated) (bbmm [2000])"
      set datafile separator ","
      set key outside right
      set grid
      set xlabel "Map Row (Y)"
      set ylabel "Density"

      # Use the first row as column headers for titles.
      set key autotitle columnhead

      set yrange [0:0.7]
      set xrange [0:70]

      plot for [col=2:*] "bbmm.csv" using 1:col with lines lw 2