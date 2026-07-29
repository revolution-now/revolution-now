#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","13.955300"
"5","7.883700"
"6","4.429000"
"7","2.541700"
"8","1.570900"
"9","0.999400"
"10","0.636100"
"11","0.404200"
"12","0.266900"
"13","0.167400"
"14","0.110100"
"15","0.066200"
"16","0.047700"
"17","0.029900"
"18","0.022600"
"19","0.015300"
"20","0.010500"
"21","0.005300"
"22","0.004100"
"23","0.002000"
"24","0.001000"
"25","0.001700"
"26","0.001200"
"27","0.000500"
"28","0.000700"
"29","0.000200"
"30","0.000200"
"31","0.000000"
"32","0.000200"
"33","0.000000"
"34","0.000000"
"35","0.000100"
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
