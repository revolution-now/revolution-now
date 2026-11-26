#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","7.674000"
"5","3.471400"
"6","1.637000"
"7","1.269600"
"8","0.928200"
"9","0.601900"
"10","0.415400"
"11","0.299900"
"12","0.217000"
"13","0.152400"
"14","0.110600"
"15","0.083500"
"16","0.060700"
"17","0.041800"
"18","0.030000"
"19","0.019500"
"20","0.018300"
"21","0.013100"
"22","0.009100"
"23","0.007200"
"24","0.006200"
"25","0.003300"
"26","0.003600"
"27","0.002900"
"28","0.001900"
"29","0.001700"
"30","0.000700"
"31","0.000800"
"32","0.000300"
"33","0.000500"
"34","0.000400"
"35","0.000300"
"36","0.000100"
"37","0.000100"
"38","0.000100"
"39","0.000000"
"40","0.000200"
"41","0.000000"
"42","0.000000"
"43","0.000100"
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
