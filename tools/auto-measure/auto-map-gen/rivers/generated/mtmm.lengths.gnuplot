#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","12.797800"
"5","4.898800"
"6","1.888600"
"7","1.480600"
"8","1.040600"
"9","0.622800"
"10","0.394400"
"11","0.266400"
"12","0.175000"
"13","0.129500"
"14","0.078500"
"15","0.060500"
"16","0.044000"
"17","0.029400"
"18","0.020200"
"19","0.016200"
"20","0.011000"
"21","0.008800"
"22","0.005500"
"23","0.003700"
"24","0.003600"
"25","0.002300"
"26","0.001500"
"27","0.001400"
"28","0.000700"
"29","0.000800"
"30","0.000400"
"31","0.000300"
"32","0.000300"
"33","0.000100"
"34","0.000300"
"35","0.000100"
"36","0.000100"
"37","0.000100"
"38","0.000100"
"39","0.000100"
"40","0.000000"
"41","0.000000"
"42","0.000000"
"43","0.000000"
"44","0.000000"
"45","0.000100"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (mtmm) [10000]"
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
