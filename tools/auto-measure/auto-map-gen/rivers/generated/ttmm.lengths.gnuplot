#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","10.214200"
"5","3.640600"
"6","1.345600"
"7","1.102600"
"8","0.777400"
"9","0.452300"
"10","0.282800"
"11","0.187400"
"12","0.126600"
"13","0.080000"
"14","0.054600"
"15","0.039400"
"16","0.025100"
"17","0.019100"
"18","0.012100"
"19","0.009400"
"20","0.006800"
"21","0.003900"
"22","0.003500"
"23","0.001900"
"24","0.001600"
"25","0.001300"
"26","0.000700"
"27","0.000600"
"28","0.000300"
"29","0.000100"
"30","0.000100"
"31","0.000300"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (ttmm) [10000]"
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
