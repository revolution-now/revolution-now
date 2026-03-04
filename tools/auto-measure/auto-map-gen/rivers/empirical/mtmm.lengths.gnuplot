#!/usr/bin/env -S gnuplot -p
set title "River Length Histogram (mtmm [2000])"
set datafile separator ","
set key outside right
set grid
set xlabel "Length"
set ylabel "Count per Map"

# Use the first row as column headers for titles.
set key autotitle columnhead

set xrange [1:20]
set yrange [0:20]

# Plot: x is column 1, then plot columns 2..N as separate lines.
plot for [col=2:*] "mtmm.lengths.csv" using 1:col with lines lw 2
