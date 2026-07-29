#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","8.744400"
"5","4.924400"
"6","2.825600"
"7","1.652800"
"8","1.015400"
"9","0.641900"
"10","0.403500"
"11","0.257500"
"12","0.157200"
"13","0.101400"
"14","0.064900"
"15","0.042500"
"16","0.028900"
"17","0.021000"
"18","0.014200"
"19","0.007900"
"20","0.005700"
"21","0.003900"
"22","0.002700"
"23","0.001700"
"24","0.001000"
"25","0.000400"
"26","0.000400"
"27","0.000300"
"28","0.000200"
"29","0.000200"
"30","0.000000"
"31","0.000000"
"32","0.000000"
"33","0.000100"
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
