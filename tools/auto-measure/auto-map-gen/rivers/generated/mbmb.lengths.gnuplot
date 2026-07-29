#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","8.743800"
"5","5.505000"
"6","3.482900"
"7","2.253100"
"8","1.510800"
"9","1.055400"
"10","0.753100"
"11","0.517100"
"12","0.371600"
"13","0.261200"
"14","0.188300"
"15","0.139200"
"16","0.101500"
"17","0.078400"
"18","0.052900"
"19","0.037400"
"20","0.029200"
"21","0.022600"
"22","0.017600"
"23","0.013000"
"24","0.008300"
"25","0.006800"
"26","0.005600"
"27","0.005400"
"28","0.003300"
"29","0.001500"
"30","0.002100"
"31","0.001900"
"32","0.000600"
"33","0.000800"
"34","0.000600"
"35","0.000500"
"36","0.000500"
"37","0.000300"
"38","0.000500"
"39","0.000200"
"40","0.000300"
"41","0.000100"
"42","0.000000"
"43","0.000000"
"44","0.000000"
"45","0.000000"
"46","0.000100"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (mbmb) [10000]"
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
