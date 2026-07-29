#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","13.076100"
"5","7.211300"
"6","4.026500"
"7","2.279700"
"8","1.377900"
"9","0.844300"
"10","0.534900"
"11","0.333800"
"12","0.211900"
"13","0.132700"
"14","0.085700"
"15","0.055500"
"16","0.034900"
"17","0.023100"
"18","0.014700"
"19","0.009500"
"20","0.006900"
"21","0.004300"
"22","0.002100"
"23","0.001900"
"24","0.001900"
"25","0.000300"
"26","0.000800"
"27","0.000200"
"28","0.000300"
"29","0.000400"
"30","0.000000"
"31","0.000000"
"32","0.000000"
"33","0.000100"
"34","0.000100"
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
