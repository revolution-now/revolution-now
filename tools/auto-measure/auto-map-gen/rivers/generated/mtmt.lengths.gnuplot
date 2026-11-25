#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","13.488200"
"5","5.240200"
"6","2.097400"
"7","0.888800"
"8","0.769300"
"9","0.531600"
"10","0.315300"
"11","0.183500"
"12","0.117000"
"13","0.083800"
"14","0.054900"
"15","0.035700"
"16","0.022700"
"17","0.017400"
"18","0.012600"
"19","0.007900"
"20","0.006700"
"21","0.003600"
"22","0.002600"
"23","0.001400"
"24","0.001300"
"25","0.001100"
"26","0.000700"
"27","0.001000"
"28","0.000300"
"29","0.000100"
"30","0.000200"
"31","0.000200"
"32","0.000100"
"33","0.000300"
"34","0.000100"
"35","0.000100"
"36","0.000100"
"37","0.000200"
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
