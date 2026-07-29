#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","8.213900"
"5","5.132000"
"6","3.227500"
"7","2.025600"
"8","1.355800"
"9","0.925300"
"10","0.623700"
"11","0.443500"
"12","0.304600"
"13","0.214100"
"14","0.151100"
"15","0.108700"
"16","0.080300"
"17","0.055600"
"18","0.042000"
"19","0.028800"
"20","0.022300"
"21","0.014500"
"22","0.010900"
"23","0.007900"
"24","0.005900"
"25","0.004000"
"26","0.003500"
"27","0.002700"
"28","0.001800"
"29","0.002000"
"30","0.000900"
"31","0.000900"
"32","0.000600"
"33","0.000300"
"34","0.000100"
"35","0.000100"
"36","0.000200"
"37","0.000000"
"38","0.000000"
"39","0.000100"
"40","0.000200"
"41","0.000100"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (mbmm) [10000]"
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
