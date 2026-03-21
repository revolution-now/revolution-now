#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","8.188700"
"5","4.886800"
"6","2.909800"
"7","1.754300"
"8","1.111200"
"9","0.715200"
"10","0.475600"
"11","0.313100"
"12","0.203200"
"13","0.139000"
"14","0.087600"
"15","0.056100"
"16","0.037100"
"17","0.025500"
"18","0.019300"
"19","0.013200"
"20","0.008600"
"21","0.005100"
"22","0.004400"
"23","0.003000"
"24","0.002700"
"25","0.001700"
"26","0.000800"
"27","0.000500"
"28","0.000600"
"29","0.000400"
"30","0.000200"
"31","0.000200"
"32","0.000200"
"33","0.000000"
"34","0.000100"
"35","0.000100"
EOF

set title "River Length Histogram (generated) (mmmt) [10000]"
set key outside right
set grid
set xlabel "Length"
set ylabel "Count Per Map"
set key autotitle columnhead
set xrange [1:20]
set yrange [0:20]
plot for [col=2:*] $CSVData using 1:col with lines lw 2
