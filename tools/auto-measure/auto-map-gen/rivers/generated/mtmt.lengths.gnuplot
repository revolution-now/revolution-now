#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","12.422600"
"5","4.752300"
"6","1.843900"
"7","1.417900"
"8","0.963400"
"9","0.586600"
"10","0.378100"
"11","0.253300"
"12","0.159700"
"13","0.114800"
"14","0.072800"
"15","0.049800"
"16","0.034200"
"17","0.024700"
"18","0.015800"
"19","0.012500"
"20","0.008600"
"21","0.005800"
"22","0.003900"
"23","0.003000"
"24","0.002600"
"25","0.001700"
"26","0.001800"
"27","0.000400"
"28","0.000700"
"29","0.000600"
"30","0.000300"
"31","0.000300"
"32","0.000100"
"33","0.000200"
"34","0.000100"
"35","0.000100"
"36","0.000100"
"37","0.000100"
"38","0.000100"
"39","0.000200"
"40","0.000000"
"41","0.000100"
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
