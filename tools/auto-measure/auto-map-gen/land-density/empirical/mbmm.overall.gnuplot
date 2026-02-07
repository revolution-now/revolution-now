#!/usr/bin/env -S gnuplot -p
set title "Overall Land Density (mbmm [2000])"
set datafile separator ","
set key outside right
set grid
set xlabel "X or Y coordinate"
set ylabel "density"

# Use the first row as column headers for titles.
set key autotitle columnhead

set yrange [0:1.0]
set xrange [0:0.5]

# Plot: x is column 1, then plot columns 2..N as separate lines.
plot for [col=2:*] "mbmm.overall.csv" using 1:col with lines lw 2
