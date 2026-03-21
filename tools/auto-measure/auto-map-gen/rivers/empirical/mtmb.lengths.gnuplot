#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.24"
"2","0.12"
"3","0.04"
"4","10.64"
"5","4.45"
"6","1.82"
"7","1.75"
"8","1.16"
"9","0.74"
"10","0.55"
"11","0.35"
"12","0.31"
"13","0.22"
"14","0.09"
"15","0.13"
"16","0.1"
"17","0.09"
"18","0.01"
"19","0.02"
"20","0.02"
"21","0.01"
"22","0.0"
"23","0.0"
"24","0.0"
"25","0.0"
"26","0.0"
"27","0.0"
"28","0.01"
EOF

set title "River Length Histogram (empirical) (mtmb) [100]"
set key outside right
set grid
set xlabel "Length"
set ylabel "Count per Map"
set key autotitle columnhead
set xrange [1:20]
set yrange [0:20]
plot for [col=2:*] $CSVData using 1:col with lines lw 2
