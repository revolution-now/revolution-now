#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.49"
"2","0.15"
"3","0.14"
"4","10.72"
"5","6.76"
"6","3.06"
"7","2.58"
"8","2.07"
"9","1.38"
"10","0.98"
"11","0.78"
"12","0.76"
"13","0.34"
"14","0.31"
"15","0.22"
"16","0.24"
"17","0.12"
"18","0.15"
"19","0.11"
"20","0.05"
"21","0.04"
"22","0.06"
"23","0.02"
"24","0.01"
"25","0.01"
"26","0.01"
"27","0.02"
"28","0.0"
"29","0.0"
"30","0.0"
"31","0.03"
"32","0.01"
EOF

set title "River Length Histogram (empirical) (mbmb) [100]"
set key outside right
set grid
set xlabel "Length"
set ylabel "Count per Map"
set key autotitle columnhead
set xrange [1:20]
set yrange [0:20]
plot for [col=2:*] $CSVData using 1:col with lines lw 2
