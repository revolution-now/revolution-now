#!/usr/bin/env -S gnuplot -p
set title "Overall Land Density (bmmm [2000])"
set datafile separator ","
set key outside right
set grid
set xlabel "density"
set ylabel "frequency"

# Use the first row as column headers for titles.
set key autotitle columnhead

set yrange [0:*]
set xrange [0:0.5]

# Plot: x is column 1, then plot columns 2..N as separate lines.
plot for [col=2:*] "bmmm.overall.csv" using 1:col with lines lw 2
