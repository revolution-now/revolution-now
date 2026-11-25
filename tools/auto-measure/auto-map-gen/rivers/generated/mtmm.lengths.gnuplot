#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","13.865900"
"5","5.383800"
"6","2.157400"
"7","0.877500"
"8","0.791200"
"9","0.527300"
"10","0.341300"
"11","0.188100"
"12","0.126600"
"13","0.083000"
"14","0.060300"
"15","0.038800"
"16","0.028000"
"17","0.015600"
"18","0.013200"
"19","0.007000"
"20","0.006200"
"21","0.004000"
"22","0.003500"
"23","0.001600"
"24","0.000800"
"25","0.001000"
"26","0.001200"
"27","0.000900"
"28","0.000800"
"29","0.000100"
"30","0.000400"
"31","0.000400"
"32","0.000100"
"33","0.000100"
"34","0.000000"
"35","0.000200"
"36","0.000000"
"37","0.000100"
"38","0.000000"
"39","0.000000"
"40","0.000000"
"41","0.000100"
"42","0.000100"
"43","0.000000"
"44","0.000000"
"45","0.000000"
"46","0.000000"
"47","0.000000"
"48","0.000000"
"49","0.000000"
"50","0.000100"
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
