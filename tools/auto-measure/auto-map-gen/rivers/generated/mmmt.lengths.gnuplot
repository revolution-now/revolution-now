#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","9.695800"
"5","4.546700"
"6","2.153900"
"7","1.591400"
"8","1.139700"
"9","0.755600"
"10","0.512100"
"11","0.358300"
"12","0.255800"
"13","0.195500"
"14","0.134500"
"15","0.098300"
"16","0.073300"
"17","0.055000"
"18","0.039100"
"19","0.028800"
"20","0.023300"
"21","0.016000"
"22","0.010900"
"23","0.006900"
"24","0.006800"
"25","0.005300"
"26","0.003900"
"27","0.003200"
"28","0.002400"
"29","0.001700"
"30","0.001100"
"31","0.001100"
"32","0.001700"
"33","0.000500"
"34","0.000100"
"35","0.000900"
"36","0.000900"
"37","0.000400"
"38","0.000100"
"39","0.000000"
"40","0.000200"
"41","0.000100"
"42","0.000000"
"43","0.000100"
"44","0.000100"
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
