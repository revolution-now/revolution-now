#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","10.302100"
"5","5.747800"
"6","3.214300"
"7","1.829700"
"8","1.111400"
"9","0.682200"
"10","0.424300"
"11","0.268000"
"12","0.164800"
"13","0.103000"
"14","0.063100"
"15","0.043500"
"16","0.027400"
"17","0.018300"
"18","0.009200"
"19","0.007900"
"20","0.006400"
"21","0.002100"
"22","0.001900"
"23","0.000900"
"24","0.000700"
"25","0.000900"
"26","0.000400"
"27","0.000300"
"28","0.000100"
"29","0.000000"
"30","0.000000"
"31","0.000100"
"32","0.000100"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (mtmt) [10000]"
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
