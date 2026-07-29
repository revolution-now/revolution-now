#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","8.064200"
"5","5.066300"
"6","3.229100"
"7","2.043000"
"8","1.378000"
"9","0.944800"
"10","0.660500"
"11","0.463600"
"12","0.332900"
"13","0.234600"
"14","0.165900"
"15","0.112800"
"16","0.090400"
"17","0.064300"
"18","0.049200"
"19","0.033500"
"20","0.025400"
"21","0.020800"
"22","0.011600"
"23","0.009000"
"24","0.007000"
"25","0.005800"
"26","0.005000"
"27","0.002400"
"28","0.002900"
"29","0.001900"
"30","0.001300"
"31","0.001200"
"32","0.001000"
"33","0.000300"
"34","0.000200"
"35","0.000700"
"36","0.000400"
"37","0.000300"
"38","0.000300"
"39","0.000400"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (mbmm) [10000]"
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
