#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","8.914500"
"5","5.523900"
"6","3.444600"
"7","2.282200"
"8","1.508900"
"9","1.034700"
"10","0.698900"
"11","0.489800"
"12","0.348100"
"13","0.245800"
"14","0.182000"
"15","0.121800"
"16","0.088200"
"17","0.064000"
"18","0.051400"
"19","0.035900"
"20","0.027700"
"21","0.020400"
"22","0.014600"
"23","0.011000"
"24","0.008600"
"25","0.005600"
"26","0.004900"
"27","0.004300"
"28","0.001800"
"29","0.002400"
"30","0.001100"
"31","0.001200"
"32","0.000600"
"33","0.000500"
"34","0.000700"
"35","0.000500"
"36","0.000300"
"37","0.000100"
"38","0.000100"
"39","0.000200"
"40","0.000200"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (mbmb) [10000]"
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
