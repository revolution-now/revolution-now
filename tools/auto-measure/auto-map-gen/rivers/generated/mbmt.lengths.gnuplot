#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","6.436100"
"5","4.051700"
"6","2.567700"
"7","1.650300"
"8","1.110700"
"9","0.734400"
"10","0.515300"
"11","0.342600"
"12","0.238800"
"13","0.158700"
"14","0.117100"
"15","0.084000"
"16","0.061600"
"17","0.044000"
"18","0.031000"
"19","0.023500"
"20","0.015000"
"21","0.013300"
"22","0.008800"
"23","0.005700"
"24","0.004300"
"25","0.004000"
"26","0.002800"
"27","0.001800"
"28","0.001000"
"29","0.000500"
"30","0.000800"
"31","0.000100"
"32","0.000300"
"33","0.000600"
"34","0.000300"
"35","0.000500"
"36","0.000000"
"37","0.000300"
"38","0.000100"
"39","0.000000"
"40","0.000100"
"41","0.000100"
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
