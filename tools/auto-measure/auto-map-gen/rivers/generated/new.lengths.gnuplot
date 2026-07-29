#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","10.370000"
"5","6.320900"
"6","3.904400"
"7","2.448600"
"8","1.617900"
"9","1.105800"
"10","0.753500"
"11","0.515900"
"12","0.364000"
"13","0.255400"
"14","0.188900"
"15","0.131600"
"16","0.093900"
"17","0.072100"
"18","0.047600"
"19","0.037600"
"20","0.023600"
"21","0.019200"
"22","0.016300"
"23","0.011000"
"24","0.008000"
"25","0.006100"
"26","0.004400"
"27","0.003000"
"28","0.002000"
"29","0.002200"
"30","0.001100"
"31","0.000700"
"32","0.000800"
"33","0.001000"
"34","0.000400"
"35","0.000300"
"36","0.000200"
"37","0.000100"
"38","0.000100"
"39","0.000100"
"40","0.000100"
"41","0.000000"
"42","0.000000"
"43","0.000000"
"44","0.000000"
"45","0.000100"
"46","0.000000"
"47","0.000000"
"48","0.000000"
"49","0.000100"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (new) [10000]"
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
