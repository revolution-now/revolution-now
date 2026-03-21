#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.1"
"2","0.03"
"3","0.04"
"4","6.84"
"5","2.78"
"6","0.94"
"7","0.8"
"8","0.56"
"9","0.33"
"10","0.22"
"11","0.15"
"12","0.16"
"13","0.09"
"14","0.08"
"15","0.03"
"16","0.03"
"17","0.03"
"18","0.02"
EOF

set title "River Length Histogram (empirical) (ttmt) [100]"
set key outside right
set grid
set xlabel "Length"
set ylabel "Count per Map"
set key autotitle columnhead
set xrange [1:20]
set yrange [0:20]
plot for [col=2:*] $CSVData using 1:col with lines lw 2
