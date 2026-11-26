#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","7.695800"
"5","3.482500"
"6","1.628800"
"7","1.256800"
"8","0.917000"
"9","0.600400"
"10","0.414100"
"11","0.297500"
"12","0.211800"
"13","0.159800"
"14","0.108300"
"15","0.084100"
"16","0.057500"
"17","0.041000"
"18","0.031000"
"19","0.025700"
"20","0.014300"
"21","0.011200"
"22","0.008300"
"23","0.005400"
"24","0.004400"
"25","0.003900"
"26","0.003300"
"27","0.002500"
"28","0.001400"
"29","0.001000"
"30","0.001100"
"31","0.000500"
"32","0.000600"
"33","0.000400"
"34","0.000400"
"35","0.000200"
"36","0.000200"
"37","0.000000"
"38","0.000100"
"39","0.000000"
"40","0.000000"
"41","0.000000"
"42","0.000100"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (tmmm) [10000]"
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
