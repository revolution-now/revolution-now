#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","10.222600"
"5","6.420300"
"6","4.102700"
"7","2.626400"
"8","1.782700"
"9","1.267800"
"10","0.867800"
"11","0.621100"
"12","0.445000"
"13","0.312600"
"14","0.226400"
"15","0.171700"
"16","0.125000"
"17","0.086000"
"18","0.065800"
"19","0.047500"
"20","0.037100"
"21","0.028900"
"22","0.019800"
"23","0.016600"
"24","0.012200"
"25","0.008900"
"26","0.006800"
"27","0.005200"
"28","0.002900"
"29","0.003300"
"30","0.001700"
"31","0.001200"
"32","0.001100"
"33","0.000400"
"34","0.000300"
"35","0.001100"
"36","0.000400"
"37","0.000200"
"38","0.000400"
"39","0.000400"
"40","0.000200"
"41","0.000000"
"42","0.000400"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (bbmm) [10000]"
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
