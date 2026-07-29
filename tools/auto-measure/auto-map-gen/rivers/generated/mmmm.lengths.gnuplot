#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","11.061100"
"5","6.224800"
"6","3.559200"
"7","2.082500"
"8","1.273600"
"9","0.809100"
"10","0.512500"
"11","0.325600"
"12","0.194600"
"13","0.134400"
"14","0.087700"
"15","0.055600"
"16","0.033800"
"17","0.024600"
"18","0.014200"
"19","0.013200"
"20","0.009600"
"21","0.005200"
"22","0.003400"
"23","0.002200"
"24","0.001000"
"25","0.001000"
"26","0.000700"
"27","0.000800"
"28","0.000000"
"29","0.000000"
"30","0.000100"
"31","0.000000"
"32","0.000000"
"33","0.000100"
"34","0.000200"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (mmmm) [10000]"
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
