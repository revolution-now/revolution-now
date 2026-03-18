#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"density","frequency"
"0.0","1.0"
EOF

set title "Desert Center Density Histograph (empirical) (bbbm) [2000]"
set key outside right
set grid
set xlabel "Density"
set ylabel "Frequency"
set key autotitle columnhead
set xrange [0:0.3]
set yrange [0:0.02]
plot for [col=2:*] $CSVData using 1:col with lines lw 2
