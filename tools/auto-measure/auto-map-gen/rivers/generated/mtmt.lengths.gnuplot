#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","9.590800"
"5","5.641700"
"6","3.303700"
"7","1.975100"
"8","1.236300"
"9","0.766300"
"10","0.513000"
"11","0.331800"
"12","0.207800"
"13","0.129500"
"14","0.089200"
"15","0.062300"
"16","0.041600"
"17","0.025300"
"18","0.017300"
"19","0.010100"
"20","0.009000"
"21","0.004400"
"22","0.004100"
"23","0.002700"
"24","0.000700"
"25","0.000900"
"26","0.000200"
"27","0.000400"
"28","0.000700"
"29","0.000200"
"30","0.000100"
"31","0.000000"
"32","0.000100"
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
