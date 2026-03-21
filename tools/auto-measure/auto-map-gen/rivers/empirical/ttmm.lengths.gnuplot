#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.12"
"2","0.0615"
"3","0.04"
"4","7.856"
"5","3.0625"
"6","0.9145"
"7","0.959"
"8","0.704"
"9","0.4065"
"10","0.2505"
"11","0.1865"
"12","0.123"
"13","0.086"
"14","0.0755"
"15","0.0415"
"16","0.0285"
"17","0.0135"
"18","0.011"
"19","0.0085"
"20","0.005"
"21","0.0065"
"22","0.002"
"23","0.0025"
"24","0.0005"
"25","0.0"
"26","0.0005"
"27","0.0005"
"28","0.0"
"29","0.0"
"30","0.0"
"31","0.0"
"32","0.0"
"33","0.001"
"34","0.0005"
EOF

set title "River Length Histogram (empirical) (ttmm) [2000]"
set key outside right
set grid
set xlabel "Length"
set ylabel "Count per Map"
set key autotitle columnhead
set xrange [1:20]
set yrange [0:20]
plot for [col=2:*] $CSVData using 1:col with lines lw 2
