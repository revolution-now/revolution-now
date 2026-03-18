#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"savannah_distance","frequency"
"0","0"
"1","0"
"2","0"
"3","0"
"4","0"
"5","0"
"6","0.004"
"7","0.1755"
"8","0.8205"
EOF

set title "Savannah Row Limit Histograph (empirical) (bbtm) [2000]"
set key outside right
set grid
set xlabel "Distance from Equator"
set ylabel "Frequency"
set key autotitle columnhead
set xrange [0:20]
set yrange [0:1]
plot for [col=2:*] $CSVData using 1:col with lines lw 2
