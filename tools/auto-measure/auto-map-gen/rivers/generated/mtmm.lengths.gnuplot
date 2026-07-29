#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","12.880300"
"5","7.228200"
"6","4.062400"
"7","2.284600"
"8","1.397900"
"9","0.885300"
"10","0.566700"
"11","0.349200"
"12","0.232600"
"13","0.140200"
"14","0.088200"
"15","0.058400"
"16","0.040700"
"17","0.024900"
"18","0.014200"
"19","0.011000"
"20","0.008500"
"21","0.004100"
"22","0.002800"
"23","0.001600"
"24","0.001200"
"25","0.001400"
"26","0.000500"
"27","0.000200"
"28","0.000200"
"29","0.000200"
"30","0.000000"
"31","0.000200"
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
