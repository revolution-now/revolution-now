#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","13.004900"
"5","4.949300"
"6","1.883300"
"7","1.518700"
"8","1.064000"
"9","0.627900"
"10","0.402000"
"11","0.271300"
"12","0.183700"
"13","0.131100"
"14","0.091000"
"15","0.060000"
"16","0.042900"
"17","0.032600"
"18","0.020300"
"19","0.014900"
"20","0.011100"
"21","0.008500"
"22","0.006900"
"23","0.004500"
"24","0.003500"
"25","0.002500"
"26","0.001700"
"27","0.000900"
"28","0.001000"
"29","0.000400"
"30","0.000300"
"31","0.000300"
"32","0.000200"
"33","0.000200"
"34","0.000100"
"35","0.000200"
"36","0.000100"
"37","0.000100"
"38","0.000200"
"39","0.000000"
"40","0.000000"
"41","0.000000"
"42","0.000000"
"43","0.000000"
"44","0.000000"
"45","0.000000"
"46","0.000100"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (mtmb) [10000]"
set key outside right
set grid
set xlabel "Length"
set ylabel "Count Per Map"
set key autotitle columnhead
set xrange [1:20]
set yrange [0:20]
plot for [col=2:*] $CSVData using 1:col with lines lw 3

set output
system sprintf( "eog --fullscreen '%s' >/dev/null 2>&1 &", outfile )
