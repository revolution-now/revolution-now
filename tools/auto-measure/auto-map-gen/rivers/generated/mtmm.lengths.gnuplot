#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","12.036400"
"5","7.067200"
"6","4.139500"
"7","2.499700"
"8","1.568300"
"9","1.021100"
"10","0.639000"
"11","0.443900"
"12","0.277400"
"13","0.181500"
"14","0.127400"
"15","0.085200"
"16","0.059600"
"17","0.040100"
"18","0.027000"
"19","0.017500"
"20","0.014900"
"21","0.006300"
"22","0.004900"
"23","0.003800"
"24","0.002300"
"25","0.002200"
"26","0.001100"
"27","0.000600"
"28","0.000400"
"29","0.000500"
"30","0.000200"
"31","0.000000"
"32","0.000000"
"33","0.000000"
"34","0.000100"
EOF

set title "River Length Histogram (generated) (mtmm) [10000]"
set key outside right
set grid
set xlabel "Length"
set ylabel "Count Per Map"
set key autotitle columnhead
set xrange [1:20]
set yrange [0:20]
plot for [col=2:*] $CSVData using 1:col with lines lw 2
