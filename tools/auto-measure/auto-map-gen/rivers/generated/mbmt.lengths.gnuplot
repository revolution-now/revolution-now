#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","6.563200"
"5","4.083800"
"6","2.552800"
"7","1.634600"
"8","1.083000"
"9","0.716400"
"10","0.480200"
"11","0.335300"
"12","0.237400"
"13","0.167200"
"14","0.110100"
"15","0.075700"
"16","0.053800"
"17","0.037400"
"18","0.028300"
"19","0.020200"
"20","0.015500"
"21","0.012300"
"22","0.007900"
"23","0.005200"
"24","0.004300"
"25","0.002600"
"26","0.002700"
"27","0.001100"
"28","0.000900"
"29","0.000400"
"30","0.000500"
"31","0.000400"
"32","0.000300"
"33","0.000000"
"34","0.000300"
"35","0.000200"
"36","0.000100"
"37","0.000000"
"38","0.000100"
"39","0.000100"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (mbmt) [10000]"
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
