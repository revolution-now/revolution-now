#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","9.599700"
"5","5.650900"
"6","3.333000"
"7","1.938500"
"8","1.241000"
"9","0.789200"
"10","0.513900"
"11","0.320000"
"12","0.208000"
"13","0.144400"
"14","0.089500"
"15","0.061800"
"16","0.043300"
"17","0.027300"
"18","0.015100"
"19","0.012100"
"20","0.005900"
"21","0.005900"
"22","0.003100"
"23","0.002900"
"24","0.001600"
"25","0.000700"
"26","0.000500"
"27","0.000400"
"28","0.000300"
"29","0.000300"
"30","0.000000"
"31","0.000000"
"32","0.000200"
"33","0.000100"
EOF

set title "River Length Histogram (generated) (mtmt) [10000]"
set key outside right
set grid
set xlabel "Length"
set ylabel "Count Per Map"
set key autotitle columnhead
set xrange [1:20]
set yrange [0:20]
plot for [col=2:*] $CSVData using 1:col with lines lw 2
