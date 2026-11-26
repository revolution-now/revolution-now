#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","10.151600"
"5","3.634800"
"6","1.359200"
"7","1.121400"
"8","0.776200"
"9","0.426800"
"10","0.282900"
"11","0.187200"
"12","0.124700"
"13","0.083900"
"14","0.056700"
"15","0.035800"
"16","0.026500"
"17","0.019600"
"18","0.015300"
"19","0.009400"
"20","0.006200"
"21","0.004700"
"22","0.003200"
"23","0.001700"
"24","0.000800"
"25","0.000800"
"26","0.000200"
"27","0.000600"
"28","0.000600"
"29","0.000500"
"30","0.000300"
"31","0.000100"
"32","0.000100"
"33","0.000100"
"34","0.000000"
"35","0.000100"
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
