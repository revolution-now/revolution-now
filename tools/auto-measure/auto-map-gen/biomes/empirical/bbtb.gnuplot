#!/usr/bin/env -S gnuplot -p
set title "Terrain Distribution (bbtb [2000])"
set datafile separator ","
set key outside right
set grid
set xlabel "Map Row (Y)"
set ylabel "value"

# Use the first row as column headers for titles.
set key autotitle columnhead

set yrange [0:0.7]

# Plot: x is column 1, then plot columns 2..N as separate lines.
plot for [col=2:*] "bbtb.csv" using 1:col with lines lw 2
