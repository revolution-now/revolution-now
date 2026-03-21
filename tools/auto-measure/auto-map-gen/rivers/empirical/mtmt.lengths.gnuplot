#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.19"
"2","0.11"
"3","0.03"
"4","9.41"
"5","4.47"
"6","1.52"
"7","1.39"
"8","1.08"
"9","0.56"
"10","0.39"
"11","0.28"
"12","0.29"
"13","0.08"
"14","0.08"
"15","0.11"
"16","0.04"
"17","0.01"
"18","0.04"
"19","0.04"
"20","0.03"
EOF

set title "River Length Histogram (empirical) (mtmt) [100]"
set key outside right
set grid
set xlabel "Length"
set ylabel "Count per Map"
set key autotitle columnhead
set xrange [1:20]
set yrange [0:20]
plot for [col=2:*] $CSVData using 1:col with lines lw 2
