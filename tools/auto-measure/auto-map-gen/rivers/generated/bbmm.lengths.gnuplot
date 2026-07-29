#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","10.493400"
"5","6.505500"
"6","4.087300"
"7","2.630700"
"8","1.763400"
"9","1.219400"
"10","0.838300"
"11","0.585800"
"12","0.403800"
"13","0.291000"
"14","0.209700"
"15","0.144500"
"16","0.107700"
"17","0.077300"
"18","0.058000"
"19","0.042400"
"20","0.032600"
"21","0.020500"
"22","0.020600"
"23","0.012200"
"24","0.008200"
"25","0.006500"
"26","0.005600"
"27","0.003500"
"28","0.001800"
"29","0.002100"
"30","0.001400"
"31","0.000700"
"32","0.001300"
"33","0.000400"
"34","0.000700"
"35","0.000400"
"36","0.000300"
"37","0.000100"
"38","0.000400"
"39","0.000000"
"40","0.000000"
"41","0.000100"
"42","0.000100"
"43","0.000200"
"44","0.000100"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (bbmm) [10000]"
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
