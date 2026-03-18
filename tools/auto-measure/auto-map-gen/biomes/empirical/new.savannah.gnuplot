#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"savannah_distance","frequency"
"0","0"
"1","0"
"2","0"
"3","0"
"4","0.002"
"5","0.0025"
"6","0.014"
"7","0.0605"
"8","0.1935"
"9","0.0705"
"10","0.177"
"11","0.0635"
"12","0.176"
"13","0.064"
"14","0.1765"
EOF

set title "Savannah Row Limit Histograph (empirical) (new) [2000]"
set key outside right
set grid
set xlabel "Distance from Equator"
set ylabel "Frequency"
set key autotitle columnhead
set xrange [0:20]
set yrange [0:1]
plot for [col=2:*] $CSVData using 1:col with lines lw 2
