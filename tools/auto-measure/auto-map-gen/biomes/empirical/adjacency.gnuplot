#!/usr/bin/env -S gnuplot -p
set title "Biome Adjacency Histogram"
set datafile separator ","
set key outside right
set grid
set xlabel "Relative Adjacency"
set ylabel "Count"

# Use the first row as column headers for titles.
set key autotitle columnhead

set xrange [.5:2]
set yrange [0:6]

# Plot: x is column 1, then plot columns 2..N as separate lines.
plot for [col=2:*] "adjacency.csv" using 1:col with lines lw 2
