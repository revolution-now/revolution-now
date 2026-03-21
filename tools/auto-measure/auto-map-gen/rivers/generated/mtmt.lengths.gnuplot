#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","9.598700"
"5","5.677700"
"6","3.323500"
"7","1.997100"
"8","1.233300"
"9","0.791500"
"10","0.495000"
"11","0.325300"
"12","0.209400"
"13","0.144000"
"14","0.085800"
"15","0.058400"
"16","0.039800"
"17","0.025600"
"18","0.016300"
"19","0.012300"
"20","0.007300"
"21","0.004800"
"22","0.004500"
"23","0.002200"
"24","0.002200"
"25","0.000200"
"26","0.000300"
"27","0.000300"
"28","0.000100"
"29","0.000300"
"30","0.000000"
"31","0.000100"
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
