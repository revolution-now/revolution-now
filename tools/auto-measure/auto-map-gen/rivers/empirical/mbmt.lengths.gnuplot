#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.16"
"2","0.13"
"3","0.05"
"4","7.12"
"5","4.54"
"6","2.6"
"7","1.97"
"8","1.24"
"9","0.94"
"10","0.67"
"11","0.45"
"12","0.26"
"13","0.19"
"14","0.15"
"15","0.07"
"16","0.07"
"17","0.12"
"18","0.04"
"19","0.03"
"20","0.03"
"21","0.03"
"22","0.01"
EOF

set title "River Length Histogram (empirical) (mbmt) [100]"
set key outside right
set grid
set xlabel "Length"
set ylabel "Count per Map"
set key autotitle columnhead
set xrange [1:20]
set yrange [0:20]
plot for [col=2:*] $CSVData using 1:col with lines lw 2
