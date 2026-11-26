#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","12.409100"
"5","4.756800"
"6","1.856800"
"7","1.428900"
"8","0.961600"
"9","0.573400"
"10","0.377100"
"11","0.251600"
"12","0.163800"
"13","0.109900"
"14","0.072700"
"15","0.050400"
"16","0.036500"
"17","0.023600"
"18","0.015300"
"19","0.012900"
"20","0.010100"
"21","0.006600"
"22","0.004400"
"23","0.002700"
"24","0.002300"
"25","0.002100"
"26","0.001700"
"27","0.001000"
"28","0.000400"
"29","0.000700"
"30","0.000600"
"31","0.000100"
"32","0.000100"
"33","0.000000"
"34","0.000100"
"35","0.000100"
"36","0.000000"
"37","0.000000"
"38","0.000100"
"39","0.000000"
"40","0.000000"
"41","0.000000"
"42","0.000000"
"43","0.000000"
"44","0.000000"
"45","0.000100"
"46","0.000000"
"47","0.000100"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (mtmt) [10000]"
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
