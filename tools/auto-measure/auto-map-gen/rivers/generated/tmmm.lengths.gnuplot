#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","8.748500"
"5","3.992100"
"6","1.886800"
"7","0.944300"
"8","0.789000"
"9","0.561600"
"10","0.373800"
"11","0.247500"
"12","0.169500"
"13","0.132800"
"14","0.091300"
"15","0.063100"
"16","0.042500"
"17","0.034600"
"18","0.026700"
"19","0.017300"
"20","0.013600"
"21","0.010600"
"22","0.007700"
"23","0.005000"
"24","0.004400"
"25","0.002500"
"26","0.001900"
"27","0.001300"
"28","0.001300"
"29","0.001000"
"30","0.000700"
"31","0.000500"
"32","0.000300"
"33","0.000300"
"34","0.000300"
"35","0.000100"
"36","0.000200"
"37","0.000100"
"38","0.000000"
"39","0.000000"
"40","0.000000"
"41","0.000000"
"42","0.000000"
"43","0.000000"
"44","0.000000"
"45","0.000100"
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
