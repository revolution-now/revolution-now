#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","8.839700"
"5","4.977500"
"6","2.866500"
"7","1.627600"
"8","0.993100"
"9","0.619300"
"10","0.379000"
"11","0.246300"
"12","0.148300"
"13","0.095600"
"14","0.059600"
"15","0.039300"
"16","0.027000"
"17","0.015600"
"18","0.010600"
"19","0.008200"
"20","0.004600"
"21","0.003300"
"22","0.002100"
"23","0.001000"
"24","0.001100"
"25","0.000900"
"26","0.000200"
"27","0.000300"
"28","0.000100"
"29","0.000200"
"30","0.000100"
"31","0.000100"
"32","0.000100"
"33","0.000000"
"34","0.000200"
"35","0.000000"
"36","0.000000"
"37","0.000100"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (mmmt) [10000]"
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
