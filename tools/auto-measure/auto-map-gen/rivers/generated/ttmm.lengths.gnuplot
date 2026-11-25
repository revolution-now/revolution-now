#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","11.170900"
"5","4.115200"
"6","1.568200"
"7","0.629500"
"8","0.590000"
"9","0.392400"
"10","0.227800"
"11","0.130800"
"12","0.089100"
"13","0.054100"
"14","0.040700"
"15","0.025500"
"16","0.018100"
"17","0.011300"
"18","0.007700"
"19","0.004500"
"20","0.002400"
"21","0.002300"
"22","0.000600"
"23","0.001500"
"24","0.001000"
"25","0.000500"
"26","0.000000"
"27","0.000100"
"28","0.000100"
"29","0.000200"
"30","0.000000"
"31","0.000000"
"32","0.000000"
"33","0.000000"
"34","0.000000"
"35","0.000000"
"36","0.000000"
"37","0.000000"
"38","0.000000"
"39","0.000100"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (ttmm) [10000]"
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
