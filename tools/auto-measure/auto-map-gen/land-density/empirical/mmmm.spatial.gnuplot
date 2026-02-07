#!/usr/bin/env -S gnuplot -p
set title "Spatial Land Density (mmmm [2000])"
set datafile separator ","
set key outside right
set grid
set xlabel "X or Y coordinate"
set ylabel "density"

# Use the first row as column headers for titles.
set key autotitle columnhead

set yrange [0:1.0]
set xrange [0:1.0]

# Plot: x is column 1, then plot columns 2..N as separate lines.
plot for [col=2:*] "mmmm.spatial.csv" using 1:col with lines lw 2
