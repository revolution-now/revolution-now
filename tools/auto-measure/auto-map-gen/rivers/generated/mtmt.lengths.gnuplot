#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","10.383200"
"5","5.817100"
"6","3.188400"
"7","1.789600"
"8","1.099500"
"9","0.658100"
"10","0.391600"
"11","0.250700"
"12","0.150400"
"13","0.098500"
"14","0.060600"
"15","0.040100"
"16","0.024600"
"17","0.013700"
"18","0.010700"
"19","0.005600"
"20","0.003300"
"21","0.003200"
"22","0.001300"
"23","0.000700"
"24","0.000600"
"25","0.000600"
"26","0.000400"
"27","0.000300"
"28","0.000000"
"29","0.000100"
"30","0.000000"
"31","0.000000"
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
