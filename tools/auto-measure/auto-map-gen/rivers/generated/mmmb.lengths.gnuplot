#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","11.947300"
"5","6.797300"
"6","3.842500"
"7","2.257500"
"8","1.411100"
"9","0.896700"
"10","0.589600"
"11","0.371100"
"12","0.242900"
"13","0.168100"
"14","0.107400"
"15","0.073800"
"16","0.045000"
"17","0.030100"
"18","0.021100"
"19","0.016600"
"20","0.009700"
"21","0.006000"
"22","0.004200"
"23","0.003100"
"24","0.001400"
"25","0.001000"
"26","0.001400"
"27","0.000500"
"28","0.000800"
"29","0.000500"
"30","0.000000"
"31","0.000300"
"32","0.000100"
"33","0.000000"
"34","0.000100"
"35","0.000000"
"36","0.000000"
"37","0.000100"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (mmmb) [10000]"
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
