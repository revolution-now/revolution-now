#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.13"
"2","0.05"
"3","0.03"
"4","7.68"
"5","2.88"
"6","0.94"
"7","0.98"
"8","0.78"
"9","0.38"
"10","0.31"
"11","0.18"
"12","0.09"
"13","0.1"
"14","0.13"
"15","0.07"
"16","0.01"
"17","0.03"
"18","0.01"
"19","0.0"
"20","0.01"
EOF

set title "River Length Histogram (empirical) (ttmb) [100]"
set key outside right
set grid
set xlabel "Length"
set ylabel "Count per Map"
set key autotitle columnhead
set xrange [1:20]
set yrange [0:20]
plot for [col=2:*] $CSVData using 1:col with lines lw 2
