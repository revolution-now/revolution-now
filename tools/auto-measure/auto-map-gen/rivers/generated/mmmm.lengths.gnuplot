#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","10.955000"
"5","6.162000"
"6","3.577200"
"7","2.055900"
"8","1.293600"
"9","0.819200"
"10","0.542400"
"11","0.349800"
"12","0.234700"
"13","0.153400"
"14","0.096400"
"15","0.067000"
"16","0.046100"
"17","0.028500"
"18","0.018600"
"19","0.015000"
"20","0.008700"
"21","0.006000"
"22","0.004200"
"23","0.002400"
"24","0.001900"
"25","0.000700"
"26","0.001000"
"27","0.001400"
"28","0.000400"
"29","0.000200"
"30","0.000300"
"31","0.000000"
"32","0.000200"
"33","0.000000"
"34","0.000400"
"35","0.000100"
"36","0.000000"
"37","0.000000"
"38","0.000000"
"39","0.000000"
"40","0.000100"
"41","0.000000"
"42","0.000100"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (mmmm) [10000]"
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
