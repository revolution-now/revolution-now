#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","12.855400"
"5","4.833800"
"6","1.890200"
"7","1.485000"
"8","1.030300"
"9","0.617800"
"10","0.384000"
"11","0.272900"
"12","0.181500"
"13","0.129600"
"14","0.084400"
"15","0.060000"
"16","0.040700"
"17","0.026200"
"18","0.020500"
"19","0.015900"
"20","0.010400"
"21","0.006700"
"22","0.006200"
"23","0.004100"
"24","0.002600"
"25","0.001400"
"26","0.001900"
"27","0.000900"
"28","0.000600"
"29","0.000600"
"30","0.000800"
"31","0.000300"
"32","0.000100"
"33","0.000300"
"34","0.000300"
"35","0.000100"
"36","0.000200"
"37","0.000000"
"38","0.000000"
"39","0.000000"
"40","0.000000"
"41","0.000100"
"42","0.000100"
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
