#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","14.173100"
"5","7.883500"
"6","4.384400"
"7","2.520800"
"8","1.522700"
"9","0.964800"
"10","0.603200"
"11","0.388400"
"12","0.241400"
"13","0.158700"
"14","0.096000"
"15","0.065400"
"16","0.040000"
"17","0.023100"
"18","0.017500"
"19","0.009800"
"20","0.008800"
"21","0.004800"
"22","0.003600"
"23","0.003100"
"24","0.000800"
"25","0.000700"
"26","0.000600"
"27","0.000100"
"28","0.000900"
"29","0.000100"
"30","0.000200"
"31","0.000100"
"32","0.000300"
"33","0.000100"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (mtmb) [10000]"
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
